def to_fix(order):
    # Very basic FIX string format
    type = '1' if order["type"] == "BUY" else '2'
    fields = [
        f"8=FIX.4.2",
        f"35=D",
        f"55={order['security']}",
        f"54={type}",
        f"44={order['price']}",
        f"38={order['quantity']}",
        f"11={order['id']}",
        "10=000"
    ]
    return ''.join(fields) + '\n'
