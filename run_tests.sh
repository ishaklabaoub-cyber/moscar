#!/usr/bin/env bash
#
# run_tests.sh - end-to-end tests for moscar (scan_port, parsing, summary).
#
# It starts real listeners on localhost, scans them, and checks the output against
# known truth: the ports this script opened, and `ss -ltn`.
#
# Every scan run is checked for: a hang, a non-zero exit code, a sanitizer report,
# and a SUMMARY that adds up:
#     scanned == open + closed + filtered + unreachable + errors
#
# Usage:
#   make test                                  # builds ./moscar_asan and runs this script
#   BIN=./moscar_asan bash run_tests.sh        # run it by hand
#   BIN=./moscar bash run_tests.sh             # test the normal build instead
#
# Needs: bash, python3, ss (iproute2), timeout. The no-route test also needs
# `unshare -rn` and is skipped when unavailable.
#
# Expected SUMMARY shape (edit the *_LABEL variables if yours changes):
#   SUMMARY: ports scanned[3] { open ports[2], closed ports[1], filtered ports[0],
#            unreachable ports[0]} --> time taken: [0.00 s]

BIN=${BIN:-./moscar_asan}
HOST=127.0.0.1

SCANNED_LABEL="ports scanned"
OPEN_LABEL="open ports"
CLOSED_LABEL="closed ports"
FILTERED_LABEL="filtered ports"
UNREACHABLE_LABEL="unreachable ports"
ERRORS_LABEL="errors"            # v0.2: probes that could not run or ended in an unexpected error

# Decisions about YOUR program's behavior. Change them here when you change the program.
HELP_EXIT=1             # exit code after --help / -h (many tools use 0)
ERR_EXIT=1              # exit code for rejected input (bad IP, bad option, bad port value)
DEFAULT_RANGE_COUNT=61  # ports scanned when -p is given with no value (default 20-80)
NO_P_COUNT=61           # ports scanned when NO -p is given at all (default range 20-80)
ORIG_RUN_TIMEOUT=${RUN_TIMEOUT:-30}
RUN_TIMEOUT=${RUN_TIMEOUT:-30}   # seconds before one run is killed and reported as a hang
FULL_RANGE=${FULL_RANGE:-1}      # 1: also scan every port 1-65535 once (slow; set 0 to skip)

# ---- DESIGN DECISIONS: set each one to what YOUR program does -----------------
# (or override from the shell:  REPEAT_P=replace bash run_tests.sh)
REPEAT_P=${REPEAT_P:-union}                  # union: every -p adds ports | replace: the last -p wins | reject: a second -p is an error
EMPTY_ITEMS=${EMPTY_ITEMS:-skip}             # skip: 22,,80  22,  ,22 ignore empty items | reject: errors
EXTRA_RANGE_PART=${EXTRA_RANGE_PART:-ignore} # ignore: 80-90-100 uses 80-90 | reject: it is an error
REVERSED_RANGE=${REVERSED_RANGE:-swap}       # swap: 90-80 means 80-90 (also inside lists) | reject: error
TIMEOUT_MIN=${TIMEOUT_MIN:-1}                # smallest --timeout accepted, in milliseconds
TIMEOUT_MAX=${TIMEOUT_MAX:-1000}            # largest --timeout accepted, in milliseconds
REPEAT_TIMEOUT=${REPEAT_TIMEOUT:-last}       # last: the last --timeout wins | reject: a second --timeout is an error

# Test ports: A and C get listeners; B and the rest of the range stay closed.
PORT_A=18080
PORT_B=18081
PORT_C=18082
SCAN_LO=$PORT_A
SCAN_HI=$((PORT_A + 9))

PASS=0
FAIL=0
RUNNER=""                # optional command prefix, e.g. "unshare -rn"
declare -A LISTENER_PID

# ---------------------------------------------------------------- helpers

pass() { PASS=$((PASS + 1)); printf '  ok    %s\n' "$1"; }
fail() {
    FAIL=$((FAIL + 1))
    printf '  FAIL  %s\n' "$1"
    [ -n "$2" ] && printf '        -> %s\n' "$2"
}

