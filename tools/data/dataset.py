"""Audit local COCO exports and prepare reviewed classification images for Studio."""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path
import re
from urllib.parse import urlsplit

from PIL import Image, ImageOps

REPO = Path(__file__).resolve().parents[2]
SPLITS = {"train": "training", "valid": "validation", "test": "test"}


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def canonical_labels():
    text = (REPO / "components/fsm/waste_labels.def").read_text()
    return set(re.findall(r'WASTE_LABEL\("([a-z_]+)", BIN_[A-Z]+\)', text))


def safe_file(root, relative):
    if not isinstance(relative, str) or not relative or Path(relative).is_absolute():
        raise ValueError("Image paths must be relative")
    result = (root / relative).resolve()
    if not result.is_relative_to(root.resolve()):
        raise ValueError("Image path escapes dataset root")
    return result


def import_coco(root, source, archive_sha256, mapping):
    records = []
    for folder, split in SPLITS.items():
        annotation = root / folder / "_annotations.coco.json"
        if not annotation.exists():
            continue
        data = json.loads(annotation.read_text(encoding="utf-8"))
        categories = {x["id"]: x["name"] for x in data["categories"]}
        if len(categories) != len(data["categories"]):
            raise ValueError("Duplicate COCO category IDs")
        image_ids = {x["id"] for x in data["images"]}
        if len(image_ids) != len(data["images"]):
            raise ValueError("Duplicate COCO image IDs")
        boxes = defaultdict(list)
        seen = set()
        for item in data["annotations"]:
            if item["id"] in seen or item["image_id"] not in image_ids or item["category_id"] not in categories:
                raise ValueError("Duplicate or dangling COCO annotation ID")
            seen.add(item["id"])
            boxes[item["image_id"]].append({"label": categories[item["category_id"]], "bbox": item["bbox"]})
        for item in data["images"]:
            relative = (Path(folder) / item["file_name"]).as_posix()
            safe_file(root, relative)
            annotations = boxes[item["id"]]
            raw = annotations[0]["label"] if len(annotations) == 1 else None
            label = mapping.get(raw, raw) if raw else "background" if not annotations else "multiple_items"
            records.append({"path": relative, "split": split, "label": label,
                            "group": None, "reviewed": False, "annotations": annotations,
                            "declared_size": [item["width"], item["height"]]})
    if not records:
        raise ValueError("No train/valid/test COCO images found")
    return {"schema": 1, "source": source, "archive_sha256": archive_sha256, "records": records}


