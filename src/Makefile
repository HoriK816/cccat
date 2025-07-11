# Makefile

TARGET = neko
SRC = neko.c
BIN_DIR = ../bin

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(SRC)
	mkdir -p $(BIN_DIR)
	gcc -o $(BIN_DIR)/$(TARGET) $(SRC)

clean:
	rm -f $(BIN_DIR)/$(TARGET)

