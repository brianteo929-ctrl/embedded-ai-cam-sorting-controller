"""Offline dataset-pipeline tests; fixtures do not represent waste or model quality."""
import importlib.util
import json
from pathlib import Path
import random
import tempfile
import unittest
from PIL import Image

spec = importlib.util.spec_from_file_location("dataset", Path(__file__).resolve().parents[1] / "tools/data/dataset.py")
data = importlib.util.module_from_spec(spec)
spec.loader.exec_module(data)


class DatasetTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        records = []
        rng = random.Random(42)
        for i, split in enumerate(("training", "validation", "test")):
            image = Image.frombytes("RGB", (24, 24), rng.randbytes(24*24*3))
            name = f"image{i}.png"
            image.save(self.root / name)
            records.append({"path": name, "split": split, "label": "battery", "group": f"object{i}", "reviewed": True})
        self.manifest = {"schema": 1, "archive_sha256": "0"*64,
                         "source": {"url": "https://example.org/data", "author": "fixture", "license": "CC0-1.0"},
                         "records": records}

    def kinds(self):
        return {x["kind"] for x in data.audit(self.root, self.manifest)["issues"]}

    def test_valid_export_and_checksum(self):
        report = data.audit(self.root, self.manifest)
        self.assertTrue(report["ready_for_import"], report["issues"])
        self.assertEqual(report["class_counts"], {"battery": 3})
        self.assertEqual(len(data.digest(self.root / "image0.png")), 64)
        data.prepare(self.root, self.manifest, self.root / "prepared")
        self.assertEqual(len(list((self.root / "prepared/test").glob("battery.*.png"))), 1)
        with self.assertRaises(ValueError):
            data.prepare(self.root, self.manifest, self.root / "prepared")

    def test_duplicates_groups_and_corruption(self):
        (self.root / "image1.png").write_bytes((self.root / "image0.png").read_bytes())
        self.manifest["records"][1]["group"] = "object0"
        (self.root / "image2.png").write_bytes(b"not an image")
        kinds = self.kinds()
        self.assertIn("duplicates_require_deduplication", kinds)
        self.assertIn("possible_near_duplicate_split_leakage", kinds)
        self.assertIn("object_session_split_leakage", kinds)
        self.assertIn("missing_corrupt_or_unsafe_image", kinds)
        with self.assertRaises(ValueError):
            data.prepare(self.root, self.manifest, self.root / "bad-export")
        self.assertFalse((self.root / "bad-export").exists())

    def test_bad_annotations_and_paths(self):
        row = self.manifest["records"][0]
        row.update(label="Plastic", reviewed=False, declared_size=[1, 1],
                   annotations=[{"label": "battery", "bbox": [23, 0, 2, 1]},
                                {"label": "battery", "bbox": [0, 0, float("nan"), 1]},
                                {"label": "battery", "bbox": [0, 0, .1, .1]}])
        kinds = self.kinds()
        self.assertTrue({"unsupported_label", "object_session_review_required", "annotation_size_mismatch",
                         "out_of_bounds_bbox", "invalid_bbox", "tiny_bbox_review_required"} <= kinds)
        row["path"] = "../escape.png"
        self.assertIn("missing_corrupt_or_unsafe_image", self.kinds())
        self.manifest["source"]["url"] = "https://example.org/data?token=redacted"
        self.assertIn("invalid_public_source_url", self.kinds())

    def test_coco_normalization_and_dangling_annotation(self):
        (self.root / "train").mkdir()
        coco = {"images": [{"id": 1, "file_name": "one.jpg", "width": 32, "height": 32}],
                "categories": [{"id": 5, "name": "Alkaline"}],
                "annotations": [{"id": 9, "image_id": 1, "category_id": 5, "bbox": [0, 0, 10, 10]}]}
        path = self.root / "train/_annotations.coco.json"
        path.write_text(json.dumps(coco))
        result = data.import_coco(self.root, self.manifest["source"], "0"*64, {"Alkaline": "battery"})
        self.assertEqual(result["records"][0]["label"], "battery")
        self.assertFalse(result["records"][0]["reviewed"])
        coco["annotations"][0]["image_id"] = 2
        path.write_text(json.dumps(coco))
        with self.assertRaises(ValueError):
            data.import_coco(self.root, {}, "0"*64, {})


if __name__ == "__main__":
    unittest.main()
