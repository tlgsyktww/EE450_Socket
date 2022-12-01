/**
** serverM.c
** BASED ON STREAM server.c & DATAGRAM talker.c in Beej's Guide to Network Programming
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

#define SERVERPORTC "21456"   // the port users will be connecting to
                              // Backend-Server (C)
#define SERVERPORTCS "22456"  // the port users will be connecting to
                              // Backend-Server (CS)
#define SERVERPORTEE "23456"  // the port users will be connecting to
                              // Backend-Server (EE)
#define UDPPORT "24456"       // the port use UDP
#define CLIENTPORT "25456"    // the port for TCP with client
#define IPADDRESS "127.0.0.1" // local IP address

#define BACKLOG 10 // how many pending connections queue will hold
#define MAXBUFLEN 200

// from Beej
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;

    errno = saved_errno;
}

// from Beej
// get socket address from a sockaddr struct
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET6)
    {
        // IPv6
        return &(((struct sockaddr_in6 *)sa)->sin6_addr);
    }
    // IPv4
    return &(((struct sockaddr_in *)sa)->sin_addr);
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

// BASED on server.c from Beej
int setupTCP(char *port)
{
    int rv;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sigaction sa;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    return sockfd;
}

// BASED on talker.c from Beej
// ONLY create and bind UDP
int setupUDP(char *port)
{
    int sockfd;
    int rv;
    struct addrinfo hints, *servinfo, *p;
    socklen_t addr_len;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("talker: socket");
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
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    freeaddrinfo(servinfo); // done with servinfo
    return sockfd;
}

// UDP communication on specific port
// send and receive message
void UDPQuery(int sockfd, char *query, char *port, char *data)
{
    int numbytes;
    int rv;
    struct addrinfo hints, *servinfo, *p;
    socklen_t addr_len;
    memset(&hints, 0, sizeof hints);
    char recv_data[MAXBUFLEN]; // data received from backend server

    // sockfd = setupUDP(query, port);
    if ((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1)
        {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "talker: failed to create socket\n");
        return;
    }

    // send message to server C/CS/EE
    if ((numbytes = sendto(sockfd, query, strlen(query), 0,
                           p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }
    else if (strcmp(port, SERVERPORTC) == 0)
    {
        printf("The main server sent an authentication request to serverC.\n");
    }
    else if (strcmp(port, SERVERPORTCS) == 0)
    {
        printf("The main server sent a request to serverCS.\n");
    }
    else
    {
        printf("The main server sent a request to serverEE.\n");
    }

    // receive message from server C/CS/EE
    int recv_bytes;

    recv_bytes = recvfrom(sockfd, recv_data, sizeof recv_data, 0, NULL, NULL);
    if (recv_bytes == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    else if (strcmp(port, SERVERPORTC) == 0)
    {
        printf("The main server received the result of the authentication request from ServerC using UDP over port %s.\n", UDPPORT);
    }
    else if (strcmp(port, SERVERPORTCS) == 0)
    {
        printf("The main server received the response from serverCS using UDP over port %s.\n", UDPPORT);
    }
    else
    {
        printf("The main server received the response from serverEE using UDP over port %s.\n", UDPPORT);
    }

    recv_data[recv_bytes] = '\0';
    strcpy(data, recv_data);
}

int main(void)
{
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    int udp_sock;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;                 // used for accept()
    char s[INET6_ADDRSTRLEN];
    int numbytes; // using in receive or send

    char message_buf[100];   // message from client through TCP
    char result_buf[4000];   // message to client through TCP
    char enc[100];           // user information after encryption
    char udp_send[100];      // message to server through UDP
    char udp_received[4000]; // message from server through UDP
    char apt[5];             // identify the department of the course
    char name[20];
    char *splited_user[2]; // form: username/password
    char *splited_coz[15]; // form: “one”/coz_code/category
                           // form: coznum/coz_code<1>/coz_code<2>...

    /****************CREATE TCP & UDP CONNECTION************************/
    sockfd = setupTCP(CLIENTPORT);
    udp_sock = setupUDP(UDPPORT);
    printf("The main server is up and running.\n");
    // printf("server: waiting for connections...\n");

    // the main accept loop
    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        // printf("server: got connection from %s\n", s);

        /****************DATA TRANSFER: LOGIN************************/
        // child process
        if (!fork())
        {
            close(sockfd);

            while (1)
            {
                /****************TCP: RECEIVE DATA************************/
                if ((numbytes = recv(new_fd, message_buf, sizeof message_buf, 0)) == -1)
                {
                    perror("recv");
                    exit(1);
                }
                message_buf[numbytes] = '\0';
                fflush(stdout);
                // in message_buf

                // split received string of args into multiple string
                split_args(splited_user, message_buf);
                // printf("username: %s, password: %s \n", splited_user[0], splited_user[1]);
                printf("The main server received the authentication for %s using TCP over port %s. \n", splited_user[0], CLIENTPORT);

                /**********************ENCRYPTION*************************/
                sprintf(enc, "%s %s", splited_user[0], splited_user[1]);
                int i = 0;
                while (enc[i] != '\0')
                {
                    if (enc[i] >= 'a' && enc[i] <= 'z')
                    {
                        if (enc[i] <= 'v')
                            enc[i] = enc[i] + 4;
                        else
                            enc[i] = enc[i] - 22;
                    }
                    else if (enc[i] >= 'A' && enc[i] <= 'Z')
                    {
                        if (enc[i] <= 'V')
                            enc[i] = enc[i] + 4;
                        else
                            enc[i] = enc[i] - 22;
                    }
                    else if (enc[i] >= '0' && enc[i] <= '9')
                    {
                        if (enc[i] <= '5')
                            enc[i] = enc[i] + 4;
                        else
                            enc[i] = enc[i] - 6;
                    }

                    i++;
                }
                // printf("user info in one char[] after encryption: %s \n", enc);

                /****************UDP: SEND DATA************************/
                strcpy(udp_send, enc);
                UDPQuery(udp_sock, udp_send, SERVERPORTC, udp_received);

                /****************UDP: RECEIVE DATA************************/
                // printf("%s\n", udp_received);

                /****************TCP: SEND DATA************************/
                strcpy(result_buf, udp_received);
                if (send(new_fd, result_buf, strlen(result_buf), 0) == -1)
                {
                    perror("send");
                }
                printf("The main server sent the authentication result to the client.\n");

                // if the account verification is successful, go to the next stage
                if (strcmp(result_buf, "PASS") == 0)
                {
                    strcpy(name, splited_user[0]);
                    break;
                }
            }

            /********************DATA TRANSFER: COURSE INQUIRY********************/
            while (1)
            {
                if ((numbytes = recv(new_fd, message_buf, sizeof message_buf, 0)) == -1)
                {
                    perror("recv");
                    exit(1);
                }
                message_buf[numbytes] = '\0';
                fflush(stdout);
                // in message_buf

                // split received string of args into multiple string
                split_args(splited_coz, message_buf);

                if (strcmp(splited_coz[0], "1") == 0)
                {
                    /****************SINGLE COURSE QUERY************************/
                    printf("The main server received from %s to query course %s about %s using TCP over port %s.\n", name, splited_coz[1], splited_coz[2], CLIENTPORT);
                    sprintf(udp_send, "%s %s", splited_coz[1], splited_coz[2]);
                    // determine the department of course
                    memcpy(apt, splited_coz[1], 2);
                    apt[2] = '\0';

                    /****************UDP: SEND DATA************************/
                    if (strcmp(apt, "CS") == 0)
                    {
                        UDPQuery(udp_sock, udp_send, SERVERPORTCS, udp_received);
                    }
                    else if (strcmp(apt, "EE") == 0)
                    {
                        UDPQuery(udp_sock, udp_send, SERVERPORTEE, udp_received);
                    }
                    else
                    {
                        strcpy(result_buf, "FAIL_NO_COURSE");
                        if (send(new_fd, result_buf, strlen(result_buf), 0) == -1)
                        {
                            perror("send");
                        }
                        printf("The main server sent the query information to the client.\n");

                        continue;
                    }

                    /****************UDP: RECEIVE DATA************************/
                    // printf("%s\n", result_buf);
                    strcpy(result_buf, udp_received);
                    /****************TCP: SEND DATA************************/
                    if (send(new_fd, result_buf, strlen(result_buf), 0) == -1)
                    {
                        perror("send");
                    }
                    printf("The main server sent the query information to the client.\n");
                }
                else
                {
                    /****************MULTI-COURSE QUERY************************/
                    int j = 0;
                    int quenum = (int)(*splited_coz[0] - '0'); // The number of courses requested by the client.
                    for (j = 1; j < quenum + 1; j++)
                    {
                        /****************UDP: SEND DATA************************/
                        // form: coz_code all
                        sprintf(udp_send, "%s all", splited_coz[j]);
                        // printf("multi-course query: from serverM to CS/EE: %s all", splited_coz[j]);

                        memcpy(apt, splited_coz[j], 2);
                        apt[2] = '\0';

                        if (strcmp(apt, "CS") == 0)
                        {
                            UDPQuery(udp_sock, udp_send, SERVERPORTCS, udp_received);
                        }
                        else if (strcmp(apt, "EE") == 0)
                        {
                            UDPQuery(udp_sock, udp_send, SERVERPORTEE, udp_received);
                        }
                        else
                        {
                            strcpy(result_buf, "FAIL_NO_COURSE");
                            if (send(new_fd, result_buf, strlen(result_buf), 0) == -1)
                            {
                                perror("send");
                            }

                            continue;
                        }

                        /****************UDP: RECEIVE DATA************************/
                        // printf("multi-course query: The server returns a single course information: %s\n", udp_received);
                        strcpy(result_buf, udp_received);
                        /****************TCP: SEND DATA************************/
                        if (send(new_fd, result_buf, strlen(result_buf), 0) == -1)
                        {
                            perror("send");
                        }
                    }
                }
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    return 0;
}