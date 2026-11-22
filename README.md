# moscar

A small TCP port scanner for the command line, written in C.

`moscar` tries to open a TCP connection to each port you ask for and reports
which ports accept it. It uses non-blocking sockets and `poll()`, so a silent
port costs a short timeout instead of hanging the whole scan.

**Status: v0.2.** It is simple on purpose: one target, one probe at a time.
See [Limitations](#limitations) and the [Roadmap](#roadmap), and
[CHANGELOG.md](CHANGELOG.md) for what changed since v0.1.

> **Use it only on systems you own or have explicit permission to scan.**
> Scanning other people's machines can break their rules and, depending on
> where you are, the law. Good targets for testing: `127.0.0.1`, your own VMs
> and containers, and `scanme.nmap.org` (which allows scanning).

## Build

Requires Linux and a C compiler (gcc).

```
make            # builds ./moscar
make test       # builds a sanitizer binary and runs the end-to-end tests
make unit       # unit tests for the port set (ports.c)
make check      # lint + unit + end-to-end: everything
make lint       # optimized build with extra warnings and gcc's static analyzer
make clean
```

## Usage

```
moscar <ipv4> [-p PORTS] [-t MS] [-j]
moscar --help
moscar --version
```

| Option | Meaning |
|---|---|
| `-p`, `--port` | Ports to scan: a port `N`, a range `N-M`, or a comma-separated list of those (1-65535) |
| `-t`, `--timeout` | How long to wait for each port, in milliseconds: 1 to 999 (default 300) |
| `-j`, `--json` | Also write the results of this scan to `json.txt` |
| `-v`, `--version` | Show the version and exit |
| `-h`, `--help` | Show the help text and exit |

The target must be an IPv4 address. Options can come before or after it.

### Port syntax

| Input | Ports scanned |
|---|---|
| `80` | 80 |
| `20-25` | 20, 21, 22, 23, 24, 25 |
| `22,80,443` | 22, 80 and 443 |
| `22,80-82,443` | 22, 80, 81, 82 and 443 |

Note that `20,80` means the two ports 20 and 80. In v0.1 it meant the range 20
to 80; use `20-80` for a range.

### Examples

Start a throwaway listener in one terminal:

```
python3 -m http.server 18090 --bind 127.0.0.1
```

Scan a range around it:

```
$ ./moscar 127.0.0.1 -p 18089-18091
127.0.0.1 is a valid IPv4 address.
port 18090 is open on 127.0.0.1

-SUMMARY: ports scanned[3]{ open ports[1], closed ports[2], filtered ports[0], unreachable ports[0], errors[0]}
-time taken: [0.00 s]
```

Only open ports are listed. Everything else is counted in the summary.

A list mixing single ports and ranges:

```
$ ./moscar 127.0.0.1 -p 22,18090,18095-18096
127.0.0.1 is a valid IPv4 address.
port 18090 is open on 127.0.0.1

-SUMMARY: ports scanned[4]{ open ports[1], closed ports[3], filtered ports[0], unreachable ports[0], errors[0]}
-time taken: [0.00 s]
```

A custom timeout (500 ms per port):

```
$ ./moscar 127.0.0.1 -p 18090 --timeout 500
```

No `-p` at all scans the default range 20-80:

```
$ ./moscar 127.0.0.1
No ports are set.
Setting the default port range[20-80].
127.0.0.1 is a valid IPv4 address.
...
```

Show the version:

```
$ ./moscar --version
moscar v0.2
```

### JSON output

With `--json` (or `-j`), moscar writes `json.txt` in the current directory
after the scan has finished:

```
$ ./moscar 127.0.0.1 -p 18089-18091 --json
$ cat json.txt
{
   "ip address": "127.0.0.1",
   "scanned ports": [18089,18090,18091],
   "open ports": [18090]
}
```

The file describes only the scan that just ran and is replaced on every run.
Nothing is written unless you ask for it.

### Reading the results

| Result | Meaning |
|---|---|
| **open** | The connection was accepted. |
| **closed** | The host actively refused the connection. |
| **filtered** | No answer within the timeout (300 ms unless you set `--timeout`). A firewall may be dropping packets, but see the note below. |
| **unreachable** | The network or host could not be reached (no route). |
| **errors** | The probe could not run or ended in an unexpected error. |

The five counters always add up to `ports scanned`. The summary is two lines:
the counters, then the time taken.

> **A "filtered" result is a weak signal.** It only means no reply arrived
> within the timeout. A single lost packet can cause it, and moscar does not
> retry yet. If a port you expect to be open shows up as filtered, run the scan
> again.

## Parsing rules

- Without `-p`, or with a bare `-p` that has no value, the range is **20-80**.
- Repeating `-p` **adds** ports: `-p 80 -p 90-91` scans 80, 90 and 91.
- A value that is really another option (`-p -p`, `--port -p`, `-p -h`,
  `-p --timeout 500`) is **rejected** with an error, and nothing is scanned.
- A reversed range is swapped: `90-80` scans 80-90, also inside a list.
- Duplicates and overlaps are merged: `22,22` scans one port, and `20-30,25`
  scans eleven. Ports are scanned in ascending order.

**Lists** (anything with a comma) are strict:

- Empty items are skipped: `22,,80` is the same as `22,80`.
- Each item must be a port or a range inside 1-65535. `0`, negative numbers,
  numbers above 65535, trailing text (`80abc`) and open-ended ranges (`80-`)
  are rejected.
- Only the first two numbers of a range are used: `80-90-100` means `80-90`.

**A single value or a plain range** (no comma) is parsed forgivingly:

- Trailing text after a number is ignored: `80abc` scans port 80.
- Values above 65535 are clamped to 65535: `65536` scans port 65535.
- A leading minus is ignored: `-5` scans port 5.
- Only the first two numbers are used: `1-2-3` scans 1-2.
- A value with no usable number (`abc`, an empty string) is rejected.

**The timeout** must be a whole number from 1 to 999. `0`, negative numbers,
`1000` and above, decimals, trailing text and a missing value are all rejected.

## Exit codes

| Code | Meaning |
|---|---|
| `0` | The scan ran to completion. |
| `1` | Help or the version was shown, or an error occurred: bad arguments, unknown option, invalid port list or timeout, or an unexpected system error while probing a port (the scan stops). |

Help, version and errors share exit code 1, so a script cannot tell them apart
by the code alone. The process never exits with `-1` (a shell would show that
as 255).

### Internal return values (for contributors)

Option handlers (`help_opt`, `port_ran`, `timeout`, ...) and the
argument-parsing functions use their own three-way convention:

| Value | Meaning |
|---|---|
| `0` | Success, keep going. |
| `1` | Stop cleanly (for example after `--help` or `--version`). |
| `-1` | Error, stop. |

`main` turns any non-zero result into exit code 1. These values are internal
and are not the process exit codes above.

## Project layout

| File | What it does |
|---|---|
| `moscar.c` | `main`: parses arguments, loops over the ports, prints the summary |
| `parsing.c/.h` | Walks the command line and dispatches options |
| `options.c/.h` | The option table and its handlers: ports, timeout, JSON, version, help |
| `ports.c/.h` | The set of ports, a 65,536-bit bitmap |
| `scan.c/.h` | Probes one port and classifies the result |
| `utils.c/.h` | Shared helpers and the `config` struct |
| `run_tests.sh`, `test_ports.c` | End-to-end and unit tests |
| `Makefile`, `CHANGELOG.md` | Build and release notes |

## Testing

```
make check
```

- `make unit` runs `test_ports.c`: every one of the 65,536 ports must have its
  own bit, `setport`/`unsetport` must touch exactly one port, and ranges
  (including ones ending at 65535) must set exactly the right ports.
- `make test` runs `run_tests.sh`. It starts real listeners on localhost (ports
  18080-18082), scans them, and checks the results against what it opened and
  against `ss -ltn`. It also covers option handling, port lists, the timeout
  option (including how long a silent port really waits), JSON output,
  rejected input, big inputs, file descriptor leaks, a failing `socket()` call,
  the no-route path (using `unshare -rn`, skipped if unavailable), and that every
  summary adds up. Each run is checked for hangs, exit codes and sanitizer
  reports, and every scan runs in a scratch directory.

It needs `bash`, `python3`, `ss` (iproute2), and `timeout`. The binary under
test is built with AddressSanitizer and UndefinedBehaviorSanitizer, so memory
errors show up as test failures.

Behaviors that are design decisions (for example, whether a repeated `-p` adds
or replaces) are settings at the top of `run_tests.sh`. To skip the slow scan
of every port from 1 to 65535, run `FULL_RANGE=0 make test`. Pass
`BIN=./other-binary` to test a different build.

## Limitations

- IPv4 only. IPv6 listeners are invisible to it.
- TCP connect scan only. No SYN scan, no UDP.
- One probe at a time. A silent (filtered) port costs the full timeout, so with
  the default 300 ms, 1,000 of them take about five minutes.
- The timeout is at most 999 ms, and there are no retries.
- One target per run: no hostnames and no CIDR ranges.
- The JSON file is always called `json.txt` and goes in the current directory.
- No service or version detection.

## Roadmap

- **v0.3:** retries for unanswered probes, concurrent probing, hostnames and
  CIDR targets.
- **Later:** SYN scanning, service detection.

## License

MIT. See [LICENSE](LICENSE).
