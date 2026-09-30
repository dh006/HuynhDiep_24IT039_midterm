CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

TARGET = myls

SRC = src/main.c \
      src/listing.c \
      src/options.c \
      src/sort.c \
      src/display.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
