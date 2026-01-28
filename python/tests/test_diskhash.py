"""Tests for DiskHash Python bindings."""

import os
import tempfile

import pytest

from diskhash import DiskHash


@pytest.fixture
def temp_db():
    """Create a temporary database path and clean up after test."""
    with tempfile.TemporaryDirectory() as tmpdir:
        yield os.path.join(tmpdir, "testdb")


class TestDiskHash:
    """Tests for DiskHash native bindings."""

    def test_set_and_get(self, temp_db):
        """Test basic set and get operations."""
        with DiskHash(temp_db) as db:
            db[b"hello"] = b"world"
            assert db[b"hello"] == b"world"

    def test_get_method(self, temp_db):
        """Test .get() method with default value."""
        with DiskHash(temp_db) as db:
            db[b"key"] = b"value"
            assert db.get(b"key") == b"value"
            assert db.get(b"missing") is None
            assert db.get(b"missing", b"default") == b"default"

    def test_contains(self, temp_db):
        """Test membership testing."""
        with DiskHash(temp_db) as db:
            db[b"exists"] = b"yes"
            assert b"exists" in db
            assert b"missing" not in db

    def test_multiple_keys(self, temp_db):
        """Test storing multiple key-value pairs."""
        with DiskHash(temp_db) as db:
            db[b"key1"] = b"value1"
            db[b"key2"] = b"value2"
            db[b"key3"] = b"value3"

            assert db[b"key1"] == b"value1"
            assert db[b"key2"] == b"value2"
            assert db[b"key3"] == b"value3"

    def test_duplicate_key_raises(self, temp_db):
        """Test that inserting duplicate key raises KeyError."""
        with DiskHash(temp_db) as db:
            db[b"key"] = b"value1"
            with pytest.raises(KeyError):
                db[b"key"] = b"value2"

    def test_missing_key_raises(self, temp_db):
        """Test that accessing missing key raises KeyError."""
        with DiskHash(temp_db) as db:
            with pytest.raises(KeyError):
                _ = db[b"nonexistent"]

    def test_persistence(self, temp_db):
        """Test that data persists across reopening."""
        with DiskHash(temp_db) as db:
            db[b"persistent"] = b"data"

        with DiskHash(temp_db) as db:
            assert db[b"persistent"] == b"data"

    def test_read_only_mode(self, temp_db):
        """Test read-only mode."""
        with DiskHash(temp_db) as db:
            db[b"key"] = b"value"

        with DiskHash(temp_db, read_only=True) as db:
            assert db[b"key"] == b"value"

    def test_bytes_allocated(self, temp_db):
        """Test bytes_allocated returns positive value."""
        with DiskHash(temp_db) as db:
            db[b"key"] = b"value"
            assert db.bytes_allocated() > 0

    def test_binary_data(self, temp_db):
        """Test storing binary data with null bytes."""
        with DiskHash(temp_db) as db:
            key = b"\x00\x01\x02\x03"
            value = b"\xff\xfe\xfd\x00\x01"
            db[key] = value
            assert db[key] == value

    def test_large_values(self, temp_db):
        """Test storing values up to bucket capacity."""
        with DiskHash(temp_db) as db:
            key = b"large"
            value = b"x" * 100
            db[key] = value
            assert db[key] == value

    def test_many_keys(self, temp_db):
        """Test storing many keys."""
        with DiskHash(temp_db) as db:
            for i in range(1000):
                key = f"key{i}".encode()
                value = f"value{i}".encode()
                db[key] = value

            for i in range(1000):
                key = f"key{i}".encode()
                value = f"value{i}".encode()
                assert db[key] == value
