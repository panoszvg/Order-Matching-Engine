import json
from unittest.mock import MagicMock, patch


VALID_ORDER = {
    "type": "BUY",
    "security": "BTC",
    "price": 100.0,
    "quantity": 5.0,
    "id": "order-001",
}


def _mock_socket(mock_create_conn):
    """Attach a mock socket to the create_connection context-manager."""
    mock_sock = MagicMock()
    mock_create_conn.return_value.__enter__.return_value = mock_sock
    mock_create_conn.return_value.__exit__.return_value = False
    return mock_sock


# ── Success cases ─────────────────────────────────────────────────────────────

@patch("order_router.socket.create_connection")
def test_send_buy_order_returns_200(mock_conn, client):
    _mock_socket(mock_conn)
    resp = client.post("/api/send-order", json=VALID_ORDER)
    assert resp.status_code == 200


@patch("order_router.socket.create_connection")
def test_send_sell_order_returns_200(mock_conn, client):
    _mock_socket(mock_conn)
    order = {**VALID_ORDER, "type": "SELL"}
    resp = client.post("/api/send-order", json=order)
    assert resp.status_code == 200


@patch("order_router.socket.create_connection")
def test_response_status_is_ok(mock_conn, client):
    _mock_socket(mock_conn)
    resp = client.post("/api/send-order", json=VALID_ORDER)
    data = resp.get_json()
    assert data["status"] == "ok"


@patch("order_router.socket.create_connection")
def test_response_echoes_received_order(mock_conn, client):
    _mock_socket(mock_conn)
    resp = client.post("/api/send-order", json=VALID_ORDER)
    data = resp.get_json()
    assert data["received"]["security"] == "BTC"
    assert data["received"]["type"] == "BUY"


@patch("order_router.socket.create_connection")
def test_fix_message_sent_to_socket(mock_conn, client):
    mock_sock = _mock_socket(mock_conn)
    client.post("/api/send-order", json=VALID_ORDER)
    assert mock_sock.sendall.called
    sent_bytes = mock_sock.sendall.call_args[0][0]
    sent_str = sent_bytes.decode("utf-8")
    assert "55=BTC" in sent_str
    assert "35=D" in sent_str


# ── Error cases ───────────────────────────────────────────────────────────────

@patch("order_router.socket.create_connection")
def test_socket_connection_failure_returns_502(mock_conn, client):
    mock_conn.side_effect = ConnectionRefusedError("engine not running")
    resp = client.post("/api/send-order", json=VALID_ORDER)
    assert resp.status_code == 502


@patch("order_router.socket.create_connection")
def test_socket_failure_response_status_is_error(mock_conn, client):
    mock_conn.side_effect = OSError("network error")
    resp = client.post("/api/send-order", json=VALID_ORDER)
    data = resp.get_json()
    assert data["status"] == "error"
    assert "message" in data


def test_no_json_body_returns_400(client):
    resp = client.post("/api/send-order", data="not json", content_type="text/plain")
    # to_fix will raise a TypeError/KeyError on a non-dict payload
    assert resp.status_code == 400
