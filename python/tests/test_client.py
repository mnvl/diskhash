"""Tests for DiskHashClient HTTP client."""

import os
import shutil
import signal
import socket
import subprocess
import tempfile
import time
import uuid

import pytest

from diskhash import DiskHashClient


def find_free_port():
    """Find a free port to use for the test server."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def get_server_path():
    """Get path to the diskhash_server binary."""
    # Check PATH first (covers pip-installed binary)
    on_path = shutil.which("diskhash_server")
    if on_path:
        return on_path
    # Fall back to common build directories
    candidates = [
        os.path.join(os.path.dirname(__file__), "../../build/diskhash_server"),
        os.path.join(os.path.dirname(__file__), "../../../build/diskhash_server"),
        "build/diskhash_server",
        "./diskhash_server",
    ]
    for path in candidates:
        abs_path = os.path.abspath(path)
        if os.path.isfile(abs_path) and os.access(abs_path, os.X_OK):
            return abs_path
    pytest.skip("diskhash_server binary not found. Install with: pip install .")


# Module-scoped: one server for all tests in this file
@pytest.fixture(scope="module")
def server():
    """Start a test server and yield the client, then clean up."""
    server_path = get_server_path()
    port = find_free_port()
    tmpdir = tempfile.mkdtemp()
    db_path = os.path.join(tmpdir, "testdb")

    proc = subprocess.Popen(
        [
            server_path,
            "--port", str(port),
            "--db", db_path,
            "--shards", "2",
            "--threads", "2",
            "--address", "127.0.0.1",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    client = DiskHashClient("127.0.0.1", port, timeout=5.0)
    for _ in range(50):
        if client.health():
            break
        time.sleep(0.1)
    else:
        proc.kill()
        stdout, stderr = proc.communicate()
        pytest.fail(f"Server failed to start.\nstdout: {stdout}\nstderr: {stderr}")

    try:
        yield client
    finally:
        proc.send_signal(signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        client.close()


def unique_key(prefix: str = "k") -> bytes:
    """Generate a unique key to avoid cross-test conflicts."""
    return f"{prefix}_{uuid.uuid4().hex[:8]}".encode()


class TestDiskHashClient:
    """Tests for DiskHashClient HTTP client."""

    def test_health(self, server):
        assert server.health() is True

    def test_set_and_get(self, server):
        key = unique_key()
        assert server.set(key, b"world") is True
        assert server.get(key) == b"world"

    def test_get_missing(self, server):
        assert server.get(unique_key("miss")) is None

    def test_set_duplicate_returns_false(self, server):
        key = unique_key()
        assert server.set(key, b"value1") is True
        assert server.set(key, b"value2") is False
        assert server.get(key) == b"value1"

    def test_delete(self, server):
        key = unique_key()
        server.set(key, b"value")
        assert server.delete(key) is True
        assert server.get(key) is None

    def test_delete_missing(self, server):
        assert server.delete(unique_key("miss")) is False

    def test_keys(self, server):
        keys_to_add = [unique_key("lst") for _ in range(3)]
        for k in keys_to_add:
            server.set(k, b"v")

        all_keys = server.keys()
        for k in keys_to_add:
            assert k in all_keys

    def test_dict_getitem(self, server):
        key = unique_key()
        server.set(key, b"value")
        assert server[key] == b"value"

    def test_dict_getitem_missing_raises(self, server):
        with pytest.raises(KeyError):
            _ = server[unique_key("miss")]

    def test_dict_setitem(self, server):
        key = unique_key()
        server[key] = b"value"
        assert server.get(key) == b"value"

    def test_dict_setitem_duplicate_raises(self, server):
        key = unique_key()
        server[key] = b"value1"
        with pytest.raises(ValueError):
            server[key] = b"value2"

    def test_dict_delitem(self, server):
        key = unique_key()
        server[key] = b"value"
        del server[key]
        assert server.get(key) is None

    def test_dict_delitem_missing_raises(self, server):
        with pytest.raises(KeyError):
            del server[unique_key("miss")]

    def test_contains(self, server):
        key = unique_key()
        server[key] = b"yes"
        assert key in server
        assert unique_key("miss") not in server

    def test_len_increases(self, server):
        before = len(server)
        server[unique_key()] = b"v"
        assert len(server) == before + 1
        server[unique_key()] = b"v"
        assert len(server) == before + 2

    def test_iter(self, server):
        keys_to_add = {unique_key("it") for _ in range(3)}
        for k in keys_to_add:
            server[k] = b"v"

        all_keys = set(server)
        for k in keys_to_add:
            assert k in all_keys

    def test_binary_keys_and_values(self, server):
        key = b"\x00\x01\x02" + unique_key("bin")
        value = b"\xff\xfe\x00\xfd"
        server.set(key, value)
        assert server.get(key) == value

    def test_large_value(self, server):
        key = unique_key("lg")
        value = b"x" * 100
        server.set(key, value)
        assert server.get(key) == value

    def test_many_keys(self, server):
        prefix = uuid.uuid4().hex[:6]
        for i in range(100):
            server.set(f"{prefix}_{i}".encode(), f"v{i}".encode())

        for i in range(100):
            assert server.get(f"{prefix}_{i}".encode()) == f"v{i}".encode()

    def test_context_manager(self):
        client = DiskHashClient("127.0.0.1", 1)
        with client:
            pass
