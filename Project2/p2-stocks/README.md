# Project 2A: Stock Market Simulation

`market.cpp` contains the simulator. `P2random.hpp` is required for
pseudo-random (`PR`) input mode.

## Build and check

```sh
make release
./market -vmit < spec-input-TL.txt > output.txt
diff -u spec-output-all.txt output.txt
```

Use `make debug` for the sanitizer-enabled build. The supplied `TL` and `PR`
inputs describe the same orders, so each pair should produce identical output
when run with the same flags.

Student-created files named `test-<number>-<option>.txt` exercise the required
output modes: `v` (verbose), `m` (median), `i` (trader info), and `t` (time
traveler).

## Student test coverage

`test-9-v.txt` keeps competing same-price orders queued before partial fills,
on both buy and sell books. Remaining orders must retain their original FIFO
position and limit price even when the last execution used a different price.
The final orders build deeper books with price priorities opposite trader ID
order, then consume several resting orders with one incoming order.

`test-6-m.txt` starts with unmatched orders, then trades stocks in order 3, 0,
2. Median output must use ascending stock IDs, keep cumulative prices, omit
untraded stocks, and include the final idle timestamp 200. Another stock adds
eight increasing odd prices; e.g. averaging 11 and 23 must produce 17, not
the 16 obtained by dividing each operand first.

`test-1-m.txt` adds a decreasing stream with repeated prices; repeated matches
must count separately. `test-8-m.txt` alternates large and small prices for 11
matches on one stock, with duplicates and a snapshot after each match. Correct
middle-pair sums stay below INT_MAX even though individual prices are large.

`test-5-t.txt` combines the previous tie and same-timestamp scenarios in 29
orders. It includes exhausted offers, reversed input order, an untouched
stock, and a high-volume low-profit pair that must not replace the best
per-share result. Test 3 retains the other traveler direction cases. Test 4
keeps its original transitions and adds prices above one billion, a profitable
two-dollar difference, declining later buy offers, and timestamp INT_MAX.

Run the local regression check after building:

```sh
python3 regression/check.py
```

The checker validates all 10 inputs and compares all output modes with an
independent small-input reference using linear order searches, sorted medians,
and exhaustive traveler pairs. Four tests also have manually
checked expected output files. Sixteen local faulty models check that these
inputs distinguish their intended mistakes; these are not the instructor's
hidden implementations or bug numbers.

## Final submission revision

The report for submission `26.10.16.134955` showed 43/43 executable tests,
no leaks, and 14/16 buggy implementations caught (missing IDs 9 and 16).
Tests 0, 2, 3, 5, and 7 together caught all 14; those five files and
all other student test inputs remain unchanged by the subsequent naming cleanup.

The earlier archive is preserved as `regression/baseline-14.tar.gz`.
The checker compares against it to guard the retained cases and confirms four
new classes of local model coverage. Three C++ fault variants (premature
division, duplicate removal, and a small traveler price sentinel) were also
compiled and run: each passed the earlier 10 inputs and failed the expanded
inputs. These experiments do not identify the instructor's remaining IDs.

The naming cleanup distinguishes order limit prices from execution prices,
arrival IDs from timestamps, and stock/trader IDs from their containers.
Comments describe FIFO after partial fills, median heap balance, traveler
ties, and the existing 32-bit arithmetic. The checker verifies that source
tokens match the earlier program after removing comments and undoing only
the reviewed identifier renames. The expanded test archive before this
cleanup is preserved as `regression/before-naming-cleanup.tar.gz`.

Files under `regression/` are development-only and are excluded by the
Makefile's submission file list. Use `make fullsubmit` to rebuild the archive.
