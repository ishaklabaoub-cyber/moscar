### TODO


## Task 1: make the counters always add up:
- Right now case _UNDEFINED and a failed socket() skip the counters. Do it test-first:    

1. Reproduce the socket() failure. Starve the process of file descriptors, so socket() fails with EMFILE:
   ( ulimit -n 3; ./moscar 127.0.0.1 -p 80-82 )

See what the summary says. Write down what it should say.
2. Decide on the new counter. An errors bucket is the simplest. Add its label to run_tests.sh (an ERRORS_LABEL, included in the total in base_checks), and add the failing test above.
3. Change the code. Every probe should land in exactly one counter. Make the outcome an enum, handle it in a switch without a catch-all default, and compile with -Wswitch-enum, so the compiler tells you when an outcome isn’t handled.
4. Run make lint && make test. Update the README sentence back to “always add up” and add the new counter to the summary example. Commit.

## Task 2: split the code into modules (one file at a time):
# Aim for ports.c/.h (parsing and the port set), scan.c/.h (scan_port and the enum), output.c/.h (summary and printing), options.c/.h (the option table and handlers), and a short main.c.

# Order: start with ports because it’s pure logic with no sockets, then scan, then output, then options.
# Rule: move one piece, update the Makefile, run make test, and commit. The suite should stay at 50 passing for the whole refactor.
# Makefile: build from $(SRC) with several .c files, and compile objects to .o files so only changed files rebuild. Add *.o to .gitignore (it’s probably already there).
# Bonus: once ports.c exists, write test_ports.c and call the parser directly with a table of inputs and expected results.

## Task 3: port lists with a bitmap:
# Spec first. Write the grammar: a list is items separated by commas, and an item is N or N#M. Then write the accept/reject table (examples to cover: 22,80,443, 22,80#90,443, 80,80, 22,,80, 22,, ,22, 0, 65536) and the tests before any code.
# Decision to make: v0.1 is lenient (80abc scans 80, 65536 clamps to 65535). Do lists keep those rules or become strict? Lenient parsing plus lists gets ambiguous (22,80abc,443), so I’d make lists strict and note it in the changelog.
# Data structure: a 65,536#bit bitmap in a struct with a count. Scanning iterates from 1 to 65535 and checks the bit, so duplicates vanish automatically and ports are visited in order.
Use endptr with strtol to detect junk and to know where each item ends.
# Wire up the summary: ports scanned is now the number of set bits, not max # min + 1.

## Task 4: retries and ##timeout:
# Add timeout_ms (default 300) and retries (default 1 or 2) to struct config, plus ##timeout <ms> as an option that takes a value (decide whether ##timeout=500 is accepted too).
# Retry only unanswered probes. A refused connection is a final answer, so don’t retry it.
# Time with CLOCK_MONOTONIC, and set sensible bounds on the timeout.
# Test it with a late listener: start the listener about 500 ms after the scan begins, and check that the port is found with retries on and missed with ##retries 0 (or whatever option you choose).

## Task 5: finish up
# --version, and a decision on the help exit code (many tools use 0). If you change it, update HELP_EXIT in the tests and the README.
# Update the README (options table, list syntax, timeouts, limitations) and the changelog.
# Run the final checks: make lint && make test, a fresh clone, then git tag -a v0.2, push, and create the release.