port_in_use() { ss -ltnH "sport = :$1" | grep -q .; }

start_listener() {
    python3 -m http.server "$1" --bind "$HOST" >/dev/null 2>&1 &
    LISTENER_PID[$1]=$!
    for _ in $(seq 1 50); do
        port_in_use "$1" && return 0
        sleep 0.1
    done
    echo "could not start a listener on port $1" >&2
    exit 2
}

stop_listener() {
    local pid=${LISTENER_PID[$1]}
    [ -z "$pid" ] && return
    kill "$pid" 2>/dev/null
    wait "$pid" 2>/dev/null
    unset 'LISTENER_PID[$1]'
    for _ in $(seq 1 50); do
        port_in_use "$1" || return 0
        sleep 0.1
    done
}

cleanup() {
    for p in "${LISTENER_PID[@]}"; do
        kill "$p" 2>/dev/null
    done
    wait 2>/dev/null
}
trap cleanup EXIT

# Number inside e.g. "open ports[2]" from the scanner's output.
field() { grep -o "$1\[[0-9]*\]" <<<"$OUT" | head -n1 | grep -o '[0-9]*'; }

# run_scan <args...>  ->  sets OUT ERR CODE ELAPSED S O C F U E
run_scan() {
    local errf t0 t1
    errf=$(mktemp)
    t0=$(date +%s.%N)
    # shellcheck disable=SC2086   # RUNNER is meant to word-split ("unshare -rn")
    OUT=$(timeout "$RUN_TIMEOUT" $RUNNER "$BIN" "$@" 2>"$errf" </dev/null)
    CODE=$?
    t1=$(date +%s.%N)
    ERR=$(<"$errf")
    rm -f "$errf"
    ELAPSED=$(awk -v a="$t0" -v b="$t1" 'BEGIN { printf "%.2f", b - a }')
    S=$(field "$SCANNED_LABEL")
    O=$(field "$OPEN_LABEL")
    C=$(field "$CLOSED_LABEL")
    F=$(field "$FILTERED_LABEL")
    U=$(field "$UNREACHABLE_LABEL")
    E=$(field "$ERRORS_LABEL")
}

sanitizer_hit() { grep -qE "Sanitizer|runtime error" <<<"$ERR"; }

# Problems that apply to every SUCCESSFUL scan run. Prints a description, or nothing.
base_checks() {
    local p="" total
    [ "$CODE" -eq 124 ] && p+="HANG (timeout); "
    [ "$CODE" -ne 0 ] && [ "$CODE" -ne 124 ] && p+="exit code $CODE (wanted 0); "
    sanitizer_hit && p+="sanitizer report on stderr; "
    if [ -z "$S" ]; then
        p+="no SUMMARY line found; "
    else
        total=$(( ${O:-0} + ${C:-0} + ${F:-0} + ${U:-0} + ${E:-0} ))
        [ "$S" -ne "$total" ] &&
            p+="summary does not add up: scanned=$S but open+closed+filtered+unreachable+errors=$total; "
    fi
    printf '%s' "$p"
}

# expect <name> <scanned> <open> <closed>   (use "-" to skip a field)
expect() {
    local name=$1 es=$2 eo=$3 ec=$4 problems
    problems=$(base_checks)
    [ "$es" != "-" ] && [ "$S" != "$es" ] && problems+="scanned=$S (wanted $es); "
    [ "$eo" != "-" ] && [ "$O" != "$eo" ] && problems+="open=$O (wanted $eo); "
    [ "$ec" != "-" ] && [ "$C" != "$ec" ] && problems+="closed=$C (wanted $ec); "
    if [ -z "$problems" ]; then pass "$name"; else fail "$name" "$problems"; fi
}

# expect_scan <name> <ports-scanned> <args...> : succeeds and scans exactly that many ports
expect_scan() {
    local name=$1 n=$2
    shift 2
    run_scan "$@"
    expect "$name" "$n" - -
}

