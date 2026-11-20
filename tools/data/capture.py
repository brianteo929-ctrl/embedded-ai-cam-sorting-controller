"""Collect original camera JPEGs and atomically update an operator-labeled manifest."""
import argparse
from contextlib import contextmanager
import csv
from datetime import datetime, timezone
import hashlib
import http.client
import io
import math
import os
from pathlib import Path
import re
import socket
import tempfile
import threading
import time
from urllib.parse import urljoin, urlsplit

from PIL import Image

from dataset import REPO, canonical_labels, digest, safe_file

LABELS = {"paper", "cardboard", "plastic_bottle", "food_scraps", "battery",
          "background", "unknown", "multiple_items"}
SPLITS = {"training", "validation", "test"}
FIELDS = ["path", "label", "object_ids", "session_id", "split", "reviewed", "notes",
          "sha256", "width", "height", "captured_at_utc"]
MAX_BYTES = 128 * 1024


class CaptureError(ValueError):
    pass


def identifier(value):
    reserved = {"con", "prn", "aux", "nul", *[f"{p}{n}" for p in ("com", "lpt") for n in range(1, 10)]}
    if not re.fullmatch(r"[a-z][a-z0-9_-]{0,63}", value) or value in reserved:
        raise CaptureError("IDs must be lowercase letters, digits, '-' or '_'; start with a letter; no reserved device names")
    return value


def validate_assignment(label, object_ids, session_id, split):
    if not LABELS <= canonical_labels() or label not in LABELS:
        raise CaptureError("Unsupported initial label; use the authoritative taxonomy")
    identifier(session_id)
    if split not in SPLITS:
        raise CaptureError("Split must be training, validation or test")
    for value in object_ids:
        identifier(value)
    if len(set(object_ids)) != len(object_ids):
        raise CaptureError("Object IDs must be unique")
    expected = 0 if label == "background" else 2 if label == "multiple_items" else 1
    if (label == "multiple_items" and len(object_ids) < expected) or (label != "multiple_items" and len(object_ids) != expected):
        raise CaptureError("Background needs no object ID; single items need one; multiple_items needs at least two")


def endpoint_parts(endpoint):
    try:
        parts = urlsplit(endpoint)
        port = parts.port
    except ValueError as error:
        raise CaptureError("Invalid endpoint") from error
    if parts.scheme not in {"http", "https"} or not parts.hostname or parts.username or parts.password or parts.query or parts.fragment:
        raise CaptureError("Use an HTTP(S) endpoint without credentials, query parameters or fragments")
    if port is not None and not 1 <= port <= 65535:
        raise CaptureError("Invalid endpoint port")
    return parts


def download(endpoint, timeout=5.0):
    if not math.isfinite(timeout) or not 0 < timeout <= 30:
        raise CaptureError("Timeout must be greater than zero and at most 30 seconds")
    original = endpoint_parts(endpoint)
    deadline = time.monotonic() + timeout
    for _ in range(3):
        parts = endpoint_parts(endpoint)
        if (parts.scheme, parts.hostname) != (original.scheme, original.hostname):
            raise CaptureError("Redirect must retain the same host and scheme")
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise CaptureError("HTTP deadline exceeded")
        connection_type = http.client.HTTPSConnection if parts.scheme == "https" else http.client.HTTPConnection
        connection = connection_type(parts.hostname, parts.port, timeout=remaining)
        timer = None
        try:
            connection.connect()
            transport = connection.sock

            def abort():
                try:
                    transport.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass

            timer = threading.Timer(max(0, deadline-time.monotonic()), abort)
            timer.daemon = True
            timer.start()
            connection.request("GET", parts.path or "/", headers={"Accept": "image/jpeg", "Cache-Control": "no-cache"})
            response = connection.getresponse()
            if response.status in {301, 302, 303, 307, 308}:
                location = response.getheader("Location")
                if not location:
                    raise CaptureError("Redirect has no Location")
                endpoint = urljoin(endpoint, location)
                continue
            if response.status != 200:
                raise CaptureError(f"HTTP status {response.status}; no image saved")
            if response.getheader("Content-Type", "").split(";", 1)[0].strip().lower() != "image/jpeg":
                raise CaptureError("Response is not image/jpeg")
            if response.getheader("Content-Encoding", "identity").lower() != "identity":
                raise CaptureError("Encoded HTTP response is not supported")
            length = response.getheader("Content-Length")
            if length is not None and (not length.isdigit() or not 0 < int(length) <= MAX_BYTES):
                raise CaptureError("Invalid or oversized Content-Length")
            chunks, size = [], 0
            while True:
                if time.monotonic() >= deadline:
                    raise CaptureError("HTTP deadline exceeded")
                chunk = response.read1(min(16384, MAX_BYTES+1-size))
                if not chunk:
                    break
                chunks.append(chunk)
                size += len(chunk)
                if size > MAX_BYTES:
                    raise CaptureError("JPEG exceeds the camera's 128 KiB response limit")
            if length is not None and size != int(length):
                raise CaptureError("Incomplete HTTP body")
            if time.monotonic() >= deadline:
                raise CaptureError("HTTP deadline exceeded")
            return b"".join(chunks)
        except (OSError, http.client.HTTPException) as error:
            raise CaptureError(f"HTTP transport failed ({type(error).__name__}); no image saved") from error
        finally:
            if timer:
                timer.cancel()
            connection.close()
    raise CaptureError("Too many HTTP redirects")


