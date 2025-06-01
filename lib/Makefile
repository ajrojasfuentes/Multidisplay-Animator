CC      := gcc
CFLAGS  := -std=gnu11 -Wall -Wextra -O2
AR      := ar rcs

# Fuentes de la biblioteca
SRCS_LIB := queues.c scheduler_rr.c scheduler_lottery.c scheduler_rt.c mypthread.c
OBJS_LIB := $(SRCS_LIB:.c=.o)
LIB      := libmypthread.a

# Ejecutables de prueba
EXE_PTHREAD      := pthread_test
EXE_MYPTHREAD    := mypthread_test

all: $(LIB) $(EXE_PTHREAD) $(EXE_MYPTHREAD)

# Crear la biblioteca estática libmypthread.a
$(LIB): $(OBJS_LIB)
	$(AR) $@ $^

# Compilar cada .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar pthread_test
$(EXE_PTHREAD): pthread_test.c
	$(CC) $(CFLAGS) pthread_test.c -pthread -o $@

# Compilar mypthread_test (necesita libmypthread.a)
$(EXE_MYPTHREAD): mypthread_test.c $(LIB)
	$(CC) $(CFLAGS) mypthread_test.c -L. -lmypthread -o $@

# Limpiar archivos intermedios y ejecutables
clean:
	rm -f $(OBJS_LIB) $(LIB) $(EXE_PTHREAD) $(EXE_MYPTHREAD)

.PHONY: all clean
