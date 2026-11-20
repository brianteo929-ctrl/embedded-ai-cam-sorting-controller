"""Offline camera collection tests against a loopback HTTP server."""
from contextlib import redirect_stdout
import csv
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import io
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import DEFAULT, patch

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/data"))
import capture


def jpeg(color):
    buffer = io.BytesIO()
    Image.new("RGB", (320, 240), color).save(buffer, format="JPEG")
    return buffer.getvalue()


class CaptureTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.payload = jpeg("red")
        payload = self.payload

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_):
                pass

            def do_GET(self):
                if self.path == "/slow":
                    time.sleep(.15)
                if self.path.startswith("/redirect"):
                    self.send_response(307)
                    target = "http://example.org/capture" if self.path.endswith("external") else "/capture"
                    if self.path == "/redirect-port":
                        target = f"http://127.0.0.1:{self.server.other_port}/capture"
                    self.send_header("Location", target)
                    self.end_headers()
                    return
                status = 503 if self.path == "/status" else 200
                body = {"/html": b"<html>Error</html>", "/corrupt": b"\xff\xd8bad\xff\xd9",
                        "/truncated": payload[:-10], "/oversize": b"x"*(capture.MAX_BYTES+1)}.get(self.path, payload)
                self.send_response(status)
                self.send_header("Content-Type", "text/html" if self.path == "/type" else "image/jpeg")
                self.send_header("Content-Length", str(len(body)+(20 if self.path == "/partial" else 0)))
                self.end_headers()
                try:
                    if self.path == "/drip":
                        for value in body:
                            self.wfile.write(bytes([value]))
                            self.wfile.flush()
                            time.sleep(.02)
                    else:
                        self.wfile.write(body)
                except (BrokenPipeError, ConnectionResetError, ConnectionAbortedError):
                    pass

        self.server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        thread = threading.Thread(target=self.server.serve_forever, kwargs={"poll_interval": .01}, daemon=True)
        thread.start()
        self.addCleanup(self.server.server_close)
        self.addCleanup(self.server.shutdown)
        self.endpoint = f"http://127.0.0.1:{self.server.server_port}"
        self.assignment = dict(label="paper", object_ids=["paper01"], session_id="session01", split="training")

    def save(self, rows, payload=None, **changes):
        return capture.save_capture(self.root, rows, self.payload if payload is None else payload,
                                    **dict(self.assignment, **changes))

    def test_http_capture_csv_creation_and_original_bytes(self):
        with redirect_stdout(io.StringIO()):
            result = capture.main(["--endpoint", self.endpoint+"/redirect", "--label", "paper",
                                   "--object-id", "paper01", "--session", "session01", "--split", "training",
                                   "--output-dir", str(self.root), "--notes", 'original, full frame\n"review"'])
        self.assertEqual(result, 0)
        rows = capture.load_rows(self.root)
        self.assertEqual(len(rows), 1)
        row = rows[0]
        self.assertEqual((self.root / row["path"]).read_bytes(), self.payload)
        self.assertEqual(row["sha256"], hashlib.sha256(self.payload).hexdigest())
        self.assertEqual((row["width"], row["height"], row["reviewed"]), ("320", "240", "false"))
        self.assertEqual(row["notes"], 'original, full frame\n"review"')
        self.assertFalse((self.root / ".capture.lock").exists())

    def test_invalid_status_type_size_and_incomplete_response(self):
        for route in ("/status", "/type", "/oversize", "/partial", "/redirect-external"):
            with self.subTest(route=route), self.assertRaises(capture.CaptureError):
                capture.download(self.endpoint+route)
        self.assertFalse((self.root / "metadata.csv").exists())

    def test_timeout(self):
        for route in ("/slow", "/drip"):
            start = time.monotonic()
            with self.subTest(route=route), self.assertRaises(capture.CaptureError):
                capture.download(self.endpoint+route, timeout=.03)
            self.assertLess(time.monotonic()-start, .5)

    def test_same_host_cross_port_redirect(self):
        target = ThreadingHTTPServer(("127.0.0.1", 0), self.server.RequestHandlerClass)
        threading.Thread(target=target.serve_forever, kwargs={"poll_interval": .01}, daemon=True).start()
        self.addCleanup(target.server_close)
        self.addCleanup(target.shutdown)
        self.server.other_port = target.server_port
        self.assertEqual(capture.download(self.endpoint+"/redirect-port"), self.payload)

    def test_duplicate_batch_reports_incomplete_count(self):
        with redirect_stdout(io.StringIO()):
            result = capture.main(["--endpoint", self.endpoint+"/capture", "--label", "paper",
                                   "--object-id", "paper01", "--session", "session01", "--split", "training",
                                   "--output-dir", str(self.root), "--count", "2", "--interval", "0"])
        self.assertEqual(result, 2)
        self.assertEqual(len(capture.load_rows(self.root)), 1)

    def test_html_corruption_truncation_and_dimensions(self):
        for route in ("/html", "/corrupt", "/truncated"):
            with self.subTest(route=route), self.assertRaises(capture.CaptureError):
                self.save([], capture.download(self.endpoint+route))
        with self.assertRaises(capture.CaptureError):
            self.save([], width=96, height=96)
        self.assertFalse(list(self.root.rglob("*.jpg")))

    def test_duplicate_and_csv_append(self):
        rows = []
        self.save(rows)
        before = (self.root / "metadata.csv").read_bytes()
        self.assertIsNone(self.save(rows))
        self.assertEqual((self.root / "metadata.csv").read_bytes(), before)
        self.save(rows, jpeg("blue"), notes="second placement")
        self.assertEqual(len(capture.load_rows(self.root)), 2)
        with (self.root / "metadata.csv").open(newline="", encoding="utf-8") as stream:
            self.assertEqual(next(csv.reader(stream)), capture.FIELDS)

    def test_filename_collision_never_overwrites(self):
        folder = self.root / "session01"
        folder.mkdir()
        path = folder / f"paper.{hashlib.sha256(self.payload).hexdigest()}.jpg"
        path.write_bytes(b"existing file")
        with self.assertRaises(FileExistsError):
            self.save([])
        self.assertEqual(path.read_bytes(), b"existing file")

    def test_labels_ids_splits_and_endpoint_validation(self):
        for changes in (dict(label="Paper"), dict(label="plastic_botel"), dict(split="train"),
                        dict(session_id="../escape"), dict(session_id="con"), dict(object_ids=[]),
                        dict(label="background"), dict(label="multiple_items")):
            with self.subTest(changes=changes), self.assertRaises(capture.CaptureError):
                capture.validate_assignment(**dict(self.assignment, **changes))
        for endpoint in ("file:///capture", "http://name:secret@localhost/capture", "http://localhost/capture?token=test"):
            with self.subTest(endpoint=endpoint), self.assertRaises(capture.CaptureError):
                capture.endpoint_parts(endpoint)
        capture.validate_assignment("background", [], "session01", "training")
        capture.validate_assignment("multiple_items", ["paper01", "eraser01"], "session01", "training")

    def test_whole_object_and_session_split_guard(self):
        rows = []
        self.save(rows)
        for objects, session in ((["paper01"], "session02"), (["paper02"], "session01"),
                                 (["paper01", "eraser01"], "session02")):
            with self.assertRaises(capture.CaptureError):
                capture.check_split(rows, objects, session, "test")
        capture.check_split(rows, ["paper02"], "session02", "test")
        self.save(rows, jpeg("blue"))
        rows[-1]["split"] = "test"
        capture.write_metadata(self.root, rows)
        with self.assertRaisesRegex(capture.CaptureError, "another split"):
            capture.load_rows(self.root)

    def test_metadata_write_interruption_retains_old_csv_and_flags_orphan(self):
        rows = []
        self.save(rows)
        before = (self.root / "metadata.csv").read_bytes()
        with patch.object(capture.os, "replace", side_effect=OSError("simulated interruption")):
            with self.assertRaises(OSError):
                self.save(rows, jpeg("blue"))
        self.assertEqual((self.root / "metadata.csv").read_bytes(), before)
        self.assertFalse(list(self.root.glob(".capture-meta-*.tmp")))
        with self.assertRaisesRegex(capture.CaptureError, "Unindexed"):
            capture.load_rows(self.root)

    def test_interrupt_after_atomic_replace_keeps_image_and_csv_consistent(self):
        rows = []
        self.save(rows)
        replace = capture.os.replace

        def interrupted_replace(source, destination):
            replace(source, destination)
            raise KeyboardInterrupt()

        with patch.object(capture.os, "replace", side_effect=interrupted_replace):
            with self.assertRaises(KeyboardInterrupt):
                self.save(rows, jpeg("blue"))
        self.assertEqual(len(capture.load_rows(self.root)), 2)

    def test_windows_sharing_violation_has_bounded_retry(self):
        busy = PermissionError("simulated sharing violation")
        busy.winerror = 32
        replace = capture.os.replace
        with patch.object(capture.os, "replace", wraps=replace, side_effect=[busy, DEFAULT]) as mocked:
            capture.write_metadata(self.root, [])
            self.assertEqual(mocked.call_count, 2)
        self.assertEqual(capture.load_rows(self.root), [])
        with patch.object(capture.os, "replace", side_effect=busy) as mocked:
            with self.assertRaises(PermissionError):
                capture.write_metadata(self.root, [])
            self.assertEqual(mocked.call_count, 3)
        with patch.object(capture.os, "replace", wraps=replace):
            capture.write_metadata(self.root, [])
        self.assertEqual(capture.load_rows(self.root), [])

    def test_partial_metadata_and_modified_image_are_rejected(self):
        rows = []
        self.save(rows)
        path = self.root / "metadata.csv"
        original = path.read_bytes()
        path.write_bytes(original[:-8])
        with self.assertRaisesRegex(capture.CaptureError, "Partial"):
            capture.load_rows(self.root)
        path.write_bytes(original)
        (self.root / rows[0]["path"]).write_bytes(jpeg("blue"))
        with self.assertRaisesRegex(capture.CaptureError, "checksum"):
            capture.load_rows(self.root)

    def test_collection_lock_excludes_other_writer(self):
        with capture.collection_lock(self.root):
            with self.assertRaisesRegex(capture.CaptureError, "locked"):
                with capture.collection_lock(self.root):
                    self.fail("Second collector acquired the lock")
        self.assertFalse((self.root / ".capture.lock").exists())


if __name__ == "__main__":
    unittest.main()