def validate_jpeg(payload, width, height):
    if not 0 < width <= 4096 or not 0 < height <= 4096 or width*height > 1_000_000:
        raise CaptureError("Invalid expected image dimensions")
    if not payload.startswith(b"\xff\xd8") or not payload.endswith(b"\xff\xd9"):
        raise CaptureError("Missing JPEG start/end marker")
    try:
        with Image.open(io.BytesIO(payload)) as image:
            if image.format != "JPEG" or image.size != (width, height):
                raise CaptureError("JPEG dimensions or format differ from the expected camera configuration")
            image.verify()
        with Image.open(io.BytesIO(payload)) as image:
            image.load()
    except (OSError, SyntaxError, Image.DecompressionBombError) as error:
        raise CaptureError("Corrupt JPEG; no image saved") from error


@contextmanager
def collection_lock(root):
    root = root.resolve()
    if root.is_relative_to(REPO) and not root.is_relative_to(REPO / "captures"):
        raise CaptureError("Repository captures must be inside ignored captures/; external output folders are allowed")
    root.mkdir(parents=True, exist_ok=True)
    lock = root / ".capture.lock"
    try:
        descriptor = os.open(lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except FileExistsError as error:
        raise CaptureError("Collection is locked; stop the other collector or inspect a stale .capture.lock before removing it") from error
    try:
        with os.fdopen(descriptor, "w") as stream:
            stream.write(str(os.getpid()))
        yield root
    finally:
        lock.unlink()


def load_rows(root):
    path = root / "metadata.csv"
    rows = []
    if path.exists():
        if not path.read_bytes().endswith(b"\n"):
            raise CaptureError("Partial metadata.csv; recover it before appending")
        with path.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream, strict=True)
            if reader.fieldnames != FIELDS:
                raise CaptureError("Unexpected CSV schema; no metadata changed")
            for row in reader:
                if set(row) != set(FIELDS) or any(value is None for value in row.values()):
                    raise CaptureError("Partial metadata row; no metadata changed")
                validate_assignment(row["label"], row["object_ids"].split(";") if row["object_ids"] else [], row["session_id"], row["split"])
                file = safe_file(root, row["path"])
                if file.parent != root / row["session_id"] or file.suffix != ".jpg" or not file.is_file() or digest(file) != row["sha256"]:
                    raise CaptureError("Capture path or checksum disagrees with metadata; inspect the collection")
                check_split(rows, row["object_ids"].split(";") if row["object_ids"] else [], row["session_id"], row["split"])
                rows.append(row)
    indexed = {safe_file(root, row["path"]) for row in rows}
    if len(indexed) != len(rows) or set(root.glob("*/*.jpg")) != indexed:
        raise CaptureError("Unindexed or repeated capture; inspect interruption recovery before appending")
    return rows


