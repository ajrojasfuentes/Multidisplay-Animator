###############################################################################
# Makefile para compilar la biblioteca “mypthreads” (línea base) y "test.c"
#   - Crea mypthread.o → libmypthread.a
#   - Compila test.o y lo enlaza con libmypthread.a
#   - Genera el ejecutable "test"
###############################################################################

CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra
AR      := ar
ARFLAGS := rcs

.PHONY: all clean

all: libmypthread.a test

# 1) Compilar mypthread.c a objeto
mypthread.o: mypthread.c mypthread.h
	$(CC) $(CFLAGS) -c mypthread.c

# 2) Crear la librería estática
libmypthread.a: mypthread.o
	$(AR) $(ARFLAGS) libmypthread.a mypthread.o

# 3) Compilar test.c a objeto
test.o: test.c mypthread.h
	$(CC) $(CFLAGS) -c test.c

# 4) Enlazar test.o con la librería estática
test: test.o libmypthread.a
	$(CC) $(CFLAGS) test.o -L. -lmypthread -o test

# 5) Limpiar archivos objeto y binarios
clean:
	rm -f *.o libmypthread.a test

