CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = ffs_simulation
DISK_IMAGE = disk.img

all: $(TARGET) $(DISK_IMAGE)

$(TARGET): fs.c
	$(CC) $(CFLAGS) -o $(TARGET) fs.c

$(DISK_IMAGE):
	dd if=/dev/zero of=$(DISK_IMAGE) bs=1M count=5000

clean:
	rm -f $(TARGET) $(DISK_IMAGE)

.PHONY: all clean
