"""Regression checks for safe reuse of compiled CI outputs."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SPEC = importlib.util.spec_from_file_location(
    "ci_build_cache", Path(__file__).resolve().parents[2] / "scripts/test/ci-build-cache.py")
cache = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cache)


class BuildCache(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        old_root = cache.ROOT
        cache.ROOT = self.root
        self.addCleanup(setattr, cache, "ROOT", old_root)
        self.git("init", "-q")
        self.git("config", "user.name", "CI Test")
        self.git("config", "user.email", "ci@example.invalid")
        self.write("src/main.cpp", "original")
        self.write("CMakeLists.txt", "version and flags")
        self.write("res/skins/test.xml", "original")
        self.commit()

    def git(self, *args):
        return subprocess.check_output(["git", *args], cwd=self.root)

    def write(self, path, text):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def commit(self):
        self.git("add", "-A")
        self.git("commit", "-qm", "test: change inputs")

    def test_assets_and_docs_reuse_but_source_config_and_environment_invalidate(self):
        before = cache.fingerprint("ubuntu toolchain v1")
        self.write("res/skins/test.xml", "updated")
        self.write("docs/example.md", "documentation")
        self.commit()
        self.assertEqual(before, cache.fingerprint("ubuntu toolchain v1"))
        self.assertNotEqual(before, cache.fingerprint("ubuntu toolchain v2"))
        for path in ("src/main.cpp", "CMakeLists.txt", ".github/workflows/tests.yml"):
            before = cache.fingerprint("environment")
            self.write(path, "new compiled input")
            self.commit()
            self.assertNotEqual(before, cache.fingerprint("environment"))
        before = cache.fingerprint("environment")
        (self.root / "src/main.cpp").unlink()
        self.commit()
        self.assertNotEqual(before, cache.fingerprint("environment"))

    def test_native_agent_instructions_reuse_but_helpers_and_embedded_files_invalidate(self):
        before = cache.fingerprint("environment")
        for path in (".codex/config.toml", ".agents/skills/example/SKILL.md",
                     ".agents/skills/example/references/guide.md",
                     ".agents/skills/example/agents/openai.yaml"):
            self.write(path, "agent configuration")
        self.commit()
        self.assertEqual(before, cache.fingerprint("environment"))
        self.write(".agents/skills/example/scripts/build.py", "compiled helper")
        self.commit()
        self.assertNotEqual(before, cache.fingerprint("environment"))
        self.write("res/mixxx.qrc", '<RCC><qresource><file>../.agents/skills/example/references/guide.md</file></qresource></RCC>')
        self.commit()
        before = cache.fingerprint("environment")
        self.write(".agents/skills/example/references/guide.md", "embedded changed")
        self.commit()
        self.assertNotEqual(before, cache.fingerprint("environment"))

    def test_qrc_embedded_asset_must_invalidate(self):
        self.write("res/mixxx.qrc", '<RCC><qresource><file>skins/test.xml</file></qresource></RCC>')
        self.commit()
        before = cache.fingerprint("environment")
        self.write("res/skins/test.xml", "changed embedded asset")
        self.commit()
        self.assertNotEqual(before, cache.fingerprint("environment"))

    def test_restore_preserves_provenance_and_removes_stale_assets(self):
        for prefix in cache.ASSETS:
            self.write(prefix + "current.txt", "new asset")
        for path in ("build/mixxx-test", "dist-linux/bin/mixxx"):
            self.write(path, "#!/bin/sh\necho 0.0.7-ci.1\n")
            (self.root / path).chmod(0o755)
        self.write("build/lib/shared.so.1", "shared library")
        (self.root / "build/lib/shared.so").symlink_to("shared.so.1")
        self.write("dist-linux/share/mixxx/skins/deleted.xml", "obsolete")
        directory = self.root / "snapshot"
        cache.snapshot(directory, "exact-key")
        provenance = (directory / "provenance.json").read_text()
        shutil.rmtree(self.root / "build")
        shutil.rmtree(self.root / "dist-linux")
        cache.restore(directory, "exact-key")
        self.assertFalse((self.root / "dist-linux/share/mixxx/skins/deleted.xml").exists())
        self.assertEqual((self.root / "dist-linux/share/mixxx/skins/current.txt").read_text(), "new asset")
        self.assertTrue((self.root / "build/lib/shared.so").is_symlink())
        self.assertEqual((directory / "provenance.json").read_text(), provenance)
        with self.assertRaisesRegex(RuntimeError, "fingerprint"):
            cache.restore(directory, "wrong-key")


if __name__ == "__main__":
    unittest.main()
