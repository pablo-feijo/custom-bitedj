#!/usr/bin/env python3
"""Exercise the guard against real Git indexes."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

SOURCE = Path(__file__).resolve().parents[2] / "scripts/test/check-git-storage.py"

class StoragePolicyTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.git("init", "-q")
        self.script = self.root / "scripts/test/check-git-storage.py"
        self.script.parent.mkdir(parents=True)
        shutil.copyfile(SOURCE, self.script)
        self.allow = self.script.with_name("git-size-exceptions.json")
        self.allow.write_text("{}")

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.root), *args], stderr=subprocess.STDOUT)

    def check(self, success):
        result = subprocess.run([sys.executable, str(self.script)], cwd="/", capture_output=True, text=True)
        self.assertEqual(result.returncode, 0 if success else 1, result.stdout + result.stderr)

    def test_small_file_with_spaces(self):
        (self.root / "small file").write_text("source")
        self.git("add", "small file")
        self.check(True)

    def test_staged_large_blob_cannot_hide_behind_small_working_file(self):
        p = self.root / "large.bin"
        p.write_bytes(b"x" * (1048576 + 1))
        self.git("add", "large.bin")
        p.write_text("small now")
        self.check(False)

    def test_exception_requires_exact_path_and_content(self):
        p = self.root / "fixture.bin"
        p.write_bytes(b"x" * (1048576 + 1))
        self.git("add", "fixture.bin")
        oid = self.git("rev-parse", ":fixture.bin").decode().strip()
        self.allow.write_text(json.dumps({"fixture.bin": oid}))
        self.check(True)
        p.write_bytes(b"y" * (1048576 + 1))
        self.git("add", "fixture.bin")
        self.check(False)

    def test_generated_output_even_when_small(self):
        p = self.root / "deploy/image.img"
        p.parent.mkdir()
        p.write_text("generated")
        self.git("add", "deploy/image.img")
        self.check(False)

    def test_force_added_local_task_is_rejected(self):
        p = self.root / "tasks/roadmap.md"
        p.parent.mkdir()
        p.write_text("local execution log")
        self.git("add", "tasks/roadmap.md")
        self.check(False)

    def test_lfs_configuration_is_rejected(self):
        (self.root / ".gitattributes").write_text("asset.bin filter=lfs\n")
        p = self.root / "asset.bin"
        p.write_text("version https://git-lfs.github.com/spec/v1\n")
        self.git("add", ".gitattributes")
        oid = self.git("hash-object", "-w", "--no-filters", "asset.bin").decode().strip()
        self.git("update-index", "--add", "--cacheinfo", "100644", oid, "asset.bin")
        self.check(False)


if __name__ == "__main__":
    unittest.main()
