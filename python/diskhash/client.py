"""HTTP client for diskhash server."""

import base64
from typing import Iterator

import requests


class DiskHashClient:
    """Client for the diskhash HTTP server.

    Keys and values are binary (bytes). Keys are base64url-encoded in URLs,
    values are sent as raw bytes in request/response bodies.

    Example:
        client = DiskHashClient("localhost", 8080)
        client[b"foo"] = b"bar"
        print(client[b"foo"])  # b"bar"
        del client[b"foo"]
    """

    def __init__(self, host: str = "localhost", port: int = 8080,
                 timeout: float = 30.0):
        """Initialize client.

        Args:
            host: Server hostname or IP address.
            port: Server port number.
            timeout: Request timeout in seconds.
        """
        self.base_url = f"http://{host}:{port}"
        self.timeout = timeout
        self._session = requests.Session()

    def _encode_key(self, key: bytes) -> str:
        """Encode binary key as base64url for URL parameter."""
        return base64.urlsafe_b64encode(key).decode("ascii").rstrip("=")

    def _decode_key(self, encoded: str) -> bytes:
        """Decode base64url key back to binary."""
        # Add padding if needed
        padding = 4 - (len(encoded) % 4)
        if padding != 4:
            encoded += "=" * padding
        return base64.urlsafe_b64decode(encoded)

    def get(self, key: bytes) -> bytes | None:
        """Get value for key.

        Args:
            key: The key to look up.

        Returns:
            The value if found, None otherwise.
        """
        url = f"{self.base_url}/get"
        params = {"key": self._encode_key(key)}
        resp = self._session.get(url, params=params, timeout=self.timeout)

        if resp.status_code == 200:
            return resp.content
        elif resp.status_code == 404:
            return None
        else:
            resp.raise_for_status()
            return None

    def set(self, key: bytes, value: bytes) -> bool:
        """Set key to value.

        Args:
            key: The key to set.
            value: The value to store.

        Returns:
            True if the key was set, False if it already exists.
        """
        url = f"{self.base_url}/set"
        params = {"key": self._encode_key(key)}
        resp = self._session.put(
            url, params=params, data=value,
            headers={"Content-Type": "application/octet-stream"},
            timeout=self.timeout
        )

        if resp.status_code == 200:
            return True
        elif resp.status_code == 409:
            return False
        else:
            resp.raise_for_status()
            return False

    def delete(self, key: bytes) -> bool:
        """Delete key.

        Args:
            key: The key to delete.

        Returns:
            True if the key was deleted, False if not found.
        """
        url = f"{self.base_url}/delete"
        params = {"key": self._encode_key(key)}
        resp = self._session.delete(url, params=params, timeout=self.timeout)

        if resp.status_code == 200:
            return True
        elif resp.status_code == 404:
            return False
        else:
            resp.raise_for_status()
            return False

    def keys(self) -> list[bytes]:
        """Return all keys in the database.

        Returns:
            List of all keys.
        """
        url = f"{self.base_url}/keys"
        resp = self._session.get(url, timeout=self.timeout)
        resp.raise_for_status()

        result = []
        for line in resp.text.strip().split("\n"):
            if line:
                result.append(self._decode_key(line))
        return result

    def health(self) -> bool:
        """Check if server is healthy.

        Returns:
            True if server responds OK, False otherwise.
        """
        try:
            url = f"{self.base_url}/health"
            resp = self._session.get(url, timeout=self.timeout)
            return resp.status_code == 200
        except requests.RequestException:
            return False

    def __getitem__(self, key: bytes) -> bytes:
        """Dict-like access: client[key].

        Args:
            key: The key to look up.

        Returns:
            The value.

        Raises:
            KeyError: If key not found.
        """
        result = self.get(key)
        if result is None:
            raise KeyError(key)
        return result

    def __setitem__(self, key: bytes, value: bytes) -> None:
        """Dict-like assignment: client[key] = value.

        Note: This will raise an error if the key already exists.
        Use set() method if you want to check the return value.

        Args:
            key: The key to set.
            value: The value to store.

        Raises:
            ValueError: If key already exists.
        """
        if not self.set(key, value):
            raise ValueError(f"Key already exists: {key!r}")

    def __delitem__(self, key: bytes) -> None:
        """Dict-like deletion: del client[key].

        Args:
            key: The key to delete.

        Raises:
            KeyError: If key not found.
        """
        if not self.delete(key):
            raise KeyError(key)

    def __contains__(self, key: bytes) -> bool:
        """Check if key exists: key in client.

        Args:
            key: The key to check.

        Returns:
            True if key exists, False otherwise.
        """
        return self.get(key) is not None

    def __iter__(self) -> Iterator[bytes]:
        """Iterate over keys: for key in client.

        Returns:
            Iterator over keys.
        """
        return iter(self.keys())

    def __len__(self) -> int:
        """Return number of keys: len(client).

        Returns:
            Number of keys in database.
        """
        return len(self.keys())

    def close(self) -> None:
        """Close the HTTP session."""
        self._session.close()

    def __enter__(self) -> "DiskHashClient":
        """Context manager entry."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """Context manager exit."""
        self.close()
