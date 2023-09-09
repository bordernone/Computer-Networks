# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Werror

all: read_pcap.out

read_pcap.out: main.c
	$(CC) $(CFLAGS) -o read_pcap.out main.c

clean:
	rm -f read_pcap.out

.PHONY: all clean
