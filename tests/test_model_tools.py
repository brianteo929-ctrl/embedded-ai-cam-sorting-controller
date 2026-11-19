import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec = importlib.util.spec_from_file_location("model", Path(__file__).resolve().parents[1] / "tools/verify_model.py")
model = importlib.util.module_from_spec(spec)
spec.loader.exec_module(model)


class ModelTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.archive = self.root / "test.zip"
        with zipfile.ZipFile(self.archive, "w") as bundle:
            bundle.writestr("model.bin", b"test fixture, not a model")
            bundle.writestr("LICENSE", b"test fixture, not a license grant")
        self.manifest = dict(project_id=1168582, deployment_version=1, sdk_version="fixture", quantization="int8",
            model_file="model.bin", license_file="LICENSE", validation_report="fixture", threshold=.8,
            model_rom_bytes=100, tensor_arena_bytes=100, estimated_latency_ms=1,
            license="MIT", source_url="https://example.org/model", resize_mode="fit-shortest",
            labels=["battery", "background", "unknown", "multiple_items"], output_type="classification",
            minimum_margin=.1, channels=1, input_width=96, input_height=96,
            archive_sha256=hashlib.sha256(self.archive.read_bytes()).hexdigest(),
            model_sha256=hashlib.sha256(b"test fixture, not a model").hexdigest())

    def test_verified_staging(self):
        model.verify(self.archive, self.manifest, self.root / "staging")
        self.assertTrue((self.root / "staging/model.bin").exists())
        with self.assertRaises(ValueError):
            model.verify(self.archive, self.manifest, self.root / "staging")

    def test_bad_manifest_and_checksum(self):
        for key, value in (("archive_sha256", "bad"), ("model_sha256", "bad"), ("license", None),
                           ("project_id", 1), ("model_rom_bytes", 600000), ("labels", ["unmapped"]),
                           ("resize_mode", "squash"), ("source_url", "https://example.org/?token=redacted")):
            changed = dict(self.manifest, **{key: value})
            with self.assertRaises(ValueError, msg=key):
                model.verify(self.archive, changed)

    def test_path_escape(self):
        with zipfile.ZipFile(self.archive, "a") as bundle:
            bundle.writestr("../escape", b"bad")
        self.manifest["archive_sha256"] = hashlib.sha256(self.archive.read_bytes()).hexdigest()
        with self.assertRaises(ValueError):
            model.verify(self.archive, self.manifest, self.root / "unsafe")
        self.assertFalse((self.root / "unsafe").exists())


if __name__ == "__main__":
    unittest.main()
