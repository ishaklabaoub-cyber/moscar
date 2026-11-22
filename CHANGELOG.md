# Changelog

## v0.2

### Added
- **Port lists.** `-p` now accepts single ports, ranges and lists mixed together:
  `-p 22,80-90,443`. Duplicates and overlaps are merged, and ports are scanned
  in ascending order.
- **`--timeout <ms>` (`-t`).** Sets how long to wait for each port, from 1 to 999
  milliseconds. The default is 300 ms. A value of 0, a negative value, or 1000 or
  more is rejected.
- **`--json` (`-j`).** Writes `json.txt` describing the scan that just ran (address,
  scanned ports, open ports). The file is replaced on every run, and nothing is
  written unless the option is given.
- **`--version` (`-v`).** Prints `moscar v0.2`.
- **`errors` counter** in the summary: probes that could not run or ended in an
  unexpected error (for example, the process ran out of file descriptors) are now
  counted, so the counters always add up to `ports scanned`.
- **Test suite.** `make check` runs static checks (`make lint`), unit tests for the
  port set (`make unit`, 83 checks) and end-to-end tests (`make test`, 127 cases).

### Changed
- **Breaking: `N,M` now means ports N and M, not the range N to M.** Use `N-M` for
  a range. In v0.1, `-p 20,80` scanned 20 through 80.
- **Breaking: the summary format changed.** It is now two lines that start with
  `-SUMMARY` and `-time taken`, and it has an `errors[...]` field. Scripts that parse
  the output need updating.
- Repeating `-p` adds ports to the set instead of replacing it:
  `-p 80 -p 90-91` scans 80, 90 and 91.
- Ports are stored in a 65,536-bit bitmap, split into its own module (`ports.c`).
- The code is split into modules: `scan`, `options`, `parsing`, `ports`, `utils`.

### Fixed
- Scans of ranges that end at 65535 (for example `-p 1-65535`) no longer hang.
- A failed `socket()` call is counted instead of being dropped from the summary.
- Port lists no longer overflow the stack.

### Parsing rules
- Empty list items are skipped: `22,,80` is the same as `22,80`.
- Only the first two numbers of a range are used: `80-90-100` means `80-90`.
- A reversed range is swapped: `90-80` means `80-90`, also inside a list.
- Ports outside 1-65535 and malformed values are rejected with an error.
- An option used as the value of another option (`-p -p`, `--port -p`,
  `--timeout -p`) is rejected.

### Known limitations
- IPv4 and TCP connect scans only.
- One probe at a time, so a port that never answers costs its full timeout.
- No retries. A port reported as filtered may just have lost a packet.
- The timeout cannot be 1000 ms or longer.
- `--help`, `--version` and errors all exit with code 1.
