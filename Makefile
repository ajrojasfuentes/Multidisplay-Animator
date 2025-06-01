# Nombre del ejecutable
TARGET = test

# Archivos fuente
SRC = main.c mypthread.c

# Flags de compilación
CFLAGS = -Wall -Wextra -std=c99

# Compilador
CC = gcc

# Regla por defecto
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Limpiar binarios y archivos objeto
clean:
	rm -f $(TARGET)