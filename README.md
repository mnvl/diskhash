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

## HTTP Server

An HTTP server provides network access to the hash map with sharding for concurrency.

### Building

```bash
cd build
cmake ..
make diskhash_server
```

### Running

```bash
./diskhash_server --port 8080 --db /path/to/db --shards 4 --threads 4
```

Options:
- `--port`, `-p`: Port to listen on (default: 8080)
- `--address`, `-a`: Address to bind to (default: 0.0.0.0)
- `--db`, `-d`: Path to database files (required)
- `--shards`, `-s`: Number of shards (default: 4)
- `--threads`, `-t`: Number of worker threads (default: number of CPU cores)

### API

| Method | Endpoint | Description | Response |
|--------|----------|-------------|----------|
| GET | `/get?key=<base64url>` | Get value | `200` + value, or `404` |
| PUT/POST | `/set?key=<base64url>` | Set value (body = value) | `200`, or `409` if exists |
| DELETE | `/delete?key=<base64url>` | Delete key | `200`, or `404` |
| GET | `/keys` | List all keys | `200` + newline-separated base64url keys |
| GET | `/health` | Health check | `200 OK` |

Keys are base64url-encoded in query parameters. Values are raw bytes in request/response bodies.

### Python Client

```python
from diskhash import DiskHashClient

client = DiskHashClient("localhost", 8080)

# Set/get
client.set(b"hello", b"world")  # Returns True on success, False if exists
client.get(b"hello")            # b"world", or None if not found

# Dict-like access
client[b"foo"] = b"bar"
print(client[b"foo"])           # b"bar"
del client[b"foo"]

# Check membership
b"hello" in client              # True

# List all keys
client.keys()                   # [b"hello", ...]

# Health check
client.health()                 # True

# Context manager
with DiskHashClient("localhost", 8080) as client:
    client[b"key"] = b"value"
```
