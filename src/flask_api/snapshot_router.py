from flask import Blueprint, request, jsonify

from datetime import datetime
import socket, json

HOST = '127.0.0.1'
ADMIN_PORT = 9100

snapshot_bp = Blueprint('snapshot', __name__)

@snapshot_bp.route('/snapshot/<symbol>', methods=['GET'])
def get_snapshot(symbol):
    try:
        with socket.create_connection((HOST, ADMIN_PORT)) as sock:
            message = json.dumps({"type": "GET_SNAPSHOT", "symbol": symbol}) + '\n'
            sock.sendall(message.encode('utf-8'))
            response = json.loads(sock.recv(4096).decode('utf-8').strip())
            return jsonify(status="ok", response=response), 200
    except Exception as e:
        return jsonify(status="error", response=str(e)), 400

@snapshot_bp.route('/snapshot/export', methods=['POST'])
def export_snapshot():
    try:
        symbol = request.json.get("symbol")
        with socket.create_connection((HOST, ADMIN_PORT)) as sock:
            message = json.dumps({"type": "EXPORT_SNAPSHOT", "symbol": symbol}) + '\n'
            sock.sendall(message.encode('utf-8'))
            response = json.loads(sock.recv(1024).decode('utf-8').strip())
            return jsonify(status="ok", response=response), 200
    except Exception as e:
        return jsonify(status="error", response=str(e)), 400
