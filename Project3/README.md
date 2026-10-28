# Project 3: Log Manager

`logman.cpp` reads a master log, supports timestamp, category, and keyword
searches, and maintains an ordered excerpt list. The assignment specification
is [here](https://eecs281staff.github.io/p3-logman/).

## Build and run

```sh
make -R -r
./logman -f spec-test-log.txt < spec-test-cmds.txt
```

The default Makefile target builds the optimized `logman` executable. Use
`make debug` to build `logman_debug` with address and undefined-behavior
sanitizers. To compare against the supplied example output:

```sh
./logman -f spec-test-log.txt < spec-test-cmds.txt > actual-out.txt
diff -u spec-test-out.txt actual-out.txt
```

## Design

The master log keeps each entry once with its original entry ID. A sorted ID
index supports binary-search timestamp queries. Category and keyword maps
store IDs in that same sorted order, so searches and appends do not need to
sort again. The excerpt list stores IDs rather than copies of log messages.
These IDs and index positions use 32-bit values to reduce memory use on large
logs.

## Tests and submission

`test-1-log.txt` through `test-15-log.txt` pair with the corresponding
`test-*-cmds.txt` files. The cases cover search ordering, case-insensitive
matching, whole-word keywords, repeated and empty results, timestamp
boundaries, and excerpt-list edits. Every test file is within the assignment's
20-line limit, and every command file ends with `q`.

```sh
make fullsubmit
```

This creates `fullsubmit.tar.gz` with the source, Makefile, and test pairs.
Generated executables, object files, and submission archives are ignored by
Git.
