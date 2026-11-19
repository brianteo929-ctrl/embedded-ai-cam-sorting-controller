"""Verify a licensed, versioned local export before extracting it for integration."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import stat
from urllib.parse import urlsplit
import zipfile


def verify(archive, manifest, output=None):
    required = ("deployment_version", "sdk_version", "quantization", "model_file", "license_file",
                "validation_report", "threshold", "model_rom_bytes", "tensor_arena_bytes", "estimated_latency_ms")
    if any(manifest.get(key) is None or manifest.get(key) == "" for key in required):
        raise ValueError("Complete the model manifest from the actual export and validation report")
    if manifest.get("project_id") != 1168582 or manifest.get("resize_mode") != "fit-shortest":
        raise ValueError("Project or resize contract mismatch")
    if manifest.get("license") not in {"CC-BY-4.0", "CC0-1.0", "MIT", "Apache-2.0", "BSD-3-Clause"}:
        raise ValueError("Model redistribution license is missing or requires separate review")
    url = urlsplit(manifest.get("source_url", ""))
    if url.scheme != "https" or not url.netloc or url.query or url.fragment or url.username or url.password:
        raise ValueError("Use a canonical source URL without authentication material")
    labels = manifest.get("labels", [])
    taxonomy = (Path(__file__).resolve().parents[1] / "components/fsm/waste_labels.def").read_text()
    allowed = set(re.findall(r'WASTE_LABEL\("([a-z_]+)"', taxonomy))
    if not 2 <= len(labels) <= 16 or len(set(labels)) != len(labels) or not set(labels) <= allowed:
        raise ValueError("Unsupported labels")
    if manifest.get("output_type") not in {"classification", "fomo"}:
        raise ValueError("Unsupported output type")
    if manifest["output_type"] == "classification" and not {"background", "unknown", "multiple_items"} <= set(labels):
        raise ValueError("Classification export must include rejection classes")
    if manifest["output_type"] == "classification" and not 0 < manifest.get("minimum_margin", 0) <= 1:
        raise ValueError("Validation-derived classification margin required")
    if not 0 < manifest["threshold"] <= 1 or manifest.get("channels") not in (1, 3):
        raise ValueError("Invalid threshold or channel count")
    if any(not isinstance(manifest.get(k), int) or not 1 <= manifest[k] <= 256 for k in ("input_width", "input_height")):
        raise ValueError("Unsupported image dimensions")
    for key, limit in (("model_rom_bytes", 512*1024), ("tensor_arena_bytes", 512*1024), ("estimated_latency_ms", 1000)):
        if not 0 < manifest[key] <= limit:
            raise ValueError(f"{key} exceeds the project budget or is invalid")
    with archive.open("rb") as stream:
        actual = hashlib.file_digest(stream, "sha256").hexdigest()
    if actual != manifest.get("archive_sha256"):
        raise ValueError("Export archive checksum mismatch")
    with zipfile.ZipFile(archive) as bundle:
        names = set()
        total = 0
        for entry in bundle.infolist():
            path = PurePosixPath(entry.filename)
            if path.is_absolute() or ".." in path.parts or "\\" in entry.filename or ":" in entry.filename:
                raise ValueError("Unsafe archive path")
            if entry.filename.casefold() in names or stat.S_ISLNK(entry.external_attr >> 16):
                raise ValueError("Duplicate archive path or symlink")
            names.add(entry.filename.casefold())
            total += entry.file_size
        if total > 256*1024*1024:
            raise ValueError("Export exceeds bounded extraction size")
        model = bundle.read(manifest["model_file"])
        if hashlib.sha256(model).hexdigest() != manifest.get("model_sha256"):
            raise ValueError("Model checksum mismatch")
        if not bundle.read(manifest["license_file"]).strip():
            raise ValueError("Empty license file")
        if output:
            if output.exists():
                raise ValueError("Use a new staging directory; existing artifacts are not overwritten")
            output.mkdir(parents=True)
            bundle.extractall(output)
    return actual


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--extract", type=Path)
    args = parser.parse_args()
    try:
        checksum = verify(args.archive, json.loads(args.manifest.read_text()), args.extract)
        print(f"Verified export {checksum}; CMake/SDK compatibility and inference still require testing")
    except (ValueError, KeyError, OSError, TypeError, zipfile.BadZipFile) as error:
        raise SystemExit(f"Model verification failed: {error}")


if __name__ == "__main__":
    main()
