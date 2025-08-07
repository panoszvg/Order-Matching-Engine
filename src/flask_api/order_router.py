from flask import Blueprint, request, jsonify
import socket, time

from lib import to_fix

HOST = '127.0.0.1'
FIX_PORT = 9002

order_bp = Blueprint('orders', __name__)

@order_bp.route('/send-order', methods=['POST'])
def send_order():
    try:
        order = request.get_json()
        with socket.create_connection((HOST, FIX_PORT)) as sock:
            message = to_fix(order)
            sock.sendall(message.encode('utf-8'))
            print(f"[Port {FIX_PORT}] Sent:", message.strip())
            print("Order received:", order)
            return jsonify(status="ok", received=order), 200
    except Exception as e:
        return jsonify(status="error", message=str(e)), 400
