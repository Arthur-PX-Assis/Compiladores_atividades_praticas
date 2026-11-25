
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -g
TARGET = compilador
SRCS = main.c csmall.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRCS) csmall.h
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) testes/programa_tac.csmall programa_tac.tac

clean:
	rm -f $(TARGET) programa_tac.tac teste_completo.tac