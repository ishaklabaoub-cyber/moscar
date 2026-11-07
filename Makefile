CC   = gcc
WARN = -Wall -Wextra -Wpedantic -Wswitch-enum

# every .c in this folder except unit tests (test_*.c)
SRCS  = $(filter-out test_%.c,$(wildcard *.c))
OBJS  = $(SRCS:.c=.o)
AOBJS = $(SRCS:.c=.asan.o)

moscar: $(OBJS)
	$(CC) $^ -o $@

moscar_asan: $(AOBJS)
	$(CC) -fsanitize=address,undefined $^ -o $@

%.o: %.c
	$(CC) $(WARN) -O2 -MMD -MP -c $< -o $@

%.asan.o: %.c
	$(CC) $(WARN) -g -fsanitize=address,undefined -MMD -MP -c $< -o $@

# unit tests for the port set (needs test_ports.c and ports.c)
unit: test_ports.c ports.c ports.h
	$(CC) $(WARN) -g -fsanitize=address,undefined test_ports.c ports.c -o test_ports
	timeout 60 ./test_ports

# everything: static checks, unit tests, end-to-end tests
check: lint unit test

test: moscar_asan
	BIN=./moscar_asan bash run_tests.sh

lint:
	@for f in $(SRCS); do \
		$(CC) $(WARN) -O2 -Wmaybe-uninitialized -fanalyzer -c $$f -o /dev/null || exit 1; \
	done

clean:
	rm -f *.o *.d moscar moscar_asan test_ports

-include $(SRCS:.c=.d) $(SRCS:.c=.asan.d)

.PHONY: test unit check lint clean
