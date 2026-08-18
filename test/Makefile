CC := $(abspath ../buildroot/output/host/bin/aarch64-linux-gcc)

CFLAGS := -Wall -Wextra -std=c11 -I../sysmon

TARGET := test_sysmon
SRC := test_sysmon.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
