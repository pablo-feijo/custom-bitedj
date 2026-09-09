"""Guard local deliverables against stale sources and incompatible artifacts."""
import contextlib
import importlib.util
import io
import json
import os
import shutil
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('build_support', Path(__file__).resolve().parents[2] / 'scripts/build/build-support.py')
build = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build)


class BuildSupport(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.patch = patch.object(build, 'ROOT', self.root)
        self.patch.start()
        self.addCleanup(self.patch.stop)
        self.git('init', '-q', '-b', 'codex/build-test')
        self.git('config', 'user.name', 'Test')
        self.git('config', 'user.email', 'test@example.invalid')
        self.write('.gitignore', 'dist-linux/\nstate.json\n')
        self.write('CMakeLists.txt', 'set(BITEDJ_VERSION "0.0.7")\nset(BITEDJ_VERSION_PRERELEASE "codex-build-test.1")\n')
        self.write('src/main.cpp', 'original')
        self.git('add', '.')
        self.git('commit', '-qm', 'test: fixture')
        self.dist = self.root / 'dist-linux'
        self.binary(183)
        self.write('dist-linux/share/mixxx/skins/test.xml', 'asset')

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root), *args])

    def write(self, name, text):
        p = self.root / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)

    def binary(self, machine):
        p = self.dist / 'bin/mixxx'
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(b'\x7fELF\x02\x01' + b'\0' * 12 + machine.to_bytes(2, 'little'))
        p.chmod(0o755)

    def call(self, action):
        args = ['build-support.py', action, '--directory', str(self.dist), '--platform', 'linux/arm64', '--image', 'sha256:test', '--state', str(self.root / 'state.json')]
        with patch('sys.argv', args), contextlib.redirect_stdout(io.StringIO()):
            build.main()

    def seal(self, output='BiteDJ 0.0.7-codex-build-test.1'):
        (self.root / 'state.json').write_text(json.dumps(build.source_state()))
        original = subprocess.check_output
        def run(args, **kwargs):
            if args[0] == 'docker':
                return output
            return original(args, **kwargs)
        with patch.object(build.subprocess, 'check_output', side_effect=run):
            self.call('seal')

    def test_seal_and_verify(self):
        self.seal()
        self.call('verify')
        manifest = json.loads((self.dist / 'build-provenance.json').read_text())
        self.assertEqual(manifest['builder_image'], 'sha256:test')
        self.assertEqual(manifest['source_branch'], 'codex/build-test')

    def test_source_edit_and_untracked_source_invalidate(self):
        self.seal()
        self.write('src/new.cpp', 'new')
        with self.assertRaisesRegex(ValueError, 'stale'):
            self.call('verify')
        (self.root / 'src/new.cpp').unlink()
        self.write('src/main.cpp', 'changed')
        with self.assertRaisesRegex(ValueError, 'stale'):
            self.call('verify')

    def test_asset_edit_or_extra_stale_file_invalidates(self):
        self.seal()
        self.write('dist-linux/share/mixxx/skins/deleted.xml', 'stale')
        with self.assertRaisesRegex(ValueError, 'contents'):
            self.call('verify')

    def test_wrong_architecture_rejected(self):
        self.seal()
        self.binary(62)
        with self.assertRaisesRegex(ValueError, 'architecture'):
            self.call('verify')

    def test_wrong_binary_version_not_published(self):
        with self.assertRaisesRegex(ValueError, 'expected version'):
            self.seal('BiteDJ 0.0.7-codex-build-test.10')
        self.assertFalse((self.dist / 'build-provenance.json').exists())

    def test_dirty_build_has_source_snapshot(self):
        self.write('src/main.cpp', 'changed')
        self.seal()
        self.assertTrue((self.dist / 'source-snapshot.tar.gz').exists())
        self.call('verify')

    def test_source_change_during_build_rejected(self):
        (self.root / 'state.json').write_text(json.dumps(build.source_state()))
        self.write('src/main.cpp', 'changed')
        with self.assertRaisesRegex(ValueError, 'during build'):
            self.call('seal')

    def test_branch_version_must_match(self):
        self.write('CMakeLists.txt', 'set(BITEDJ_VERSION "0.0.7")\nset(BITEDJ_VERSION_PRERELEASE "another-branch.1")')
        with self.assertRaisesRegex(ValueError, 'before building'):
            build.source_state()

    def test_shell_stages_install_and_preserves_previous_output_on_failure(self):
        repo = Path(__file__).resolve().parents[2]
        for name in ('scripts/build/docker-build.sh', 'scripts/build/build-support.py', 'scripts/build/generate-pi-image.sh', 'docker/build.Dockerfile', 'tools/debian_buildenv.sh'):
            dest = self.root / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(repo / name, dest)
        with (self.root / '.gitignore').open('a') as stream:
            stream.write('build-linux/\ntest-results/\ndist-linux-staging.*/\n.bitedj-build-lock/\nmixxx-pi-gen/\n')
        self.write('fake-bin/docker', r'''#!/usr/bin/env python3
import os, pathlib, sys
args = sys.argv[1:]
if args[:2] == ['image', 'inspect']:
    print('sha256:fixture')
elif args[0] == 'run' and args[-1] == '--version':
    print('BiteDJ ' + os.environ.get('FAKE_VERSION', '0.0.7-codex-build-test.1'))
elif args[0] == 'run':
    assert '-i' in args, 'Container must receive the build script on stdin'
    script = sys.stdin.read()
    assert 'cmake -G Ninja' in script
    assert '--no-tests=error' in script
    mount = next(args[i+1] for i,x in enumerate(args) if x == '-v' and args[i+1].endswith(':/src'))
    root = pathlib.Path(mount[:-5])
    stage = next(x.split('=', 1)[1] for x in args if x.startswith('BITEDJ_STAGE='))
    binary = root / stage.removeprefix('/src/') / 'bin/mixxx'
    binary.parent.mkdir(parents=True)
    binary.write_bytes(b'\x7fELF\x02\x01' + b'\0'*12 + (183).to_bytes(2, 'little'))
    binary.chmod(0o755)
    (root / 'build-linux/CMakeCache.txt').write_text('fixture')
''')
        (self.root / 'fake-bin/docker').chmod(0o755)
        self.git('add', '.')
        self.git('commit', '-qm', 'test: build fixture')
        env = dict(os.environ, PATH=str(self.root / 'fake-bin') + os.pathsep + os.environ['PATH'])
        command = ['bash', str(self.root / 'scripts/build/docker-build.sh'), '--platform', 'linux/arm64', '--jobs', '2']
        # Deliberately launch outside the repository.
        first = subprocess.run(command, cwd=self.temp.name, env=env, capture_output=True, text=True)
        self.assertEqual(first.returncode, 0, first.stdout + first.stderr)
        self.assertFalse((self.dist / 'share/mixxx/skins/test.xml').exists())
        self.write('mixxx-pi-gen/build-docker.sh', '#!/bin/sh\ntouch image-started\n')
        (self.root / 'mixxx-pi-gen/build-docker.sh').chmod(0o755)
        self.write('mixxx-pi-gen/config', 'IMG_NAME="bitedj-pi-v0.0.7-wrong.1"\n')
        image_command = ['bash', str(self.root / 'scripts/build/generate-pi-image.sh')]
        mismatch = subprocess.run(image_command, cwd='/', env=env, capture_output=True, text=True)
        self.assertNotEqual(mismatch.returncode, 0)
        self.assertIn('IMG_NAME', mismatch.stderr)
        self.assertFalse((self.root / 'mixxx-pi-gen/image-started').exists())
        self.write('mixxx-pi-gen/config', 'IMG_NAME="bitedj-pi-v0.0.7-codex-build-test.1"\n')
        image = subprocess.run(image_command, cwd='/', env=env, capture_output=True, text=True)
        self.assertEqual(image.returncode, 0, image.stdout + image.stderr)
        self.assertTrue((self.root / 'mixxx-pi-gen/image-started').exists())
        manifest = (self.dist / 'build-provenance.json').read_bytes()
        failed = subprocess.run(command, cwd='/', env=dict(env, FAKE_VERSION='wrong'), capture_output=True, text=True)
        self.assertNotEqual(failed.returncode, 0)
        self.assertEqual((self.dist / 'build-provenance.json').read_bytes(), manifest)
        self.assertFalse((self.root / '.bitedj-build-lock').exists())
        self.assertFalse(list(self.root.glob('dist-linux-staging.*')))
        (self.root / 'build-linux/.bitedj-builder').unlink()
        legacy = subprocess.run(command, cwd='/', env=env, capture_output=True, text=True)
        self.assertNotEqual(legacy.returncode, 0)
        self.assertIn('--clean', legacy.stderr)

    def test_memory_limits(self):
        gib = 1024**3
        self.assertEqual(build.workers(16, 64*gib, [str(7*gib)]), 2)
        self.assertEqual(build.workers(2, 64*gib, ['max']), 2)
        self.assertEqual(build.workers(16, gib), 1)
        self.assertEqual(build.workers(16, 64*gib, [str(12*gib), str(6*gib)]), 2)


if __name__ == '__main__':
    unittest.main()
