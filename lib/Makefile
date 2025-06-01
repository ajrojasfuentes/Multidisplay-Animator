CC      := gcc
CFLAGS  := -std=gnu11 -Wall -Wextra -O2
AR      := ar rcs

# Fuentes de la biblioteca
SRCS_LIB := queues.c scheduler_rr.c scheduler_lottery.c scheduler_rt.c mypthread.c
OBJS_LIB := $(SRCS_LIB:.c=.o)
LIB      := libmypthread.a

# Programa de prueba
SRC_TEST := test.c
EXE      := test_prog

all: $(LIB) $(EXE)

# Regla para crear la biblioteca estática
$(LIB): $(OBJS_LIB)
	$(AR) $@ $^

# Compilar cada .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar y enlazar el programa de prueba
$(EXE): $(SRC_TEST) $(LIB)
	$(CC) $(CFLAGS) $(SRC_TEST) -L. -lmypthread -lpthread -o $@

# Limpiar objetos y ejecutables
clean:
	rm -f $(OBJS_LIB) $(LIB) $(EXE)

.PHONY: all clean
