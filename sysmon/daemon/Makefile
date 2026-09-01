CC ?= gcc

CFLAGS := -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
CFLAGS += -I../include

TARGET := sysmond

.PHONY: all clean

all: $(TARGET)

$(TARGET): sysmond.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
