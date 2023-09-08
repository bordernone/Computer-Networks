#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <ctype.h>


struct UDPHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
};

struct UDPPacket {
    struct UDPHeader header;
    unsigned char *data;
};

long int fileSize(FILE *fp) {
    long int current = ftell(fp);
    if (fseek(fp, 0, SEEK_END) != 0) {
        printf("Error seeking to end of file\n");
        exit(1);
    }
    long int size = ftell(fp);
    if (fseek(fp, current, SEEK_SET) != 0) {
        printf("Error seeking to current position\n");
        exit(1);
    }
    return size;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <pcap_file>\n", argv[0]);
        exit(1);
    }
    const char *pcap_file = argv[1];
    FILE *fp = fopen(pcap_file, "rb");
    if (fp == NULL) {
        printf("Error opening file %s\n", pcap_file);
        exit(1);
    }

    if (fseek(fp, 24, SEEK_SET) != 0) {
        printf("Error seeking to offset 24\n");
        exit(1);
    }

    long int size = fileSize(fp);
    while (ftell(fp) < size) {
        if (fseek(fp, 50, SEEK_CUR) != 0) {
            printf("Error seeking to offset 16\n");
            exit(1);
        }

        struct UDPPacket *packet = malloc(sizeof(struct UDPPacket));
        if (packet == NULL) {
            printf("Error allocating memory for packet\n");
            exit(1);
        }
        if (fread(&(packet->header), sizeof(struct UDPHeader), 1, fp) != 1) {
            printf("Error reading packet header\n");
            exit(1);
        }
        packet->header.src_port = ntohs(packet->header.src_port);
        packet->header.dst_port = ntohs(packet->header.dst_port);
        packet->header.len = ntohs(packet->header.len);
        packet->header.checksum = ntohs(packet->header.checksum);
        printf("Src Port: %u\n", packet->header.src_port);
        printf("Dest Port: %u\n", packet->header.dst_port);
        printf("UDP Packet Length: %u\n", packet->header.len);
        printf("Checksum: 0x%X\n", packet->header.checksum);
        long int dataSize = packet->header.len - sizeof(struct UDPHeader);
        packet->data = malloc(dataSize);
        if (packet->data == NULL) {
            printf("Error allocating memory for packet data\n");
            exit(1);
        }
        if (fread(packet->data, dataSize, 1, fp) != 1) {
            printf("Error reading packet data\n");
            exit(1);
        }
        for (int i = 0; i < dataSize; i++) {
            printf("%c", isprint(packet->data[i])? packet->data[i] : '.');
        }
        printf("\n ---------------------------- \n");
        free(packet->data);
        free(packet);
    }

    fclose(fp);
    return 0;
}