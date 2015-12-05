EXEC     = cuppa
CC       = gcc -fdiagnostics-color=always

CFLAGS   = -std=c99 -O3 -Wall -Wextra -Wpedantic -Wstrict-aliasing
CFLAGS  += $(shell pkg-config --cflags glib-2.0 gio-2.0)
LDFLAGS  = $(shell pkg-config --libs   glib-2.0 gio-2.0)

SRC      = $(wildcard src/*.c)
OBJ      = $(SRC:.c=.o)

all: $(EXEC)

${EXEC}: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean mrproper

clean:
	@rm -rf src/*.o

mrproper: clean
	@rm -rf $(EXEC)
			