def audit(root, manifest):
    issues, records, hashes, pixel_hashes = [], [], defaultdict(list), defaultdict(list)
    classes, splits, sizes, objects = Counter(), Counter(), Counter(), Counter()
    class_splits = defaultdict(Counter)
    groups, paths = defaultdict(set), set()
    labels = canonical_labels()

    def issue(kind, path):
        issues.append({"kind": kind, "path": path})

    source = manifest.get("source", {})
    url = urlsplit(source.get("url", ""))
    if url.scheme != "https" or not url.netloc or url.query or url.fragment or url.username or url.password:
        issue("invalid_public_source_url", "manifest")
    if source.get("license") not in {"CC-BY-4.0", "CC0-1.0", "MIT", "BSD-3-Clause"} or not source.get("author"):
        issue("unreviewed_license_or_attribution", "manifest")
    if not re.fullmatch(r"[0-9a-f]{64}", manifest.get("archive_sha256", "")):
        issue("missing_archive_checksum", "manifest")
    for row in manifest.get("records", []):
        path = row.get("path", "")
        if path in paths:
            issue("repeated_manifest_path", path)
        paths.add(path)
        if row.get("label") not in labels:
            issue("unsupported_label", path)
        if row.get("split") not in SPLITS.values():
            issue("invalid_split", path)
        if not row.get("group") or not row.get("reviewed"):
            issue("object_session_review_required", path)
        else:
            groups[row["group"]].add(row["split"])
        try:
            file = safe_file(root, path)
            sha = digest(file)
            with Image.open(file) as original:
                original.load()
                width, height = original.size
                if row.get("declared_size", [width, height]) != [width, height]:
                    issue("annotation_size_mismatch", path)
                if original.getexif().get(274, 1) != 1 and row.get("annotations"):
                    issue("annotation_exif_rotation_requires_review", path)
                rgb = ImageOps.exif_transpose(original).convert("RGB")
                pixel_sha = hashlib.sha256(str(rgb.size).encode() + rgb.tobytes()).hexdigest()
                gray = rgb.convert("L").resize((9, 8), Image.Resampling.BILINEAR)
                px = list(gray.get_flattened_data())
                dhash = sum((px[y*9+x] > px[y*9+x+1]) << (y*8+x) for y in range(8) for x in range(8))
        except (OSError, ValueError, Image.DecompressionBombError):
            issue("missing_corrupt_or_unsafe_image", path)
            continue
        for box in row.get("annotations", []):
            bbox = box.get("bbox", [])
            if len(bbox) != 4 or any(not isinstance(v, (int, float)) or not math.isfinite(v) for v in bbox):
                issue("invalid_bbox", path)
                continue
            x, y, w, h = bbox
            if x < 0 or y < 0 or w <= 0 or h <= 0 or x+w > width or y+h > height:
                issue("out_of_bounds_bbox", path)
            elif w*h / (width*height) < .001:
                issue("tiny_bbox_review_required", path)
            objects[box.get("label", "")] += 1
        hashes[sha].append(path)
        pixel_hashes[pixel_sha].append(path)
        label = row.get("label") or "<missing>"
        split = row.get("split") or "<missing>"
        classes[label] += 1
        splits[split] += 1
        class_splits[label][split] += 1
        sizes[f"{width}x{height}"] += 1
        records.append({"path": path, "sha256": sha, "pixel_sha256": pixel_sha,
                        "dhash": dhash, "split": row["split"], "label": row["label"]})
    duplicates = [v for v in hashes.values() if len(v) > 1]
    pixel_duplicates = [v for v in pixel_hashes.values() if len(v) > 1]
    for group, split in groups.items():
        if len(split) > 1:
            issue("object_session_split_leakage", group)
    # Five bands guarantee a shared band when Hamming distance <= 4.
    buckets, near = defaultdict(list), []
    for i, row in enumerate(records):
        candidates = set()
        for band in range(5):
            candidates.update(buckets[(band, (row["dhash"] >> (13*band)) & 8191)])
        for j in sorted(candidates):
            other = records[j]
            if (row["dhash"] ^ other["dhash"]).bit_count() <= 4:
                pair = [other["path"], row["path"]]
                near.append(pair)
                if row["split"] != other["split"]:
                    issue("possible_near_duplicate_split_leakage", " | ".join(pair))
        for band in range(5):
            buckets[(band, (row["dhash"] >> (13*band)) & 8191)].append(i)
    if duplicates or pixel_duplicates:
        issue("duplicates_require_deduplication", "dataset")
    for split in SPLITS.values():
        if not splits[split]:
            issue("missing_split", split)
    if not records:
        issue("empty_dataset", "dataset")
    return {"schema": 1, "ready_for_import": not issues, "images": len(records),
            "class_counts": dict(classes), "split_counts": dict(splits), "sizes": dict(sizes),
            "class_split_counts": {k: dict(v) for k, v in class_splits.items()},
            "class_imbalance_ratio": max(classes.values()) / min(classes.values()) if classes else None,
            "raw_object_counts": dict(objects), "duplicates": duplicates,
            "decoded_duplicates": pixel_duplicates, "near_duplicate_pairs": near,
            "issues": issues, "files": records,
            "limitations": ["dHash is a review heuristic, not proof of distinct physical objects",
                            "Lighting, pose, occlusion, background and box quality require visual review",
                            "No model training or performance measurement is performed"]}


def prepare(root, manifest, output):
    report = audit(root, manifest)
    if not report["ready_for_import"]:
        raise ValueError(f"Audit has {len(report['issues'])} unresolved issues; no files exported")
    if output.exists():
        raise ValueError("Use a new output directory; existing exports are not overwritten")
    output.mkdir(parents=True)
    for row in report["files"]:
        folder = output / row["split"]
        folder.mkdir(exist_ok=True)
        # Studio infers the class from the filename prefix; preserve original geometry.
        name = row["label"] + "." + row["sha256"] + ".png"
        with Image.open(safe_file(root, row["path"])) as image:
            ImageOps.exif_transpose(image).convert("RGB").save(folder / name)
    write_json(output / "audit.json", report)
    write_json(output / "provenance.json", manifest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    checksum = commands.add_parser("checksum")
    checksum.add_argument("archive", type=Path)
    checksum.add_argument("--expected")
    convert = commands.add_parser("import-coco")
    convert.add_argument("root", type=Path)
    convert.add_argument("--source", required=True, choices=json.loads((Path(__file__).with_name("sources.json")).read_text()))
    convert.add_argument("--archive", required=True, type=Path)
    convert.add_argument("--mapping", type=Path)
    convert.add_argument("--output", required=True, type=Path)
    for action in ("audit", "prepare"):
        cmd = commands.add_parser(action)
        cmd.add_argument("root", type=Path)
        cmd.add_argument("manifest", type=Path)
        cmd.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if args.command == "checksum":
        sha = digest(args.archive)
        if args.expected and sha != args.expected.lower():
            raise ValueError("Archive checksum mismatch")
        print(sha)
    elif args.command == "import-coco":
        source = json.loads(Path(__file__).with_name("sources.json").read_text())[args.source]
        mapping = json.loads(args.mapping.read_text()) if args.mapping else {}
        write_json(args.output, import_coco(args.root, source, digest(args.archive), mapping))
    else:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        if args.command == "prepare":
            prepare(args.root, manifest, args.output)
            print("Prepared reviewed training/validation/test folders")
        else:
            report = audit(args.root, manifest)
            write_json(args.output, report)
            print(f"Audited {report['images']} images; {len(report['issues'])} issues")
            return 0 if report["ready_for_import"] else 2
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, KeyError, OSError) as error:
        raise SystemExit(f"Dataset operation failed: {error}")
