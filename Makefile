CC = gcc
CFLAGS = -Wall -I/usr/include/openssl
LDFLAGS = -lssl -lcrypto -lcurl -lm

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))
TARGET = crymail

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) 