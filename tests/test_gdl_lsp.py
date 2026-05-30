#!/usr/bin/env python3
"""Test gdl_lsp server by sending JSON-RPC messages via stdin."""

import json
import os
import select
import subprocess
import sys
import time


def send_msg(proc, msg):
    """Send a JSON-RPC message via stdin with Content-Length framing."""
    body = json.dumps(msg)
    framed = f"Content-Length: {len(body)}\r\n\r\n{body}"
    proc.stdin.write(framed.encode())
    proc.stdin.flush()


def _read_raw(proc, count, timeout=5.0):
    """Read exactly `count` bytes from stdout using the raw fd (unbuffered)."""
    fd = proc.stdout.fileno()
    data = b""
    deadline = time.time() + timeout
    while len(data) < count and time.time() < deadline:
        r, _, _ = select.select([fd], [], [], max(0, deadline - time.time()))
        if not r:
            continue
        chunk = os.read(fd, count - len(data))
        if not chunk:
            break
        data += chunk
    return data


def recv_msg(proc, timeout=5.0):
    """Receive a single JSON-RPC response from stdout."""
    fd = proc.stdout.fileno()
    header = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        remaining = deadline - time.time()
        if remaining <= 0:
            break
        r, _, _ = select.select([fd], [], [], min(remaining, 1.0))
        if not r:
            continue
        byte = os.read(fd, 1)
        if not byte:
            break
        header += byte
        if header.endswith(b"\r\n\r\n"):
            break
    if not header.endswith(b"\r\n\r\n"):
        return None

    # Parse Content-Length
    for line in header.decode().split("\r\n"):
        if line.lower().startswith("content-length:"):
            length = int(line.split(":")[1].strip())
            break
    else:
        return None

    body = _read_raw(proc, length, timeout)
    if len(body) != length:
        return None
    return json.loads(body)


def main():
    proc = subprocess.Popen(
        ["./build/tools/gdl_lsp/gdl_lsp"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        # 1. Send initialize
        send_msg(proc, {
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {
                "processId": None,
                "capabilities": {},
            }
        })
        resp = recv_msg(proc)
        assert resp is not None, "No response to initialize"
        assert resp.get("id") == 1, f"Expected id=1, got {resp.get('id')}"
        caps = resp.get("result", {}).get("capabilities", {})
        assert "semanticTokensProvider" in caps, "Missing semanticTokensProvider"
        print(f"[PASS] initialize returned capabilities with semanticTokensProvider")

        # 2. Send initialized notification
        send_msg(proc, {
            "jsonrpc": "2.0",
            "method": "initialized",
            "params": {}
        })

        # 3. Open a document
        gdl_code = """digit = '0'-'9'
int = digit+
"""
        send_msg(proc, {
            "jsonrpc": "2.0",
            "method": "textDocument/didOpen",
            "params": {
                "textDocument": {
                    "uri": "file:///test.gdl",
                    "languageId": "gdl",
                    "version": 1,
                    "text": gdl_code,
                }
            }
        })

        # Wait for debounce (100ms) + processing
        time.sleep(0.3)

        # 4. Request semantic tokens
        send_msg(proc, {
            "jsonrpc": "2.0",
            "id": 2,
            "method": "textDocument/semanticTokens/full",
            "params": {
                "textDocument": {
                    "uri": "file:///test.gdl",
                }
            }
        })
        resp = recv_msg(proc)
        assert resp is not None, "No response to semanticTokens/full"
        assert resp.get("id") == 2, f"Expected id=2, got {resp.get('id')}"
        data = resp.get("result", {}).get("data", [])
        assert len(data) > 0, "Empty semantic tokens data"
        print(f"[PASS] semanticTokens/full returned {len(data)} integers ({len(data)//5} tokens)")

        # 5. Send shutdown
        send_msg(proc, {
            "jsonrpc": "2.0",
            "id": 3,
            "method": "shutdown",
            "params": {}
        })
        resp = recv_msg(proc)
        assert resp is not None, "No response to shutdown"
        assert resp.get("id") == 3
        print(f"[PASS] shutdown returned OK")

        # 6. Send exit
        send_msg(proc, {
            "jsonrpc": "2.0",
            "method": "exit",
            "params": {}
        })

        # Wait for process to exit
        proc.wait(timeout=2.0)
        print(f"[PASS] gdl_lsp exited cleanly")

    except Exception as e:
        proc.kill()
        stderr = proc.stderr.read().decode() if proc.stderr else ""
        print(f"[FAIL] {e}", file=sys.stderr)
        if stderr:
            print(f"stderr:\n{stderr}", file=sys.stderr)
        sys.exit(1)

    # Print any stderr output (debug)
    stderr = proc.stderr.read().decode()
    if stderr:
        print(f"\n[DEBUG] Server stderr output:\n{stderr}")

    print("\nAll tests passed!")


if __name__ == "__main__":
    main()
