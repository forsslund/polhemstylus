/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2020.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

//#include "ud_ucase.h"
#include <sys/un.h>
#include <sys/socket.h>
//#include <ctype.h>
//#include "tlpi_hdr.h"

//#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
//#include <string.h>     /* Commonly used string-handling functions */



#define BUF_SIZE 10             /* Maximum size of messages exchanged
                                   between client and server */

#define SV_SOCK_PATH "/tmp/ud_ucase"

void errExit(const char *str) { printf("%s",str); exit(1);}
void fatal(const char *str) { printf("%s",str);  exit(1);}
void usageErr(const char *str) { printf("%s",str); exit(1);}










int
main(int argc, char *argv[])
{
    struct sockaddr_un svaddr, claddr;
    int sfd;
    size_t msgLen;
    ssize_t numBytes;
    u_int8_t resp[BUF_SIZE];

    //if (argc < 2 || strcmp(argv[1], "--help") == 0)
    //    usageErr("./datagram_client msg...\n");

    /* Create client socket; bind to unique pathname (based on PID) */

    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1)
        errExit("socket");

    memset(&claddr, 0, sizeof(struct sockaddr_un));
    claddr.sun_family = AF_UNIX;
    snprintf(claddr.sun_path, sizeof(claddr.sun_path),
            "/tmp/ud_ucase_cl.%ld", (long) getpid());

    if (bind(sfd, (struct sockaddr *) &claddr, sizeof(struct sockaddr_un)) == -1)
        errExit("bind");

    /* Construct address of server */

    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SV_SOCK_PATH, sizeof(svaddr.sun_path) - 1);

    /* Send messages to server; echo responses on stdout */

    //for (j = 1; j < argc; j++) {
        msgLen = 3;//strlen(argv[j]);       /* May be longer than BUF_SIZE */
        if (sendto(sfd, "get", msgLen, 0, (struct sockaddr *) &svaddr,
                sizeof(struct sockaddr_un)) != msgLen)
            fatal("Server down?\n");

        numBytes = recvfrom(sfd, resp, 2, 0, NULL, NULL);
        if (numBytes == -1)
            errExit("recvfrom");
        //printf("Response %d: %.*s\n", j, (int) numBytes, resp);
        printf("Response %02x %02x \n", resp[0],resp[1]);
    //}

    remove(claddr.sun_path);            /* Remove client socket pathname */
    exit(EXIT_SUCCESS);
}
