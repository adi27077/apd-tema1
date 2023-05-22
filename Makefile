CC = g++
CFLAGS = -Wall -Werror -pthread
SRC = tema1.cpp
OUT = tema1

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

run:
	./$(OUT)

.PHONY: clean

clean:
	rm -f $(OUT)
