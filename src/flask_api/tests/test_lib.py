import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from lib import to_fix

BASE_ORDER = {"type": "BUY", "security": "BTC", "price": 100.0, "quantity": 5.0, "id": "abc-123"}


def test_buy_order_encodes_side_as_1():
    assert "54=1" in to_fix({**BASE_ORDER, "type": "BUY"})


def test_sell_order_encodes_side_as_2():
    assert "54=2" in to_fix({**BASE_ORDER, "type": "SELL"})


def test_symbol_included():
    assert "55=BTC" in to_fix(BASE_ORDER)


def test_price_included():
    msg = to_fix(BASE_ORDER)
    assert "44=100.0" in msg


def test_quantity_included():
    msg = to_fix(BASE_ORDER)
    assert "38=5.0" in msg


def test_order_id_included():
    msg = to_fix({**BASE_ORDER, "id": "my-order-id"})
    assert "11=my-order-id" in msg


def test_begins_with_fix_version():
    assert to_fix(BASE_ORDER).startswith("8=FIX.4.4\x019=")


def test_contains_message_type_new_order():
    assert "35=D" in to_fix(BASE_ORDER)


def test_ends_with_newline():
    assert to_fix(BASE_ORDER).endswith("\n")


def test_checksum_field_present():
    assert "10=" in to_fix(BASE_ORDER)


def test_fields_separated_by_soh():
    msg = to_fix(BASE_ORDER)
    assert "\x01" in msg


def test_fix44_required_fields_present():
    tags = {f.split("=", 1)[0] for f in to_fix(BASE_ORDER).strip().split("\x01") if f}
    assert {"8", "9", "35", "49", "56", "34", "52", "11", "55", "54", "38", "40", "60", "10"} <= tags


def test_limit_order_type():
    assert "\x0140=2\x01" in to_fix(BASE_ORDER)


def test_body_length_matches():
    msg = to_fix(BASE_ORDER).rstrip("\n")
    header, rest = msg.split("\x019=", 1)
    length, body = rest.split("\x01", 1)
    body = body[: body.rindex("10=")]
    assert int(length) == len(body.encode("utf-8"))


def test_checksum_matches():
    msg = to_fix(BASE_ORDER).rstrip("\n")
    idx = msg.rindex("10=")
    assert int(msg[idx + 3 : -1]) == sum(msg[:idx].encode("utf-8")) % 256


def test_sequence_number_increments():
    def seq(msg):
        return int(next(f for f in msg.split("\x01") if f.startswith("34="))[3:])
    assert seq(to_fix(BASE_ORDER)) + 1 == seq(to_fix(BASE_ORDER))
