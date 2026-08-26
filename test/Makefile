CC := $(abspath ../buildroot/output/host/bin/aarch64-linux-gcc)

CFLAGS := -Wall -Wextra -std=c11 -I../sysmon

SRCS := $(wildcard test_*.c)
TARGETS := $(SRCS:.c=)

.PHONY: all clean

all: $(TARGETS)

%.: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)
