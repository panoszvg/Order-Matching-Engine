import socket
import json
import time
import random
import uuid
import threading

HOST = '127.0.0.1'
JSON_PORTS = [9000, 9001]
SECURITY = "SOFR"
ORDERS_PER_CLIENT = 10
DELAY_BETWEEN_ORDERS = 0.1

PRICE_LEVELS = [98.0, 99.0, 100.0, 101.0, 102.0]

def generate_order():
    return {
        "id": str(uuid.uuid4()),
        "security": SECURITY,
        "type": random.choice(["BUY", "SELL"]),
        "price": random.choice(PRICE_LEVELS),
        "quantity": random.randint(1, 20)
    }

def send_orders(port):
    try:
        with socket.create_connection((HOST, port)) as sock:
            for _ in range(ORDERS_PER_CLIENT):
                order = generate_order()
                message = json.dumps(order) + '\n'
                sock.sendall(message.encode('utf-8'))
                print(f"[Port {port}] Sent:", message.strip())
                time.sleep(DELAY_BETWEEN_ORDERS)
    except Exception as e:
        print(f"[Port {port}] Connection failed: {e}")

def main():
    threads = [threading.Thread(target=send_orders, args=(port,)) for port in JSON_PORTS]

    for t in threads:
        t.start()
    for t in threads:
        t.join()

if __name__ == '__main__':
    main()
