/**
** serverC.c
** BASED ON DATAGRAM listener.c in Beej's Guide to Network Programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#define IPADDRESS "127.0.0.1" // local IP address
#define MYPORT "21456"        // the port used for UDP connection with serverM
#define SERVERMPORT "24456"   // the UDP port of serverM
#define MAXBUFLEN 4000
#define MAX_LINE 1024 // txt document read
#define MAX_STU 50

struct STU_INFO
{
    char enc_name[20];
    char enc_pw[30];
};

// from Beej
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// separate the received messages and store them in an array
void split_args(char *args[], char *message)
{
    char *p = strtok(message, " ");
    int i = 0;
    while (p != NULL)
    {
        args[i++] = p;
        p = strtok(NULL, " ");
    }
}

int main(void)
{
    // for documents
    char txtbuf[MAX_LINE];
    FILE *fp;   // file pointer
    int txtlen; // number of line characters
    int stunum; // the number of accounts in txt
    struct STU_INFO stuinfo[MAX_STU];

    // for udp connection
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    char buf[MAXBUFLEN];
    char data[MAXBUFLEN];
    char *args[2];

    /*********************INPUT TXT FILE*****************************/
    if ((fp = fopen("cred.txt", "r")) == NULL)
    {
        perror("fail to read");
        exit(1);
    }
    else
    {
        int i = 0;
        while (fgets(txtbuf, MAX_LINE, fp) != NULL)
        {

            // remove line breaks except the last line
            txtlen = strlen(txtbuf);
            if (txtbuf[txtlen - 1] == '\n')
            {
                txtbuf[txtlen - 1] = '\0';
                txtbuf[txtlen - 2] = '\0';
                // printf("txtbuf: %s\n", txtbuf);
            }
            // split txtbuf and load two values in stuinfo struct
            char *p = strtok(txtbuf, ",");
            int j = 0;
            while (p != NULL)
            {
                if (j == 0)
                    strcpy(stuinfo[i].enc_name, p);
                if (j == 1)
                    strcpy(stuinfo[i].enc_pw, p);
                j++;
                p = strtok(NULL, ",");
            }
            // printf("test:splited name:%s, splited pw:%s\n", stuinfo[i].enc_name, stuinfo[i].enc_pw);

            i++;
        }
        stunum = i;
    }

    /*********************UDP CONNECTION*****************************/
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // get information of backend server itself
    if ((rv = getaddrinfo(IPADDRESS, MYPORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    addr_len = sizeof their_addr;

    printf("The Server C is up and running using UDP on port %s.\n", MYPORT);
    // fflush(stdout);

    while (1)
    {
        /****************UDP: RECEIVE DATA************************/
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }
        else
            printf("The ServerC received an authentication request from the Main Server.\n");
        buf[numbytes] = '\0';

        // split messages
        split_args(args, buf);

        // printf("The Server C has received user information: %s, %s\n", args[0], args[1]);

        strcpy(data, "Before authentication.");

        /****************AUTHENTICATION************************/
        // send FAIL_NO_USER to serverM: fail, no username.
        // send FAIL_PASS_NO_MATCH to serverM: fail, password does not match.
        // send PASS to serverM: pass.
        int k = 0;
        for (k = 0; k < stunum; k++)
        {
            if (strcmp(args[0], stuinfo[k].enc_name) == 0)
            {
                if (strcmp(args[1], stuinfo[k].enc_pw) == 0)
                {
                    strcpy(data, "PASS");
                    break;
                }
                else
                {
                    strcpy(data, "FAIL_PASS_NO_MATCH");
                    break;
                }
            }
            else
                strcpy(data, "FAIL_NO_USER");
        }

        /****************UDP: SEND DATA************************/
        numbytes = sendto(sockfd, data, strlen(data), 0, (struct sockaddr *)&their_addr, addr_len);

        if (numbytes == -1)
        {
            perror("listener: sendto");
            exit(1);
        }
        else
            printf("The ServerC finished sending the response to the Main Server.\n");

        // close(sockfd);
    }

    return 0;
}