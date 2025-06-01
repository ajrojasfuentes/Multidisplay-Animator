CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/include/cjson -I./lib
LDFLAGS = -lcjson

OBJS = main.o config_parser.o animator_mt.o anim_utils.o lib/mypthread.o

test_anim: $(OBJS)
	$(CC) -o test_anim $(OBJS) $(LDFLAGS)

clean:
	rm -f *.o lib/*.o test_anim