# expect_reject <name> <args...> : bad input must exit with ERR_EXIT and must NOT start a scan
expect_reject() {
    local name=$1 p=""
    shift
    run_scan "$@"
    [ "$CODE" -eq 124 ] && p+="HANG (timeout); "
    [ "$CODE" -ne "$ERR_EXIT" ] && [ "$CODE" -ne 124 ] && p+="exit code $CODE (wanted $ERR_EXIT); "
    sanitizer_hit && p+="sanitizer report on stderr; "
    grep -q "SUMMARY" <<<"$OUT" && p+="a scan ran despite bad input; "
    if [ -z "$p" ]; then pass "$name"; else fail "$name" "$p"; fi
}

# expect_help <name> <args...> : prints usage, exits with HELP_EXIT, does not scan
expect_help() {
    local name=$1 p=""
    shift
    run_scan "$@"
    [ "$CODE" -eq 124 ] && p+="HANG (timeout); "
    [ "$CODE" -ne "$HELP_EXIT" ] && p+="exit code $CODE (wanted $HELP_EXIT); "
    sanitizer_hit && p+="sanitizer report on stderr; "
    grep -qi "usage" <<<"$OUT" || p+="no usage text printed; "
    grep -q "SUMMARY" <<<"$OUT" && p+="a scan ran after help; "
    if [ -z "$p" ]; then pass "$name"; else fail "$name" "$p"; fi
}

# Sorted, space-separated list of ports the scanner reported open.
scanner_open_ports() {
    grep -oE 'port [0-9]+ is open' <<<"$OUT" | awk '{print $2}' | sort -n | tr '\n' ' '
}

# Same, from `ss`. Only IPv4-reachable listeners count (the scanner is IPv4 only).
ss_open_ports() {
    ss -ltnH "( sport >= :$1 and sport <= :$2 )" |
        awk '$4 ~ /^(127\.0\.0\.1|0\.0\.0\.0|\*):/ {print $4}' |
        sed 's/.*://' | sort -nu | tr '\n' ' '
}

# ------------------------------------------------------------ preflight

if [ ! -x "$BIN" ]; then
    echo "binary '$BIN' not found or not executable (run 'make test', or build it first)" >&2
    exit 2
fi
for tool in ss python3 timeout; do
    command -v "$tool" >/dev/null || { echo "missing tool: $tool" >&2; exit 2; }
done
for p in $(seq "$SCAN_LO" "$SCAN_HI"); do
    if port_in_use "$p"; then
        echo "port $p is already in use; change PORT_A in this script" >&2
        exit 2
    fi
done

start_listener "$PORT_A"
start_listener "$PORT_C"

# ----------------------------------------------------------------- tests

echo "== single ports =="
run_scan "$HOST" -p "$PORT_A"
expect "open port is counted open" 1 1 0
grep -q "port $PORT_A is open" <<<"$OUT" \
    && pass "open port is printed by number" \
    || fail "open port is printed by number" "no 'port $PORT_A is open' line"
grep -qE 'time taken: \[[0-9.]+ s\]' <<<"$OUT" \
    && pass "summary reports time taken" \
    || fail "summary reports time taken" "no 'time taken: [N s]' in summary"

run_scan "$HOST" -p "$PORT_B"
expect "closed port is counted closed" 1 0 1
grep -q "is open" <<<"$OUT" \
    && fail "closed port prints nothing as open" "an 'is open' line was printed" \
    || pass "closed port prints nothing as open"

echo "== ranges and argument order =="
expect_scan "range with 2 open + 1 closed" 3 "$HOST" -p "$PORT_A-$PORT_C"
expect "  ...and the counts are right" 3 2 1    # reuses that run's results
expect_scan "range of one port (N-N)" 1 "$HOST" -p "$PORT_A-$PORT_A"
expect_scan "long option --port" 3 "$HOST" --port "$PORT_A-$PORT_C"
expect_scan "target before options" 3 "$HOST" -p "$PORT_A-$PORT_C"
expect_scan "options before target" 3 -p "$PORT_A-$PORT_C" "$HOST"

