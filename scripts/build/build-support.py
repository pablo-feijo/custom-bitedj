#!/usr/bin/env python3
"""Local build provenance and resource limits (Python standard library only)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tarfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]


def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args]).decode().strip()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def builder_key(details):
    # OCI index IDs may change solely because BuildKit refreshes attestations.
    # Runtime configuration and content-addressed rootfs layers define the toolchain.
    stable = {key: details[key] for key in ('Architecture', 'Os', 'RootFS', 'Config')}
    return 'content-' + hashlib.sha256(json.dumps(stable, sort_keys=True).encode()).hexdigest()


def source_state():
    paths = subprocess.check_output(['git', '-C', str(ROOT), 'ls-files', '-z', '--cached', '--others', '--exclude-standard']).decode().split('\0')
    entries = {}
    for name in sorted(set(paths) - {''}):
        path = ROOT / name
        if path.is_symlink():
            entries[name] = ['link', os.readlink(path)]
        elif path.is_file():
            entries[name] = [path.stat().st_mode & 0o777, digest(path)]
        elif not path.exists():
            entries[name] = ['deleted']
        # Submodule source is not compiled into the application.
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    core = re.search(r'set\(BITEDJ_VERSION "([^"]+)"\)', cmake)[1]
    pre = re.search(r'set\(BITEDJ_VERSION_PRERELEASE "([^"]*)"\)', cmake)[1]
    version = core + ('-' + pre if pre else '')
    if not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*)?', version):
        raise ValueError('Invalid BiteDJ SemVer: ' + version)
    if any(part.isdigit() and len(part) > 1 and part.startswith('0') for part in pre.split('.')):
        raise ValueError('Numeric SemVer prerelease identifiers cannot have leading zeros')
    branch = git('branch', '--show-current')
    slug = re.sub('[^a-z0-9]+', '-', branch.lower()).strip('-')
    if slug.isdigit():
        slug = 'branch-' + slug
    final_release = (not pre and branch == 'codex/v' + core
                     and os.environ.get('BITEDJ_RELEASE_BUILD') == '1')
    if branch.startswith('codex/') and not final_release and not re.fullmatch(re.escape(slug) + r'\.[1-9]\d*', pre):
        raise ValueError('Set BITEDJ_VERSION_PRERELEASE to ' + slug + '.<build-number> before building')
    return dict(source_commit=git('rev-parse', 'HEAD'), source_branch=branch,
                source_hash=hashlib.sha256(json.dumps(entries, sort_keys=True).encode()).hexdigest(),
                version=version, dirty=bool(git('status', '--porcelain')), entries=entries)


def tree_hash(directory):
    entries = {}
    for path in sorted(directory.rglob('*')):
        if path.name == 'build-provenance.json' and path.parent == directory:
            continue
        if path.is_symlink():
            entries[str(path.relative_to(directory))] = ['link', os.readlink(path)]
        elif path.is_file():
            entries[str(path.relative_to(directory))] = [path.stat().st_mode & 0o777, digest(path)]
    return hashlib.sha256(json.dumps(entries, sort_keys=True).encode()).hexdigest()


def architecture(binary):
    with binary.open('rb') as stream:
        header = stream.read(20)
    if len(header) != 20 or header[:4] != b'\x7fELF' or header[4] != 2 or header[5] not in (1, 2):
        raise ValueError('Expected a 64-bit ELF binary')
    machine = int.from_bytes(header[18:20], 'little' if header[5] == 1 else 'big')
    return {183: 'linux/arm64', 62: 'linux/amd64'}.get(machine, 'unsupported')


def workers(cpu_count, memory_bytes, limits=()):
    available = min([memory_bytes] + [int(x) for x in limits if str(x).isdigit() and int(x) > 0])
    return max(1, min(cpu_count, available // (3 * 1024**3)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['state', 'seal', 'verify', 'jobs', 'builder-key'])
    parser.add_argument('--directory', type=Path, default=ROOT / 'dist-linux')
    parser.add_argument('--platform', default='linux/arm64')
    parser.add_argument('--image')
    parser.add_argument('--state', type=Path)
    args = parser.parse_args()
    if args.action == 'builder-key':
        details = json.loads(subprocess.check_output(['docker', 'image', 'inspect', args.image]))[0]
        print(builder_key(details))
        return
    if args.action == 'jobs':
        mem = int(re.search(r'MemTotal:\s+(\d+)', Path('/proc/meminfo').read_text())[1]) * 1024
        limits = [p.read_text().strip() for p in (Path('/sys/fs/cgroup/memory.max'), Path('/sys/fs/cgroup/memory/memory.limit_in_bytes')) if p.exists()]
        cpus = int(subprocess.check_output(['nproc']))
        print(workers(cpus, mem, limits))
        return
    state = source_state()
    if args.action == 'state':
        print(json.dumps(state, sort_keys=True))
        return
    if architecture(args.directory / 'bin/mixxx') != args.platform:
        raise ValueError('Artifact architecture does not match ' + args.platform)
    manifest = args.directory / 'build-provenance.json'
    if args.action == 'verify':
        old = json.loads(manifest.read_text())
        for field in ('source_hash', 'source_commit', 'source_branch', 'version'):
            if old[field] != state[field]:
                raise ValueError('Artifact is stale: ' + field)
        if old['platform'] != args.platform or old['artifact_hash'] != tree_hash(args.directory):
            raise ValueError('Artifact contents or platform changed')
        print('Verified ' + state['version'] + ' (' + args.platform + ')')
        return
    before = json.loads(args.state.read_text())
    if before != state:
        raise ValueError('Source changed during build; rerun the build')
    output = subprocess.check_output(['docker', 'run', '--rm', '--platform', args.platform,
        '-e', 'QT_QPA_PLATFORM=offscreen', '-v', str(args.directory.resolve()) + ':/artifact:ro',
        args.image, '/artifact/bin/mixxx', '--version'], text=True)
    if not re.search(r'(?<![\w.+-])' + re.escape(state['version']) + r'(?![\w.+-])', output):
        raise ValueError('Built binary did not report expected version: ' + output)
    if state['dirty']:
        with tarfile.open(args.directory / 'source-snapshot.tar.gz', 'w:gz') as archive:
            for name in state['entries']:
                path = ROOT / name
                if path.is_file() or path.is_symlink():
                    archive.add(path, arcname=name, recursive=False)
    state.pop('entries')
    state.update(platform=args.platform, builder_image=args.image, binary_version=output.strip(),
                 built_at=datetime.now(timezone.utc).isoformat(), artifact_hash=tree_hash(args.directory))
    manifest.write_text(json.dumps(state, indent=2) + '\n')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit('Build validation failed: ' + str(error))
