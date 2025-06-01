CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/include/cjson
LDFLAGS = -lcjson

OBJS = main.o config_parser.o animator.o anim_utils.o

test_anim: $(OBJS)
	$(CC) -o test_anim $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o test_anim