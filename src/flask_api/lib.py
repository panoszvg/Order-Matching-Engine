import itertools
from datetime import datetime, timezone

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
