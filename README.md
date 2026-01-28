# diskhash

A disk-based hash table library using memory-mapped files and extensible hashing. Data is persisted across process restarts in two files per hash map (`*.cat` for the directory, `*.dat` for bucket storage).

## Installation

```bash
pip install git+https://github.com/mnvl/diskhash.git
```

## Usage

```python
from diskhash import DiskHash

# Open or create a hash map (creates mydb.cat and mydb.dat)
db = DiskHash("mydb")

# Insert key-value pairs (both must be bytes)
db[b"hello"] = b"world"
db[b"foo"] = b"bar"

# Lookup
db[b"hello"]          # b"world"

# .get() with optional default (returns None if missing)
db.get(b"hello")      # b"world"
db.get(b"missing")    # None
db.get(b"missing", b"default")  # b"default"

# Membership test
b"hello" in db        # True
b"missing" in db      # False

# Bytes allocated on disk
db.bytes_allocated()

# Close when done
db.close()
```

Use as a context manager:

```python
with DiskHash("mydb") as db:
    db[b"key"] = b"value"
    print(db[b"key"])
```

Reopen an existing hash map in read-only mode:

```python
with DiskHash("mydb", read_only=True) as db:
    print(db[b"key"])
```

Note: inserting a duplicate key raises `KeyError`. Records are packed inline with variable-length encoding, so in-place update of existing keys is not supported.
