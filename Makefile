CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I/usr/include/cjson
LDLIBS = -lcjson

OBJS = main.o config_parser.o

all: test_config

test_config: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

main.o: main.c config_parser.h anim_config.h
config_parser.o: config_parser.c config_parser.h anim_config.h

clean:
	rm -f *.o test_config