echo "== boundaries =="
expect_scan "lowest port (1)" 1 "$HOST" -p 1
expect_scan "highest port (65535)" 1 "$HOST" -p 65535
expect_scan "ports 1-3" 3 "$HOST" -p 1-3

echo "== lenient parsing (design choices - edit the numbers if you change a rule) =="
expect_scan "reversed range is swapped (90-80)" 11 "$HOST" -p 90-80
expect_scan "trailing text ignored (80abc -> 80)" 1 "$HOST" -p 80abc
expect_scan "extra range part ignored (1-2-3 -> 1-2)" 2 "$HOST" -p 1-2-3
expect_scan "above 65535 is clamped (65536 -> 65535)" 1 "$HOST" -p 65536
expect_scan "leading minus ignored (-5 -> 5)" 1 "$HOST" -p -5
expect_scan "-p with no value uses the default range" "$DEFAULT_RANGE_COUNT" "$HOST" -p

echo "== help =="
expect_help "--help" --help
expect_help "-h" -h
expect_help "target then -h" "$HOST" -h
expect_help "-h then target" -h "$HOST"

echo "== bad input is rejected and never scans =="
expect_reject "no arguments"
expect_reject "invalid IP (999.1.1.1)" 999.1.1.1
expect_reject "not an IP (abc)" abc
expect_reject "unknown short option (-x)" "$HOST" -x
expect_reject "unknown long option (--foo)" "$HOST" --foo
expect_reject "port value 0" "$HOST" -p 0
expect_reject "port value abc" "$HOST" -p abc
expect_reject "empty port value" "$HOST" -p ""
expect_reject "two targets" "$HOST" 127.0.0.2 -p 80    # delete if you add support
expect_reject "option used as a port value (-p -p)" "$HOST" -p -p
expect_reject "option used as a port value (-p -h)" "$HOST" -p -h
expect_reject "junk argument after a valid option" "$HOST" -p 80 abc
expect_reject "unknown option after a valid one" "$HOST" -p 80 -x
expect_reject "unknown option before a valid one" "$HOST" -x -p 80

echo "== several options =="
expect_help "-h after another option" "$HOST" -p 80 -h
expect_help "--help after another option" "$HOST" -p 80 --help
# Two VALID port options in one command (-p 80 -p 90-91). This is a different case
# from "-p -p", where the second option is used as the VALUE of the first one:
# that is rejected (see the "bad input" group) whatever you choose here.
case "$REPEAT_P" in
union)
    expect_scan "-p twice: ports from both are scanned" 3 "$HOST" -p 80 -p 90-91
    expect_scan "-p then --port: ports from both are scanned" 3 "$HOST" -p 80 --port 81-82
    expect_scan "-p twice with the same port counts it once" 1 "$HOST" -p 80 -p 80
    expect_scan "a bare -p after a port uses the default range" "$DEFAULT_RANGE_COUNT" "$HOST" -p 80 -p
    ;;
replace)
    expect_scan "-p twice: the last one wins" 2 "$HOST" -p 80 -p 90-91
    expect_scan "-p then --port: the last one wins" 2 "$HOST" -p 80 --port 81-82
    expect_scan "a bare -p after a port uses the default range" "$DEFAULT_RANGE_COUNT" "$HOST" -p 80 -p
    ;;
reject)
    expect_reject "-p given twice is an error" "$HOST" -p 80 -p 90-91
    expect_reject "-p then --port is an error" "$HOST" -p 80 --port 81-82
    expect_reject "-p twice with the same port is an error" "$HOST" -p 80 -p 80
    ;;
*)
    echo "  skip  unknown REPEAT_P='$REPEAT_P' (use union, replace or reject)"
    ;;
esac
if [ -n "$NO_P_COUNT" ]; then
    expect_scan "no -p: default range is used" "$NO_P_COUNT" "$HOST"
else
    echo "  skip  no -p default (set NO_P_COUNT once you decide the default range)"
fi

