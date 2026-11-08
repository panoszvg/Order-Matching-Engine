from flask import Blueprint, request, jsonify
import socket, json

from lib import to_fix

HOST = '127.0.0.1'
FIX_PORT = 9002
ENGINE_TIMEOUT_SECONDS = 5

order_bp = Blueprint('orders', __name__)

REQUIRED_FIELDS = ["id", "security", "type", "price", "quantity"]


def _recv_line(sock):
    """Read one newline-terminated reply; recv() alone may return a partial message."""
    data = b""
    while not data.endswith(b"\n"):
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("Engine closed the connection without replying")
        data += chunk
    return data.decode('utf-8').strip()


@order_bp.route('/send-order', methods=['POST'])
def send_order():
    order = request.get_json(silent=True)
    if not order:
        return jsonify(status="error", message="Request body must be JSON"), 400

    missing = [field for field in REQUIRED_FIELDS if field not in order]
    if missing:
        return jsonify(status="error", message=f"Missing fields: {', '.join(missing)}"), 400

    try:
        message = to_fix(order)
    except Exception as e:
        return jsonify(status="error", message=str(e)), 400

    try:
        with socket.create_connection((HOST, FIX_PORT), timeout=ENGINE_TIMEOUT_SECONDS) as sock:
            sock.sendall(message.encode('utf-8'))
            print(f"[Port {FIX_PORT}] Sent:", message.strip())
            reply = json.loads(_recv_line(sock))
    except OSError as e:  # includes connection refused, timeouts and early disconnects
        return jsonify(status="error", message=str(e)), 502
    except (json.JSONDecodeError, UnicodeDecodeError) as e:
        return jsonify(status="error", message=f"Unreadable reply from engine: {e}"), 502

    if not isinstance(reply, dict):
        return jsonify(status="error", message="Unreadable reply from engine: expected a JSON object"), 502

    engine_message = reply.get("message", "")
    if reply.get("status") == "ok":
        return jsonify(status="ok", message=engine_message, received=order), 200
    # The engine rejected the order (unknown symbol, price off tick, invalid FIX, ...)
    return jsonify(status="error", message=engine_message), 400
