CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -I../lib
LDFLAGS = -L../lib -lmypthread

TESTS = test_rr_basic test_mutex_counter test_lottery_fairness \
        test_rt_priority test_switch_sched

all: $(TESTS)

%: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

run_%: %
	@echo \"\\n>>> Ejecutando $<\"; ./< $<

clean:
	rm -f $(TESTS)
