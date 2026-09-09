#!/usr/bin/env python3
"""Standard-library Python entry point for BiteDJ's layered test suite."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "suite",
        choices=["fast", "native", "bitedj", "removable", "e2e", "all"],
    )
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument(
        "--jobs",
        type=int,
        default=1,
        help="Native CTest workers; keep 1 until shared fixtures are isolated",
    )
    parser.add_argument(
        "--fixture", choices=["stores", "samplers"], help=argparse.SUPPRESS
    )
    args = parser.parse_args()
    if args.fixture and args.suite != "removable":
        parser.error(
            "--fixture is reserved for the removable fixture launcher"
        )
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    os.chdir(ROOT)
    for suite in (
        ["fast", "native", "removable", "e2e"]
        if args.suite == "all"
        else [args.suite]
    ):
        if suite in ("native", "bitedj", "removable"):
            if suite == "removable" and not args.fixture:
                subprocess.run(
                    [
                        "bash",
                        str(ROOT / "scripts/test/run-removable-tests.sh"),
                        "--build-dir",
                        str(args.build_dir.resolve()),
                        "--jobs",
                        str(args.jobs),
                    ],
                    check=True,
                )
                continue
            build = args.build_dir.resolve()
            if args.fixture:
                if not os.path.ismount("/mnt"):
                    raise RuntimeError(
                        "Missing private removable mount fixture"
                    )
                if os.path.ismount("/mnt/usbtest") != (
                    args.fixture == "stores"
                ):
                    raise RuntimeError(
                        "Incorrect removable fixture: " + args.fixture
                    )
            selection = f"removable-{args.fixture}" if args.fixture else suite
            report = build / f"{selection}-results.xml"
            report.unlink(missing_ok=True)
            command = [
                "ctest",
                "--test-dir",
                str(build),
                "--no-tests=error",
                "--output-on-failure",
                "--timeout",
                "45",
                "--parallel",
                str(args.jobs),
                "--output-junit",
                str(report),
            ]
            if suite in ("bitedj", "removable"):
                command += ["-L", selection]
            if suite != "removable":
                command += ["-LE", "removable"]
            subprocess.run(command, check=True)
            if suite == "removable" and ET.parse(report).findall(".//skipped"):
                raise RuntimeError(
                    "Removable-drive tests skipped despite mounted fixtures; see "
                    + str(report)
                )
        else:
            if suite == "fast":
                subprocess.run(
                    ["node", str(ROOT / "tests/controllers/test_ddj400_mappings.cjs")],
                    check=True, timeout=30,
                )
                subprocess.run(
                    ["node", str(ROOT / "tests/padfx/test_padfx.cjs")],
                    check=True,
                    timeout=30,
                )
                subprocess.run(
                    ["node", str(ROOT / "tests/padfx/test_controller_pad_display.cjs")],
                    check=True, timeout=30,
                )
                subprocess.run(
                    [sys.executable, str(ROOT / "tests/novnc/test_repair.py")],
                    check=True, timeout=30,
                )
                subprocess.run(
                    ["node", str(ROOT / "tests/controllers/test_ddj400_effect_select.cjs")],
                    check=True, timeout=30,
                )
                subprocess.run(
                    [sys.executable, str(ROOT / "tests/effects/test_beatfx_catalog.py")],
                    check=True, timeout=30,
                )
            tests = unittest.defaultTestLoader.discover(
                str(ROOT / "tests" / suite)
            )
            if not tests.countTestCases():
                raise RuntimeError(f"No tests discovered for {suite}")
            if (
                not unittest.TextTestRunner(verbosity=2)
                .run(tests)
                .wasSuccessful()
            ):
                return 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"Test run failed: {error}", file=sys.stderr)
        sys.exit(1)
