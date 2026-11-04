import subprocess
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="session")
def regression_binary(tmp_path_factory):
    binary = tmp_path_factory.mktemp("regression") / "regression"
    sources = sorted(path for path in (ROOT / "src").glob("*.c") if path.name != "main.c")
    subprocess.run(
        [
            "cc", "-Wall", "-Wextra", "-O2", "-fno-strict-aliasing",
            "-I", str(ROOT / "src"), str(ROOT / "test" / "regression.c"),
            *(str(path) for path in sources),
            "-pthread", "-Wl,--wrap=recv", "-o", str(binary),
        ],
        check=True,
    )
    return binary


def run_regression(binary, *arguments):
    result = subprocess.run(
        [str(binary), *arguments], capture_output=True, text=True, check=True, timeout=10
    )
    return result.stdout.splitlines()[-1]


@pytest.mark.parametrize("receiver", ["select", "epoll"])
def test_receive_error_is_not_counted_as_data(regression_binary, receiver):
    assert run_regression(regression_binary, f"receive-{receiver}") == "RESULT -1108 0"


@pytest.mark.parametrize(
    "contents,section,key,expected",
    [
        ("Tcp: OutSegs RetransSegs\nTcp: 100 17\n", "Tcp", "RetransSegs", 17),
        ("Tcp: RetransSegs OutSegs\nTcp: 17 100\n", "Tcp", "RetransSegs", 17),
        ("TcpExt: RetransSegs Other\nTcpExt: 99 0\n"
         "Tcp: RetransSegs OutSegs\nTcp: 17 100\n", "Tcp", "RetransSegs", 17),
        ("Tcp: TCPSynRetrans Other\nTcp: 99 0\n"
         "TcpExt: TCPSynRetrans Other\nTcpExt: 23 0\n", "TcpExt", "TCPSynRetrans", 23),
        ("Tcp: Other\nTcp: 17\nIp: Other\nIp: 99\n", "Tcp", "Missing", 0),
        ("Tcp: RetransSegs\nTcp: 18446744073709551614\n", "Tcp", "RetransSegs",
         18446744073709551614),
    ],
)
def test_proc_counter_columns(regression_binary, tmp_path, contents, section, key, expected):
    counters = tmp_path / "counters"
    counters.write_text(contents)
    assert run_regression(
        regression_binary, "counter", str(counters), section, key
    ) == f"RESULT {expected}"


@pytest.mark.parametrize(
    "options",
    [
        ["-p", "0"],
        ["-p", "-1"],
        ["-p", "65536"],
        ["-p", "65535", "-P", "2"],
        ["-p", "1"],
        ["-f", "65536"],
        ["-f", "65535", "-P", "1", "-n", "1", "-l", "2"],
        ["-f", "25001", "-P", "512", "-n", "25600", "-l", "1000"],
    ],
)
def test_invalid_port_ranges(regression_binary, options):
    assert run_regression(
        regression_binary, "args", "-s127.0.0.1", *options
    ) == "RESULT -1001"


@pytest.mark.parametrize(
    "options",
    [
        ["-p", "65535", "-P", "1"],
        ["-p", "65534", "-P", "2"],
        ["-p", "1", "-N"],
        ["-f", "65535", "-P", "1", "-n", "1", "-l", "1"],
        ["-f", "65534", "-P", "1", "-n", "1", "-l", "2"],
        ["-P", "512", "-n", "25600", "-l", "1000"],
    ],
)
def test_valid_port_boundaries(regression_binary, options):
    assert run_regression(
        regression_binary, "args", "-s127.0.0.1", *options
    ) == "RESULT 0"