def check_split(rows, object_ids, session_id, split):
    for row in rows:
        shared_object = set(object_ids) & set(row["object_ids"].split(";"))
        if (row["session_id"] == session_id or shared_object) and row["split"] != split:
            raise CaptureError("Object/session already belongs to another split; no capture requested")


def write_metadata(root, rows):
    descriptor, name = tempfile.mkstemp(prefix=".capture-meta-", suffix=".tmp", dir=root)
    try:
        with os.fdopen(descriptor, "w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(stream, fieldnames=FIELDS)
            writer.writeheader()
            writer.writerows(rows)
            stream.flush()
            os.fsync(stream.fileno())
        for attempt in range(3):
            try:
                os.replace(name, root / "metadata.csv")
                break
            except PermissionError as error:
                if getattr(error, "winerror", None) not in {32, 33} or attempt == 2:
                    raise
                time.sleep(.05 * (attempt+1))
    finally:
        Path(name).unlink(missing_ok=True)


def save_capture(root, rows, payload, *, label, object_ids, session_id, split, notes="", width=320, height=240):
    validate_assignment(label, object_ids, session_id, split)
    check_split(rows, object_ids, session_id, split)
    validate_jpeg(payload, width, height)
    sha = hashlib.sha256(payload).hexdigest()
    if any(row["sha256"] == sha for row in rows):
        return None
    folder = root / session_id
    folder.mkdir(exist_ok=True)
    if folder.resolve() != folder:
        raise CaptureError("Session folder must not be a symlink or junction")
    file = safe_file(root, f"{session_id}/{label}.{sha}.jpg")
    row = dict(path=file.relative_to(root).as_posix(), label=label, object_ids=";".join(sorted(object_ids)),
               session_id=session_id, split=split, reviewed="false", notes=notes, sha256=sha,
               width=str(width), height=str(height), captured_at_utc=datetime.now(timezone.utc).isoformat())
    with file.open("xb") as stream:
        try:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        except BaseException:
            stream.close()
            file.unlink()
            raise
    # A crash may leave an unindexed image, but must never leave a row without its image.
    write_metadata(root, [*rows, row])
    rows.append(row)
    return row


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--endpoint", default="http://192.168.4.1/capture")
    parser.add_argument("--label", required=True, choices=sorted(LABELS))
    parser.add_argument("--object-id", action="append", default=[])
    parser.add_argument("--session", required=True)
    parser.add_argument("--split", required=True, choices=sorted(SPLITS))
    parser.add_argument("--count", type=int, default=1)
    parser.add_argument("--interval", type=float, default=3)
    parser.add_argument("--timeout", type=float, default=5)
    parser.add_argument("--notes", default="")
    parser.add_argument("--output-dir", type=Path, default=REPO / "captures")
    parser.add_argument("--width", type=int, default=320)
    parser.add_argument("--height", type=int, default=240)
    args = parser.parse_args(argv)
    validate_assignment(args.label, args.object_id, args.session, args.split)
    endpoint_parts(args.endpoint)
    if not 1 <= args.count <= 100 or not math.isfinite(args.interval) or not 0 <= args.interval <= 60:
        raise CaptureError("Use 1-100 attempts and a finite interval from 0 to 60 seconds")
    saved = 0
    with collection_lock(args.output_dir) as root:
        rows = load_rows(root)
        check_split(rows, args.object_id, args.session, args.split)
        for index in range(args.count):
            if index:
                time.sleep(args.interval)
            payload = download(args.endpoint, args.timeout)
            row = save_capture(root, rows, payload, label=args.label, object_ids=args.object_id,
                               session_id=args.session, split=args.split, notes=args.notes,
                               width=args.width, height=args.height)
            if row:
                saved += 1
                print(f"Saved {row['path']} ({args.width}x{args.height}); review pending")
            else:
                print("Skipped exact duplicate; vary the scene before capturing again")
    print(f"Saved {saved}/{args.count} attempts; metadata: {root / 'metadata.csv'}")
    return 0 if saved == args.count else 2


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (CaptureError, OSError, csv.Error) as error:
        raise SystemExit(f"Capture stopped: {error}")
    except KeyboardInterrupt:
        raise SystemExit("Capture stopped by operator; previously committed rows are retained")
