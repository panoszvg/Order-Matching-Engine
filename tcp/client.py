import socket
import json
import time
import random
import uuid

HOST = '127.0.0.1'
PORT = 9000

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

def main():
    with socket.create_connection((HOST, PORT)) as sock:
        for _ in range(10):
            order = generate_order()
            message = json.dumps(order) + '\n'
            sock.sendall(message.encode('utf-8'))
            print("Sent:", message.strip())
            time.sleep(0.1)

if __name__ == '__main__':
    main()
