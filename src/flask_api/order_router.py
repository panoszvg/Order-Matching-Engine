from flask import Blueprint, request, jsonify
import socket, time

from lib import to_fix

HOST = '127.0.0.1'
FIX_PORT = 9002

order_bp = Blueprint('orders', __name__)

REQUIRED_FIELDS = ["id", "security", "type", "price", "quantity"]

@order_bp.route('/send-order', methods=['POST'])
def send_order():
    order = request.get_json(silent=True)
    if not order:
        return jsonify(status="error", message="Request body must be JSON"), 400

    missing = [field for field in REQUIRED_FIELDS if field not in order]
    if missing:
        return jsonify(status="error", message=f"Missing fields: {', '.join(missing)}"), 400

    try:
        with socket.create_connection((HOST, FIX_PORT)) as sock:
            message = to_fix(order)
            sock.sendall(message.encode('utf-8'))
            print(f"[Port {FIX_PORT}] Sent:", message.strip())
            print("Order received:", order)
            return jsonify(status="ok", received=order), 200
    except OSError as e:
        return jsonify(status="error", message=str(e)), 502
    except Exception as e:
        return jsonify(status="error", message=str(e)), 400
