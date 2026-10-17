# Project 2B: Priority Queues

The priority-queue implementations live in the `*PQ.hpp` headers.
`Eecs281PQ.hpp` defines the common interface and should not be modified because
the autograder supplies its own copy.

## Build and check

```sh
make alltests
./test_comprehensive

make release
./project2b
```

`project2b.cpp` is the interactive course-provided test driver.
`test_comprehensive.cpp` runs every implementation against deterministic,
randomized, comparator, pointer-priority, copy/move, and pairing-heap tests.
The sanitizer-enabled `make debug` target is intended for the CAEN Linux
environment; some macOS toolchains cannot link its supplied sanitizer flags.

The submission targets in the Makefile package the required headers:

```sh
make fullsubmit
make partialsubmit
```
