import socket
import json
import time
import random
import uuid
import threading
import itertools
from datetime import datetime, timezone

HOST = '127.0.0.1'
JSON_PORTS = [9000, 9001]
FIX_PORTS = [9002, 9003]
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

_seq_num = itertools.count(1)

def to_fix(order):
    # FIX 4.4 New Order Single (35=D), limit order
    side = '1' if order["type"] == "BUY" else '2'
    now = datetime.now(timezone.utc).strftime("%Y%m%d-%H:%M:%S.%f")[:-3]
    fields = [
        "35=D",
        "49=CLIENT",
        "56=ENGINE",
        f"34={next(_seq_num)}",
        f"52={now}",
        f"11={order['id']}",
        f"55={order['security']}",
        f"54={side}",
        f"38={order['quantity']}",
        "40=2",
        f"44={order['price']}",
        f"60={now}",
    ]
    body = '\x01'.join(fields) + '\x01'
    message = f"8=FIX.4.4\x019={len(body.encode('utf-8'))}\x01" + body
    checksum = sum(message.encode('utf-8')) % 256
    return message + f"10={checksum:03d}" + '\x01' + '\n'

def send_orders(port, use_fix):
    try:
        with socket.create_connection((HOST, port)) as sock:
            for _ in range(ORDERS_PER_CLIENT):
                order = generate_order()
                message = to_fix(order) if use_fix else json.dumps(order) + '\n'
                sock.sendall(message.encode('utf-8'))
                print(f"[Port {port}] Sent:", message.strip())
                time.sleep(DELAY_BETWEEN_ORDERS)
    except Exception as e:
        print(f"[Port {port}] Connection failed: {e}")

def main():
    threads = []
    for port in JSON_PORTS:
        threads.append(threading.Thread(target=send_orders, args=(port, False)))
    for port in FIX_PORTS:
        threads.append(threading.Thread(target=send_orders, args=(port, True)))

    for t in threads:
        t.start()
    for t in threads:
        t.join()

if __name__ == '__main__':
    main()
