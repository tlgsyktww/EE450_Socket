/**
** serverCS.c
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
#define MYPORT "22456"        // the port used for UDP connection with serverM
#define SERVERMPORT "24456"   // the UDP port of serverM
#define MAXBUFLEN 4000
#define MAX_LINE 1024 // txt document read
#define MAX_COZ 50

struct COURSE_INFO
{
    char coz_code[10];  // course code
    char coz_cdt[5];    // course credit
    char coz_prof[50];  // course professor
    char coz_days[30];  // course days
    char coz_name[100]; // course name
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
    int coznum; // number of courses in txt
    struct COURSE_INFO cozinfo[MAX_COZ];

    // for udp
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    char buf[MAXBUFLEN];  // message from serverM through UDP
    char data[MAXBUFLEN]; // message to serverM through UDP
    char *args[5];

    /*********************INPUT TXT FILE*****************************/
    if ((fp = fopen("cs.txt", "r")) == NULL)
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
                txtbuf[txtlen - 1] = '\0';
            // printf("course info: %s\n", txtbuf);

            // split txtbuf and load 5 values in cozinfo struct
            char *p = strtok(txtbuf, ",");
            int j = 0;
            while (p != NULL)
            {
                if (j == 0)
                    strcpy(cozinfo[i].coz_code, p);
                if (j == 1)
                    strcpy(cozinfo[i].coz_cdt, p);
                if (j == 2)
                    strcpy(cozinfo[i].coz_prof, p);
                if (j == 3)
                    strcpy(cozinfo[i].coz_days, p);
                if (j == 4)
                    strcpy(cozinfo[i].coz_name, p);
                j++;
                p = strtok(NULL, ",");
            }
            // printf("splited code:%s, splited name:%s\n", cozinfo[i].coz_code, cozinfo[i].coz_name);

            i++;
        }
        coznum = i;
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

    printf("The ServerCS is up and running using UDP on port %s.\n", MYPORT);
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
        buf[numbytes] = '\0';

        split_args(args, buf);
        printf("The ServerCS received a request from the Main Server about the %s of %s.\n", args[1], args[0]);
        // printf("received info: %s, %s\n", args[0], args[1]);

        /****************FIND COURSE INFO************************/
        strcpy(data, "Before matching.");
        // For one course: send only category(credit/professor/days/coursename)info
        // For multi course: send all info
        // If fail: send FAIL_NO_COURSE

        int k = 0;
        for (k = 0; k < coznum; k++)
        {
            if (strcmp(args[0], cozinfo[k].coz_code) == 0)
            {
                if (strcmp(args[1], "Credit") == 0)
                {
                    strcpy(data, cozinfo[k].coz_cdt);
                    printf("The course information has been found: The credit of %s is %s.\n", args[0], cozinfo[k].coz_cdt);
                    break;
                }
                else if (strcmp(args[1], "Professor") == 0)
                {
                    strcpy(data, cozinfo[k].coz_prof);
                    printf("The course information has been found: The professor of %s is %s.\n", args[0], cozinfo[k].coz_prof);
                    break;
                }
                else if (strcmp(args[1], "Days") == 0)
                {
                    strcpy(data, cozinfo[k].coz_days);
                    printf("The course information has been found: The days of %s is %s.\n", args[0], cozinfo[k].coz_days);
                    break;
                }
                else if (strcmp(args[1], "CourseName") == 0)
                {
                    strcpy(data, cozinfo[k].coz_name);
                    printf("The course information has been found: The coursename of %s is %s.\n", args[0], cozinfo[k].coz_name);
                    break;
                }
                else // "all"
                {
                    // EE450: 4, Ali Zahid, Tue;Thu, Introduction to Computer Networks
                    sprintf(data, "%s: %s, %s, %s, %s", cozinfo[k].coz_code, cozinfo[k].coz_cdt, cozinfo[k].coz_prof, cozinfo[k].coz_days, cozinfo[k].coz_name);
                    break;
                }
            }
            else
            {
                strcpy(data, "FAIL_NO_COURSE");
            }
        }
        if (strcmp(data, "FAIL_NO_COURSE") == 0)
        {
            printf("Didn't find the course: %s.\n", args[0]);
        }

        /****************UDP: SEND DATA************************/
        numbytes = sendto(sockfd, data, strlen(data), 0, (struct sockaddr *)&their_addr, addr_len);

        if (numbytes == -1)
        {
            perror("listener: sendto");
            exit(1);
        }
        else
            printf("The ServerCS finished sending the response to the Main Server.\n");

        // close(sockfd);
    }

    return 0;
}