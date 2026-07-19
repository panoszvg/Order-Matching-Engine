import json
from unittest.mock import MagicMock, patch


SNAPSHOT_RESPONSE = {"symbol": "BTC", "bids": [{"price": 100.0, "quantity": 5.0}], "asks": []}


def _mock_socket(mock_create_conn, recv_payload: dict):
    """Attach a mock socket that returns recv_payload on recv()."""
    mock_sock = MagicMock()
    mock_sock.recv.return_value = json.dumps(recv_payload).encode("utf-8")
    mock_create_conn.return_value.__enter__.return_value = mock_sock
    mock_create_conn.return_value.__exit__.return_value = False
    return mock_sock


# ── GET /api/snapshot/<symbol> ────────────────────────────────────────────────

@patch("snapshot_router.socket.create_connection")
def test_get_snapshot_returns_200(mock_conn, client):
    _mock_socket(mock_conn, SNAPSHOT_RESPONSE)
    resp = client.get("/api/snapshot/BTC")
    assert resp.status_code == 200


@patch("snapshot_router.socket.create_connection")
def test_get_snapshot_status_is_ok(mock_conn, client):
    _mock_socket(mock_conn, SNAPSHOT_RESPONSE)
    resp = client.get("/api/snapshot/BTC")
    assert resp.get_json()["status"] == "ok"


@patch("snapshot_router.socket.create_connection")
def test_get_snapshot_response_contains_symbol(mock_conn, client):
    _mock_socket(mock_conn, SNAPSHOT_RESPONSE)
    resp = client.get("/api/snapshot/BTC")
    data = resp.get_json()
    assert data["response"]["symbol"] == "BTC"


@patch("snapshot_router.socket.create_connection")
def test_get_snapshot_sends_correct_command(mock_conn, client):
    mock_sock = _mock_socket(mock_conn, SNAPSHOT_RESPONSE)
    client.get("/api/snapshot/ETH")
    sent = mock_sock.sendall.call_args[0][0].decode("utf-8")
    payload = json.loads(sent.strip())
    assert payload["type"] == "GET_SNAPSHOT"
    assert payload["symbol"] == "ETH"


@patch("snapshot_router.socket.create_connection")
def test_get_snapshot_connection_failure_returns_502(mock_conn, client):
    mock_conn.side_effect = ConnectionRefusedError("engine not running")
    resp = client.get("/api/snapshot/BTC")
    assert resp.status_code == 502


@patch("snapshot_router.socket.create_connection")
def test_get_snapshot_malformed_response_returns_400(mock_conn, client):
    mock_sock = MagicMock()
    mock_sock.recv.return_value = b"not valid json"
    mock_conn.return_value.__enter__.return_value = mock_sock
    mock_conn.return_value.__exit__.return_value = False
    resp = client.get("/api/snapshot/BTC")
    assert resp.status_code == 400


# ── POST /api/snapshot/export ─────────────────────────────────────────────────

EXPORT_RESPONSE = {"status": "exported", "file": "snapshots/book_BTC_123.json"}


@patch("snapshot_router.socket.create_connection")
def test_export_snapshot_returns_200(mock_conn, client):
    _mock_socket(mock_conn, EXPORT_RESPONSE)
    resp = client.post("/api/snapshot/export", json={"symbol": "BTC"})
    assert resp.status_code == 200


@patch("snapshot_router.socket.create_connection")
def test_export_snapshot_status_is_ok(mock_conn, client):
    _mock_socket(mock_conn, EXPORT_RESPONSE)
    resp = client.post("/api/snapshot/export", json={"symbol": "BTC"})
    assert resp.get_json()["status"] == "ok"


@patch("snapshot_router.socket.create_connection")
def test_export_snapshot_sends_correct_command(mock_conn, client):
    mock_sock = _mock_socket(mock_conn, EXPORT_RESPONSE)
    client.post("/api/snapshot/export", json={"symbol": "ETH"})
    sent = mock_sock.sendall.call_args[0][0].decode("utf-8")
    payload = json.loads(sent.strip())
    assert payload["type"] == "EXPORT_SNAPSHOT"
    assert payload["symbol"] == "ETH"


@patch("snapshot_router.socket.create_connection")
def test_export_snapshot_connection_failure_returns_502(mock_conn, client):
    mock_conn.side_effect = ConnectionRefusedError("engine not running")
    resp = client.post("/api/snapshot/export", json={"symbol": "BTC"})
    assert resp.status_code == 502
