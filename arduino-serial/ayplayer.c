#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "arduino-serial-lib.h"


void error(char* msg)
{
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
        if (argc < 3) {
                fprintf(stderr, "\nUsage: %s /dev/ttyUSBXX filename.ay3\n\n", argv[0]);
                return EXIT_FAILURE;
        }

        int fd = serialport_init(argv[1], 500000);
        if (fd <= 0) {
                error("Could not init serial port.");
        }
        printf("Waiting...\n");
        usleep(3000000);        
        FILE *song = fopen(argv[2], "r");
        uint8_t byte = 0;
        byte = 'P';
        int res;

        while (write(fd, &byte, 1) != 1);
        for (int retry = 0; retry < 5; retry++) {
                usleep(1000);
                res = read(fd, &byte, 1); 
                if (res < 1) {
                        printf("Got nothing\n");
                        continue;
                }
                if (byte != 1) {
                        printf("Got %u\n", byte);
                        printf("error, didn't get block request\n");
                } else {
                        printf("Got 1\n");
                        break;
                }
        }

        uint8_t buffer[193208];
        size_t len = 0;
        int request_len = 0;
        do {
                int retries = 5;
                /* wait for arduino to tell us how many bytes */
                do {
                        while (read(fd, &byte, 1) < 1) {
                                usleep(100);
                        }
                        if (byte) break;
                        usleep(10000);
                } while (byte == 0 && retries--);
                if (byte == 0) {
                        break;
                }
                request_len = fread(buffer, 1, byte, song);
                if (!request_len) {
                        break;
                }
                int sent = 0;
                do {
                        res = write(fd, buffer + sent, request_len - sent);
                        if (res > 0) {
                                printf("Wrote %u bytes to serial\n", res);
                                sent += res;
                        }
                } while (sent < request_len);
        } while (true);
        /* get confirmation from device */

        fclose(song);
        char *a[5];
        serialport_close(fd);


        return EXIT_SUCCESS;
}

