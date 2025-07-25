import socket
import json
import time
import random
import uuid
import threading

HOST = '127.0.0.1'
PORTS = [9000, 9001, 9002]
ORDERS_PER_CLIENT = 10
DELAY_BETWEEN_ORDERS = 0.1

def generate_order():
    security = random.choice(["ADA", "ETH", "BTC"])
    tick_size = {
        "ADA": 0.001,
        "BTC": 0.01,
        "ETH": 0.01,
    }[security]

    ticks = random.randint(99000, 105000)
    price = round(ticks * tick_size, 6)

    return {
        "id": str(uuid.uuid4()),
        "security": security,
        "type": random.choice(["BUY", "SELL"]),
        "price": price,
        "quantity": random.randint(1, 10)
    }

def send_orders_to_port(port):
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
    threads = []
    for port in PORTS:
        t = threading.Thread(target=send_orders_to_port, args=(port,))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

if __name__ == '__main__':
    main()
