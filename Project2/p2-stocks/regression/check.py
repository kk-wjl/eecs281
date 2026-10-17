#!/usr/bin/env python3
"""Check small TL tests with scans, sorted medians, and exhaustive traveler pairs."""

import argparse
from dataclasses import dataclass
import difflib
from pathlib import Path
import re
import subprocess
import sys
import tarfile


ROOT = Path(__file__).resolve().parents[1]


def normalized_cpp_tokens(source):
    # Only the explicitly reviewed identifier renames may differ from the baseline.
    original_names = {
        "traderID": "trader", "limitPrice": "price", "arrivalID": "sequence",
        "matchPrice": "value", "medianPrice": "value", "tradeValue": "value",
        "lowerHalf": "lower", "upperHalf": "upper", "stockBooks": "stocks",
        "traderStats": "traders", "side": "intent", "stockIndex": "stock",
        "nextArrivalID": "nextSequence", "validateOrder": "validate",
        "matchOrders": "match", "sharesTraded": "quantity", "tradePrice": "price",
        "tradedStockIDs": "stocksWithMedians", "medianIDsNeedSort": "medianOrderDirty",
        "commentLine": "ignoredComment", "headerLabel": "label", "inputMode": "mode",
    }
    pattern = r'''//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|[^\s]'''
    return [original_names.get(token, token) for token in re.findall(pattern, source)
            if not token.startswith(("//", "/*"))]


@dataclass(frozen=True)
class Order:
    time: int
    side: str
    trader: int
    stock: int
    price: int
    quantity: int


def read_test(path, text=None):
    lines = (path.read_text() if text is None else text).splitlines()
    if len(lines) < 4 or not lines[0].startswith("COMMENT: "):
        raise ValueError(f"{path.name}: missing input header")
    if lines[1] != "MODE: TL":
        raise ValueError(f"{path.name}: student tests must use TL mode")
    traders = int(lines[2].removeprefix("NUM_TRADERS: "))
    stocks = int(lines[3].removeprefix("NUM_STOCKS: "))
    if traders <= 0 or stocks <= 0:
        raise ValueError(f"{path.name}: invalid trader/stock count")
    orders = []
    previous_time = 0
    for line in lines[4:]:
        match = re.fullmatch(r"(\d+) (BUY|SELL) T(\d+) S(\d+) \$(\d+) #(\d+)", line)
        if not match:
            raise ValueError(f"{path.name}: malformed order {line!r}")
        time, side, trader, stock, price, quantity = match.groups()
        order = Order(int(time), side, int(trader), int(stock), int(price), int(quantity))
        if (order.time < previous_time or order.trader >= traders
                or order.stock >= stocks or order.price <= 0 or order.quantity <= 0
                or max(order.time, order.trader, order.stock,
                       order.price, order.quantity) > 2147483647):
            raise ValueError(f"{path.name}: invalid order {line!r}")
        orders.append(order)
        previous_time = order.time
    if len(orders) > 30:
        raise ValueError(f"{path.name}: exceeds 30 orders")
    return traders, stocks, orders


def signed32(value):
    # Match the existing 32-bit transfer convention exercised by test-7-i.
    return (value + 2**31) % 2**32 - 2**31


