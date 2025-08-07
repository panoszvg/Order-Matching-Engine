import socket
import json
import sys

HOST = '127.0.0.1'
ADMIN_PORT = 9100

def send_admin_command(command: dict):
    try:
        with socket.create_connection((HOST, ADMIN_PORT)) as sock:
            message = json.dumps(command) + '\n'
            sock.sendall(message.encode('utf-8'))

            response = sock.recv(1024).decode('utf-8').strip()
            print("Response:", response)
            return json.loads(response)
    except Exception as e:
        print(f"[AdminPort {ADMIN_PORT}] Connection failed: {e}")
        return {"error": str(e)}

def get_snapshot(symbol: str):
    return send_admin_command({"type": "GET_SNAPSHOT", "symbol": symbol})

def export_snapshot(symbol: str):
    return send_admin_command({"type": "EXPORT_SNAPSHOT", "symbol": symbol})

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python admin.py <SYMBOL>")
        sys.exit(1)

    symbol = sys.argv[1]
    export_snapshot(symbol)
    get_snapshot(symbol)
