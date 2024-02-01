#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

// set the socket to non-blocking
// void set_nonblocking(int *sockfd)
// {
//     int flags = fcntl(*sockfd, F_GETFL, 0);
//     if (flags == -1)
//     {
//         perror("fcntl");
//         exit(EXIT_FAILURE);
//     }

//     if (fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
//     {
//         perror("fcntl");
//         exit(EXIT_FAILURE);
//     }
// }

int main(int argc, char *argv[])
{

    int server_socket, client_socket;
    int new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len = sizeof(client_addr);

    char buf[2048];

    // creating the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error in creating the socket\n");
        exit(1);
    }

    // setting the server address
    int port;
    // printf("Enter the port number: ");
    // scanf("%d", &port);

    if (argc == 2)
    {
        port = atoi(argv[1]);
    }

    else
    {
        printf("Usage: %s <smtp_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error in binding\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // listen for the connections
    if (listen(server_socket, 5) == -1)
    {
        perror("Error in listening\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("SMTP server ready and listening for connections\n");

    while (1)
    {
        // accept the connection
        new_sock = accept(server_socket, (struct sockaddr *)&client_addr, &sin_len);

        // identify the client
        printf("Got connection from (%s , %d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (new_sock < 0)
        {
            perror("Error in accepting the connection\n");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // making a fork for concurrency
        int pid;
        if ((pid = fork()) == 0)
        {
            // close the old socket in the child process
            close(server_socket);

            // connection established
            char msg[2048] = "220 <Domain_name> Service ready\r\n";

            send(new_sock, msg, strlen(msg), 0);

            // recv HELO
            memset(buf, 0, sizeof(buf));
            int len;
            while (1)
            {
                len = recv(new_sock, buf, sizeof(buf), 0);
                // printf("%s\n", buf);
                if (buf[len - 1] == '\n' && buf[len - 2] == '\r')
                    break;
            }

            if (strncmp(buf, "HELO", 4) != 0)
            {
                printf("Not proper response\nclosing connection");
                close(new_sock);
                exit(0);
            }

            printf("%s\n", buf);

            // send 250 OK
            strcpy(msg, "250 OK hello ");
            strcat(msg, buf + 5);
            strcat(msg, "\r\n");
            send(new_sock, msg, strlen(msg), 0);

            // recv MAIL FROM
            memset(buf, 0, sizeof(buf));
            while (1)
            {
                len = recv(new_sock, buf, sizeof(buf), 0);
                if (buf[len - 1] == '\n' && buf[len - 2] == '\r')
                    break;
            }

            if (strncmp(buf, "MAIL FROM", 9) != 0)
            {
                printf("Not proper response\nclosing connection");
                close(new_sock);
                exit(0);
            }

            printf("%s\n", buf);
            char sender[100];
            strcpy(sender, buf + 11);
            // set last char to null
            sender[strlen(sender) - 2] = '\0';

            // send 250 OK
            strcpy(msg, "250");
            strcat(msg, buf + 10);
            strcat(msg, " Sender OK\r\n");

            send(new_sock, msg, strlen(msg), 0);

            // recv RCPT TO
            memset(buf, 0, sizeof(buf));
            while (1)
            {
                len = recv(new_sock, buf, sizeof(buf), 0);
                if (buf[len - 1] == '\n' && buf[len - 2] == '\r')
                    break;
            }

            if (strncmp(buf, "RCPT TO", 7) != 0)
            {
                printf("Not proper response\nclosing connection");
                close(new_sock);
                exit(0);
            }

            printf("%s\n", buf);
            char receiver[100];
            strcpy(receiver, buf + 9);

            // send 250 OK
            strcpy(msg, "250 root... Recipient OK\r\n");

            send(new_sock, msg, strlen(msg), 0);

            // recv DATA
            memset(buf, 0, sizeof(buf));
            while (1)
            {
                len = recv(new_sock, buf, sizeof(buf), 0);
                if (buf[len - 1] == '\n' && buf[len - 2] == '\r')
                    break;
            }

            if (strncmp(buf, "DATA", 4) != 0)
            {
                printf("Not proper response\nclosing connection");
                close(new_sock);
                exit(0);
            }

            printf("%s\n", buf);

            // send 354
            strcpy(msg, "354 Enter mail, end with \".\" on a line by itself\r\n");

            send(new_sock, msg, strlen(msg), 0);

            // recv mail

            // open mailbox
            FILE *fp;
            char path[42];

            // extract directory from sender by removing everything after @
            char receivername[20];
            for (int i = 0; i < strlen(receiver); i++)
            {
                if (receiver[i] == '@')
                {
                    receivername[i] = '\0';
                    break;
                }
                receivername[i] = receiver[i];
            }

            snprintf(path, sizeof(path), "%s/mailbox", receivername);

            fp = fopen(path, "a");
            if (fp == NULL)
            {
                perror("Error opening mailbox");
                exit(EXIT_FAILURE);
            }

            memset(buf, 0, sizeof(buf));
            while (1)
            {
                while (1)
                {
                    memset(buf, 0, sizeof(buf));
                    len = recv(new_sock, buf, sizeof(buf), 0);
                    if (buf[len - 1] == '\n' && buf[len - 2] == '\r')
                        break;
                }
                buf[len - 2] = '\0';
                printf("%s\n", buf);

                // store to user's mailbox
                if (strncmp(buf, "\r\n", 2) == 0)
                {
                    write(fileno(fp), ".", 1);
                }
                else write(fileno(fp), buf, strlen(buf));
                write(fileno(fp), "\n", 1);

                // add time to the message after subject is received
                if (strncmp(buf, "Subject", 7) == 0)
                {
                    // Received: <time>
                    time_t t = time(NULL);
                    struct tm *tm = localtime(&t);
                    char time[64];
                    strftime(time, sizeof(time), "%c", tm);

                    write(fileno(fp), "Received: ", 10);
                    write(fileno(fp), time, strlen(time));
                    write(fileno(fp), "\n", 1);
                }

                if (strncmp(buf, "\r\n", 2) == 0)
                {
                    break;
                }
            }
            printf("\n\n");

            // send 250 OK
            strcpy(msg, "250 OK Message accepted for delivery\r\n");

            send(new_sock, msg, strlen(msg), 0);

            close(new_sock);
            printf("Connection closed\n");
            exit(0);
        }
        close(new_sock);
    }
}