echo "== port lists (strict rules - edit a line if you decide differently) =="
expect_scan "two single ports"                          2  "$HOST" -p 22,80
expect_scan "singles and a range mixed"                 5  "$HOST" -p 22,80-82,443
expect_scan "two ranges"                                6  "$HOST" -p 80-82,85-87
expect_scan "range ending at 65535"                     6  "$HOST" -p 65530-65535
expect_scan "list with a range ending at 65535"         7  "$HOST" -p 22,65530-65535
expect_scan "order does not matter"                     5  "$HOST" -p 443,80-82,22
expect_scan "duplicate port counts once"                1  "$HOST" -p 22,22
expect_scan "overlapping ranges merge"                 11  "$HOST" -p 20-30,25
expect_scan "a port inside a range counts once"         3  "$HOST" -p "$PORT_A-$PORT_C,$PORT_B"
expect_scan "lowest and highest port in one list"       2  "$HOST" -p 1,65535
run_scan "$HOST" -p "$PORT_A,$PORT_C"
expect "list finds both listeners" 2 2 0
if [ "$EMPTY_ITEMS" = "reject" ]; then
    expect_reject "empty item in the middle (22,,80)"   "$HOST" -p 22,,80
    expect_reject "trailing comma (22,)"                "$HOST" -p 22,
    expect_reject "leading comma (,22)"                 "$HOST" -p ,22
else
    expect_scan "empty item is skipped (22,,80)"      2 "$HOST" -p 22,,80
    expect_scan "trailing comma is skipped (22,)"     1 "$HOST" -p 22,
    expect_scan "leading comma is skipped (,22)"      1 "$HOST" -p ,22
fi
if [ "$REVERSED_RANGE" = "swap" ]; then
    expect_scan "reversed range in a list is swapped (90-80,22)" 12 "$HOST" -p 90-80,22
else
    expect_reject "reversed range in a list (90-80,22)" "$HOST" -p 90-80,22
fi
expect_reject "port 0 in a list (0,22)"                 "$HOST" -p 0,22
expect_reject "port above 65535 in a list (70000,80)"   "$HOST" -p 70000,80
expect_reject "negative port in a list (-5,22)"         "$HOST" -p -5,22
expect_reject "open-ended range in a list (80-,22)"     "$HOST" -p 80-,22
if [ "$EXTRA_RANGE_PART" = "reject" ]; then
    expect_reject "range with 3 numbers (80-90-100,22)" "$HOST" -p 80-90-100,22
else
    expect_scan "third number of a range is ignored (80-90-100,22)" 12 "$HOST" -p 80-90-100,22
fi
expect_reject "junk after a number in a list (22,80abc)" "$HOST" -p 22,80abc
expect_reject "--port followed by an option (--port -p)" "$HOST" --port -p

echo "== timeout option (--timeout <ms>) =="
expect_scan "--timeout with a valid value"                1 "$HOST" -p 80 --timeout 500
expect_scan "--timeout before the other options"          1 "$HOST" --timeout 500 -p 80
expect_scan "--timeout alone keeps the default range"     "$DEFAULT_RANGE_COUNT" "$HOST" --timeout 500
expect_scan "smallest accepted timeout ($TIMEOUT_MIN ms)" 1 "$HOST" -p 80 --timeout "$TIMEOUT_MIN"
expect_scan "largest accepted timeout ($TIMEOUT_MAX ms)"  1 "$HOST" -p 80 --timeout "$TIMEOUT_MAX"
expect_reject "below the smallest timeout"     "$HOST" -p 80 --timeout "$((TIMEOUT_MIN - 1))"
expect_reject "above the largest timeout"      "$HOST" -p 80 --timeout "$((TIMEOUT_MAX + 1))"
expect_reject "--timeout with no value"        "$HOST" -p 80 --timeout
expect_reject "--timeout abc"                  "$HOST" -p 80 --timeout abc
expect_reject "--timeout 12abc (trailing junk)" "$HOST" -p 80 --timeout 12abc
expect_reject "--timeout -5 (negative)"        "$HOST" -p 80 --timeout -5
expect_reject "--timeout 1.5 (decimal)"        "$HOST" -p 80 --timeout 1.5
expect_reject "--timeout with an empty value"  "$HOST" -p 80 --timeout ""
expect_reject "--timeout too big for a long (ERANGE)" "$HOST" -p 80 --timeout 99999999999999999999
expect_reject "--timeout followed by an option (--timeout -p)" "$HOST" --timeout -p 80
expect_reject "-p followed by --timeout"       "$HOST" -p --timeout 500
if [ "$REPEAT_TIMEOUT" = "reject" ]; then
    expect_reject "--timeout given twice is an error"   "$HOST" -p 80 --timeout 100 --timeout 200
