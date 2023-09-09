#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <ctype.h>

#define GLOBAL_HEADER_SIZE 24
#define RECORD_HEADER_SIZE 16
#define ETHERNET_HEADER_SIZE 14
#define IP_HEADER_SIZE 20

// Define the structure for the UDP header
// Adapted from https://sites.uclouvain.be/SystInfo/usr/include/netinet/udp.h.html
struct UDPHeader {
    uint16_t src_port;    // Source port number
    uint16_t dst_port;    // Destination port number
    uint16_t len;         // Length of the UDP packet (header + data)
    uint16_t checksum;    // Checksum
};

// Define the structure for a UDP packet that includes the header and data
struct UDPPacket {
    struct UDPHeader header;    // UDP header
    unsigned char *data;        // UDP data
};

// Function to get the size of a file, without changing the file position
long int fileSize(FILE *fp) {
    long int current = ftell(fp); // Copy the current file position
    // Seek to the end of the file and get the position
    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error seeking to end of file\n");
        exit(1);
    }
    long int size = ftell(fp); // Copy the file position as the size

    // Seek back to the original position
    if (fseek(fp, current, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking to current position\n");
        exit(1);
    }
    return size;
}

int main(int argc, char *argv[]) {
    // Check if the correct number of command-line arguments are provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pcap_file>\n", argv[0]);
        exit(1);
    }

    // Get the pcap file name from the command line
    const char *pcap_file = argv[1];

    // Open the pcap file for reading in binary mode
    FILE *fp = fopen(pcap_file, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Error opening file %s\n", pcap_file);
        exit(1);
    }

    // Skip the global header
    if (fseek(fp, GLOBAL_HEADER_SIZE, SEEK_SET) != 0) {
        fprintf(stderr, "Error seeking to offset %d\n", GLOBAL_HEADER_SIZE);
        exit(1);
    }

    // Get the total size of the file
    long int size = fileSize(fp);

    // Loop through the file until the end is reached
    while (ftell(fp) < size) {
        fprintf(stderr, "\n----------------------------\n");

        // Skip the Ethernet and IP headers to reach the UDP header
        if (fseek(fp, RECORD_HEADER_SIZE + ETHERNET_HEADER_SIZE + IP_HEADER_SIZE, SEEK_CUR) != 0) {
            fprintf(stderr, "Error seeking to offset %d\n", RECORD_HEADER_SIZE + ETHERNET_HEADER_SIZE + IP_HEADER_SIZE);
            exit(1);
        }

        // Allocate memory for a UDP packet
        struct UDPPacket *packet = malloc(sizeof(struct UDPPacket));
        if (packet == NULL) {
            fprintf(stderr, "Error allocating memory for packet\n");
            exit(1);
        }

        // Read the UDP header from the file
        if (fread(&(packet->header), sizeof(struct UDPHeader), 1, fp) != 1) {
            fprintf(stderr, "Error reading packet header\n");
            exit(1);
        }

        // Convert header fields from network byte order to host byte order
        packet->header.src_port = ntohs(packet->header.src_port);
        packet->header.dst_port = ntohs(packet->header.dst_port);
        packet->header.len = ntohs(packet->header.len);
        packet->header.checksum = ntohs(packet->header.checksum);

        // Print UDP header information
        printf("Src Port: %u\n", packet->header.src_port);
        printf("Des Port: %u\n", packet->header.dst_port);
        printf("UDP Packet Length: %u\n", packet->header.len);
        printf("Checksum: 0x%X\n", packet->header.checksum);

        // Calculate the size of the UDP data
        long int dataSize = packet->header.len - sizeof(struct UDPHeader);

        // Allocate memory for the UDP data
        packet->data = malloc(dataSize);
        if (packet->data == NULL) {
            fprintf(stderr, "Error allocating memory for packet data\n");
            exit(1);
        }

        // Read the UDP data from the file
        if (fread(packet->data, dataSize, 1, fp) != 1) {
            fprintf(stderr, "Error reading packet data\n");
            exit(1);
        }

        // Print the UDP data as characters, replacing non-printable characters with '.'
        for (int i = 0; i < dataSize; i++) {
            printf("%c", isprint(packet->data[i]) ? packet->data[i] : '.'); // isprint() checks if the character is printable
        }

        printf("\n----------------------------\n");

        // Free allocated memory for data and packet
        free(packet->data);
        free(packet);
    }

    // Close the file
    fclose(fp);

    return 0;
}
