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

test: moscar_asan
	BIN=./moscar_asan bash run_tests.sh

lint:
	@for f in $(SRCS); do \
		$(CC) $(WARN) -O2 -Wmaybe-uninitialized -fanalyzer -c $$f -o /dev/null || exit 1; \
	done

clean:
	rm -f *.o *.d moscar moscar_asan

-include $(SRCS:.c=.d) $(SRCS:.c=.asan.d)

.PHONY: test lint clean