else
    expect_scan "--timeout given twice: the last one wins" 1 "$HOST" -p 80 --timeout 100 --timeout 200
fi
run_scan --help
if grep -qi "timeout" <<<"$OUT"; then pass "help mentions --timeout"; else fail "help mentions --timeout" "the word 'timeout' is not in the help text"; fi

# The option must really change how long an unanswered probe waits.
# 192.0.2.1 never answers; on a network without a route the probe fails at once
# and these three checks are skipped.
timing_check() {   # timing_check <name> <min-seconds> <max-seconds> <args...>
    local name=$1 lo=$2 hi=$3
    shift 3
    run_scan 192.0.2.1 -p 80 "$@"
    if [ "${F:-0}" != "1" ]; then
        echo "  skip  $name (192.0.2.1 did not time out here: filtered=${F:-0})"
    elif awk -v t="$ELAPSED" -v lo="$lo" -v hi="$hi" 'BEGIN { exit !(t >= lo && t <= hi) }'; then
        pass "$name (${ELAPSED}s)"
    else
        fail "$name" "took ${ELAPSED}s, wanted between $lo and $hi s"
    fi
}
timing_check "default timeout is about 300 ms"  0.25 0.9
timing_check "--timeout 1000 waits about 1 s"   0.9  1.7  --timeout 1000
timing_check "--timeout 50 gives up quickly"    0.0  0.25 --timeout 50

echo "== robustness =="
# 1. a list must not need megabytes of stack (4 MB is below the usual 8 MB default)
if command -v prlimit >/dev/null; then
    RUNNER="prlimit --stack=4194304"
    run_scan "$HOST" -p 22,80
    RUNNER=""
    expect "a list works with a 4 MB stack limit" 2 - -
else
    echo "  skip  (prlimit not available)"
fi
# 2. big inputs
LONG_LIST=$(seq -s, 1 3000)
expect_scan "a list of 3000 different ports" 3000 "$HOST" -p "$LONG_LIST"
DUP_LIST=$(yes 22 | head -n 20000 | paste -sd, -)
expect_scan "20000 copies of one port count once" 1 "$HOST" -p "$DUP_LIST"
HUGE=$(printf '9%.0s' $(seq 1 5000))
expect_reject "a 5000-digit number" "$HOST" -p "$HUGE"
expect_reject "a 5000-digit number inside a list" "$HOST" -p "22,$HUGE"
LONG_OPT=$(printf 'a%.0s' $(seq 1 5000))
expect_reject "a 5000-character option name" "$HOST" "--$LONG_OPT"
# 3. both ends of the port space inside one list
expect_scan "ends of the port space in one list" 6 "$HOST" -p 1-3,65533-65535
# 4. results come out in ascending order, whatever order the user typed
run_scan "$HOST" -p "$PORT_C,$PORT_A"
raw_order=$(grep -oE 'port [0-9]+ is open' <<<"$OUT" | awk '{print $2}' | tr '\n' ' ')
if [ "$raw_order" = "$PORT_A $PORT_C " ]; then
    pass "open ports are listed in ascending order"
else
    fail "open ports are listed in ascending order" "got [${raw_order% }]"
fi
# 5. a list mixed with a range, checked against ss
run_scan "$HOST" -p "$SCAN_LO-$SCAN_HI,$PORT_C"
expect "range plus a port inside it adds up" 10 - -
want=$(ss_open_ports "$SCAN_LO" "$SCAN_HI")
got=$(scanner_open_ports)
if [ "$want" = "$got" ]; then
    pass "list open ports match ss  [${got% }]"