def reference(data, flags, fault=None):
    trader_count, stock_count, orders = data
    output = ["Processing orders..."]
    books = [{"BUY": [], "SELL": []} for _ in range(stock_count)]
    remaining = [order.quantity for order in orders]
    prices = [order.price for order in orders]
    sequence = list(range(len(orders)))
    stats = [[0, 0, 0] for _ in range(trader_count)]
    matches = {}
    current_time = 0
    next_sequence = 0
    completed = 0
    traded_at_time = False

    def print_medians():
        if "m" not in flags or (fault == "median-skip-idle" and not traded_at_time):
            return
        stock_ids = list(matches) if fault == "median-discovery-order" else sorted(matches)
        for stock in stock_ids:
            history = matches[stock]
            if fault == "median-last-four":
                history = history[-4:]
            values = sorted(set(history) if fault == "median-drop-duplicates" else history)
            middle = len(values) // 2
            median = (values[middle] if len(values) % 2
                      else (values[middle - 1] + values[middle]) // 2)
            if fault == "median-divide-before-add" and len(values) % 2 == 0:
                median = values[middle - 1] // 2 + values[middle] // 2
            output.append(f"Median match price of Stock {stock} at time "
                          f"{current_time} is ${median}")

    for index, order in enumerate(orders):
        if order.time != current_time:
            print_medians()
            if fault == "median-reset-per-time":
                matches.clear()
            current_time = order.time
            traded_at_time = False
        sequence[index] = next_sequence
        next_sequence += 1
        book = books[order.stock]
        book[order.side].append(index)
        while True:
            buys = [item for item in book["BUY"] if remaining[item] > 0]
            sells = [item for item in book["SELL"] if remaining[item] > 0]
            if not buys or not sells:
                break
            buyer = min(buys, key=lambda item: (-prices[item], sequence[item]))
            seller = min(sells, key=lambda item: (prices[item], sequence[item]))
            if prices[buyer] < prices[seller]:
                break
            quantity = min(remaining[buyer], remaining[seller])
            price = prices[buyer] if sequence[buyer] < sequence[seller] else prices[seller]
            completed += 1
            traded_at_time = True
            matches.setdefault(order.stock, []).append(price)
            buying_trader = orders[buyer].trader
            selling_trader = orders[seller].trader
            if "v" in flags:
                output.append(f"Trader {buying_trader} purchased {quantity} shares of Stock "
                              f"{order.stock} from Trader {selling_trader} for ${price}/share")
            stats[buying_trader][0] += quantity
            stats[selling_trader][1] += quantity
            stats[buying_trader][2] = signed32(stats[buying_trader][2] - quantity * price)
            stats[selling_trader][2] = signed32(stats[selling_trader][2] + quantity * price)
            for item in (buyer, seller):
                remaining[item] -= quantity
                if remaining[item] == 0:
                    continue
                side = orders[item].side.lower()
                if fault == f"refresh-{side}-sequence":
                    sequence[item] = next_sequence
                    next_sequence += 1
                if fault == f"overwrite-{side}-limit":
                    prices[item] = price

    if fault != "median-skip-eof":
        print_medians()
    output.extend(["---End of Day---", f"Trades Completed: {completed}"])
    if "i" in flags:
        output.append("---Trader Info---")
        for trader, (bought, sold, net) in enumerate(stats):
            output.append(f"Trader {trader} bought {bought} and sold {sold} "
                          f"for a net transfer of ${net}")
    if "t" in flags:
        output.append("---Time Travelers---")
        for stock in range(stock_count):
            candidates = []
            for sell_index, sell in enumerate(orders):
                if sell.stock != stock or sell.side != "SELL":
                    continue
                if fault == "traveler-small-price-sentinel" and sell.price >= 1073741824:
                    continue
                for buy_index, buy in enumerate(orders):
                    if buy.stock != stock or buy.side != "BUY" or buy.price <= sell.price:
                        continue
                    if fault != "traveler-reverse-order" and buy_index <= sell_index:
                        continue
                    if fault == "traveler-later-timestamp" and buy.time <= sell.time:
                        continue
                    profit = buy.price - sell.price
                    if fault == "traveler-weight-quantity":
                        profit *= min(sell.quantity, buy.quantity)
                    tie = (sell.time, buy.time, sell_index, buy_index)
                    if fault == "traveler-latest-tie":
                        tie = tuple(-value for value in tie)
                    candidates.append(((-profit, *tie), sell, buy))
            if not candidates:
                output.append(f"A time traveler could not make a profit on Stock {stock}")
            else:
                _, sell, buy = min(candidates, key=lambda candidate: candidate[0])
                output.append(f"A time traveler would buy Stock {stock} at time {sell.time} "
                              f"for ${sell.price} and sell it at time {buy.time} for ${buy.price}")
    return "\n".join(output) + "\n"


def compare(expected, actual, label):
    if expected != actual:
        difference = "".join(difflib.unified_diff(
            expected.splitlines(True), actual.splitlines(True),
            fromfile=f"{label}: expected", tofile=f"{label}: actual"))
        raise ValueError(difference)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "market")
    args = parser.parse_args()
    binary = args.binary.resolve()
    paths = sorted(ROOT.glob("test-*.txt"))
    if len(paths) != 10:
        raise ValueError(f"Expected 10 student tests; found {len(paths)}")
    used_numbers = set()
    tests = {}
    runs = 0
    golden_outputs = 0
    for path in paths:
        match = re.fullmatch(r"test-([0-9])-([vmit])\.txt", path.name)
        if not match or match[1] in used_numbers:
            raise ValueError(f"Invalid or duplicate test number: {path.name}")
        used_numbers.add(match[1])
        data = read_test(path)
        tests[path.name] = data
        expected_file = ROOT / "regression" / f"expected-{match[1]}-{match[2]}.out"
        if expected_file.exists():
            compare(expected_file.read_text(), reference(data, match[2]), path.name)
            golden_outputs += 1
        for flags in ("v", "m", "i", "t", "vmit"):
            result = subprocess.run([str(binary), f"-{flags}"], input=path.read_text(),
                                    text=True, capture_output=True, timeout=5, check=True)
            if result.stderr:
                raise ValueError(f"{path.name}: unexpected stderr: {result.stderr}")
            compare(reference(data, flags), result.stdout, f"{path.name} -{flags}")
            runs += 1
        print(f"PASS {path.name}: {len(data[2])} orders; v/m/i/t/all")

    faults = {
        "test-9-v.txt": ("refresh-sell-sequence", "refresh-buy-sequence",
                         "overwrite-sell-limit", "overwrite-buy-limit"),
        "test-6-m.txt": ("median-discovery-order", "median-skip-eof",
                         "median-skip-idle", "median-reset-per-time",
                         "median-divide-before-add"),
        "test-1-m.txt": ("median-drop-duplicates", "median-last-four"),
        "test-8-m.txt": ("median-drop-duplicates", "median-last-four"),
        "test-4-t.txt": ("traveler-small-price-sentinel",),
        "test-5-t.txt": ("traveler-reverse-order", "traveler-later-timestamp",
                         "traveler-latest-tie", "traveler-weight-quantity"),
    }
    for name, mutations in faults.items():
        data = tests[name]
        flag = name[-5]
        expected = reference(data, flag)
        for fault in mutations:
            if reference(data, flag, fault) == expected:
                raise ValueError(f"{name} does not distinguish local fault: {fault}")
            print(f"DISTINGUISH {name}: {fault}")
    baseline_path = ROOT / "regression" / "baseline-14.tar.gz"
    baseline_tests = {}
    if baseline_path.exists():
        with tarfile.open(baseline_path) as baseline:
            original_source = baseline.extractfile("market.cpp").read().decode()
            current_source = (ROOT / "market.cpp").read_text()
            if normalized_cpp_tokens(original_source) != normalized_cpp_tokens(current_source):
                raise ValueError("market.cpp changed beyond the reviewed names and comments")
            for name in ("test-0-v.txt", "test-2-i.txt", "test-3-t.txt",
                         "test-5-t.txt", "test-7-i.txt"):
                if baseline.extractfile(name).read() != (ROOT / name).read_bytes():
                    raise ValueError(f"Previously passing code/test changed: {name}")
            for name in tests:
                text = baseline.extractfile(name).read().decode()
                baseline_tests[name] = read_test(ROOT / name, text)
        print("PASS: source tokens differ only in reviewed names/comments; 5 coverage tests unchanged")
        for fault in ("median-divide-before-add", "median-drop-duplicates",
                      "median-last-four", "traveler-small-price-sentinel"):
            old_catches = [name for name, data in baseline_tests.items()
                           if reference(data, name[-5], fault) != reference(data, name[-5])]
            new_catches = [name for name, data in tests.items()
                           if reference(data, name[-5], fault) != reference(data, name[-5])]
            if old_catches or not new_catches:
                raise ValueError(f"No new coverage for {fault}: old={old_catches}; new={new_catches}")
            print(f"NEW COVERAGE {fault}: {', '.join(new_catches)} (previously none)")
    model_count = len({fault for mutations in faults.values() for fault in mutations})
    print(f"PASS: {runs} program comparisons; {golden_outputs} hand-checked outputs; "
          f"{model_count} local fault models (not the instructor's hidden implementations).")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