else
    fail "list open ports match ss" "ss says [${want% }] but scanner says [${got% }]"
fi
# 6. the whole port space (slow)
if [ "$FULL_RANGE" = "1" ]; then
    RUN_TIMEOUT=180
    expect_scan "every port from 1 to 65535" 65535 "$HOST" -p 1-65535
    RUN_TIMEOUT=${ORIG_RUN_TIMEOUT:-30}
else
    echo "  skip  full range 1-65535 (FULL_RANGE=0)"
fi

echo "== ground truth (ss) =="
run_scan "$HOST" -p "$SCAN_LO-$SCAN_HI"
expect "10-port range completes and adds up" 10 - -
want=$(ss_open_ports "$SCAN_LO" "$SCAN_HI")
got=$(scanner_open_ports)
if [ "$want" = "$got" ]; then
    pass "open ports match ss  [${got% }]"
else
    fail "open ports match ss" "ss says [${want% }] but scanner says [${got% }]"
fi

echo "== no response (filtered / timeout path) =="
# 192.0.2.1 is reserved for documentation: nothing should ever answer.
run_scan 192.0.2.1 -p 80
expect "unanswered port: scanned 1, none open" 1 0 -
awk -v t="$ELAPSED" 'BEGIN { exit !(t < 5) }' \
    && pass "unanswered port finishes quickly (${ELAPSED}s)" \
    || fail "unanswered port finishes quickly" "took ${ELAPSED}s"

echo "== no route (unreachable path) =="
if unshare -rn true 2>/dev/null; then       # needs unprivileged user namespaces
    RUNNER="unshare -rn"
    run_scan 192.0.2.1 -p 80-82
    RUNNER=""
    expect "no route: scan completes and adds up" 3 0 0
    [ "$U" = "3" ] \
        && pass "no route: all 3 ports counted unreachable" \
        || fail "no route: all 3 ports counted unreachable" "unreachable=${U:-missing}"
else
    echo "  skip  (unshare -rn is not available on this system)"
fi

echo "== file descriptor leaks =="
# With only 16 descriptors allowed, scanning 300 ports works only if every
# socket is closed before the next probe. A leak shows up as errors or a
# summary that does not add up.
if command -v prlimit >/dev/null; then
    RUNNER="prlimit --nofile=16"
    run_scan "$HOST" -p 1-300
    RUNNER=""
    expect "300 ports under a 16-descriptor limit (no socket leak)" 300 - -
else
    echo "  skip  (prlimit not available)"
fi

echo "== socket() failure =="
# A tiny shared library makes every socket() call fail with EMFILE, so the
# "probe could not even start" path is exercised. Every port must still be
# counted somewhere. Decision encoded here: the scan keeps going and exits 0.
SHIM_DIR=$(mktemp -d)
cat >"$SHIM_DIR/fail_socket.c" <<'SHIM'
#define _GNU_SOURCE
#include <errno.h>
#include <sys/socket.h>
int socket(int domain, int type, int protocol)
{
    (void)domain; (void)type; (void)protocol;
    errno = EMFILE;
    return -1;
}
SHIM
if command -v gcc >/dev/null &&
   gcc -shared -fPIC -o "$SHIM_DIR/fail_socket.so" "$SHIM_DIR/fail_socket.c" 2>/dev/null; then
    RUNNER="env LD_PRELOAD=$SHIM_DIR/fail_socket.so ASAN_OPTIONS=verify_asan_link_order=0:detect_leaks=0"
    run_scan "$HOST" -p 80-82
    RUNNER=""
    expect "socket() failing: scan completes and every port is counted" 3 0 0
else
    echo "  skip  (could not build the socket() shim; gcc needed)"
fi
rm -rf "$SHIM_DIR"

echo "== listener goes away =="
stop_listener "$PORT_C"
run_scan "$HOST" -p "$PORT_C"
expect "stopped listener is now closed" 1 0 1

# ---------------------------------------------------------------- result

echo
echo "passed: $PASS   failed: $FAIL"
[ "$FAIL" -eq 0 ]
