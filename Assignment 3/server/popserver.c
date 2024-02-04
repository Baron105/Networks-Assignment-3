#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>

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

    int port;

    if (argc == 2)
    {
        port = atoi(argv[1]);
    }

    else
    {
        printf("Usage: %s <pop3_port>\n", argv[0]);
        exit(0);
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

    printf("POP3 server is listening on port %d\n", port);

    while (1)
    {
        // accept the connection
        new_sock = accept(server_socket, (struct sockaddr *)&client_addr, &sin_len);

        if (new_sock < 0)
        {
            perror("Error in accepting the connection\n");
            exit(1);
        }

        // identify the client
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // make forks for concurrent connections
        if (fork() == 0)
        {
            close(server_socket);

            // send the welcome message
            char *welcome = "+OK POP3 server ready\r\n";
            send(new_sock, welcome, strlen(welcome), 0);

            memset(buf, 0, sizeof(buf));

            // receive the username
            recv(new_sock, buf, sizeof(buf), 0);
            printf("%s", buf);

            // remove the USER and \r\n from the username
            buf[strlen(buf) - 2] = '\0';

            // check first 4 characters
            if (strncmp(buf, "USER", 4) != 0)
            {
                char *invalid_msg = "-ERR Invalid command\r\n";
                send(new_sock, invalid_msg, strlen(invalid_msg), 0);
                close(new_sock);
                exit(0);
            }

            // now extract the username

            // go to first non space character
            int i = 4;
            while (buf[i] == ' ')
            {
                i++;
            }

            char uname[100];
            strcpy(uname, buf + i);

            // check if username is valid from the file user.txt
            FILE *file = fopen("user.txt", "r");
            char username[100];
            char password[100];
            int valid = 0;

            while (fscanf(file, "%s %s", username, password) != EOF)
            {
                if (strncmp(username, uname, strlen(username)) == 0)
                {
                    valid = 1;
                    break;
                }
            }

            fclose(file);

            if (valid)
            {
                char *valid_msg = "+OK User exists\r\n";
                send(new_sock, valid_msg, strlen(valid_msg), 0);
            }
            else
            {
                char *invalid_msg = "-ERR User does not exist\r\n";
                send(new_sock, invalid_msg, strlen(invalid_msg), 0);
                close(new_sock);
                exit(0);
            }

            memset(buf, 0, sizeof(buf));

            // now receive the password
            // as we have already received the username and the corresponding password is in the file user.txt
            recv(new_sock, buf, sizeof(buf), 0);
            buf[strlen(buf) - 2] = '\0';

            // check first 4 characters
            if (strncmp(buf, "PASS", 4) != 0)
            {
                char *invalid_msg = "-ERR Invalid command\r\n";
                send(new_sock, invalid_msg, strlen(invalid_msg), 0);
                close(new_sock);
                exit(0);
            }

            printf("%s\n", buf);

            // now extract the password

            // go to first non space character
            i = 4;
            while (buf[i] == ' ')
            {
                i++;
            }

            char pass[100];
            strcpy(pass, buf + i);
            FILE *mailbox;

            if (strcmp(pass, password) == 0)
            {
                // go to mailbox and check for emails and octets
                // char *valid_msg = "+OK";

                // open the mailbox file
                char mailboxpath[100];

                snprintf(mailboxpath, sizeof(mailboxpath), "%s/mailbox", username);
                mailbox = fopen(mailboxpath, "r");

                // lock the mailbox
                if (flock(fileno(mailbox), LOCK_EX | LOCK_NB) == -1)
                {
                    perror("Error in locking the mailbox\n");
                    close(new_sock);
                    exit(1);
                }

                // count the number of emails by tracking \r\n.\r\n
                int emails = 0;
                int chars = 0;
                char blah[100];
                while (fgets(blah, sizeof(blah), mailbox))
                {
                    if (strncmp(blah, ".\r\n", 2) == 0)
                    {
                        emails++;
                    }
                    chars += strlen(blah);
                }
                int octets = chars / 8;

                char msg[100];
                snprintf(msg, sizeof(msg), "+OK %s's maildrop has %d messages (%d octets)\r\n", username, emails, octets);
                send(new_sock, msg, strlen(msg), 0);
            }
            else
            {
                char *invalid_msg = "-ERR Password incorrect\r\n";
                send(new_sock, invalid_msg, strlen(invalid_msg), 0);
                close(new_sock);
                exit(0);
            }

            
            int s;
            // now send the list of emails
            while(1)
            {
                // recv command
                memset(buf, 0, sizeof(buf));
                recv(new_sock, buf, sizeof(buf), 0);
                printf("%s", buf);

                if(strncmp(buf,"LIST",4)==0)
                {
                    //extract the mails in the format 
                    fseek(mailbox, 0, SEEK_SET);
                    s = 0;
                    char semail[100];
                    char rdatetime[100];
                    char subject[100];
                    int sc = 0, rc = 0, subc = 0;
                    while(fgets(buf, sizeof(buf), mailbox))
                    {
                        if(strncmp(buf, ".\r\n", 3) == 0)
                        {
                            s++;
                            sc = 0; rc = 0; subc = 0;
                            // send all data together
                            char msg[400];
                            snprintf(msg, sizeof(msg), "%d\t%s\t%s\t%s\r\n", s, semail, rdatetime, subject);
                            // printf("%s", msg);
                            send(new_sock, msg, strlen(msg), 0);
                        }

                        if(strncmp(buf, "From: ", 6) == 0 && sc == 0)
                        {
                            buf[strlen(buf) - 2] = '\0';
                            strcpy(semail, buf + 6);
                            sc = 1;
                        }

                        if(strncmp(buf, "Received: ", 10) == 0 && rc == 0)
                        {
                            buf[strlen(buf) - 2] = '\0';
                            strcpy(rdatetime, buf + 10);
                            rc = 1;
                        }

                        if(strncmp(buf, "Subject: ", 9) == 0 && subc == 0)
                        {
                            buf[strlen(buf) - 2] = '\0';
                            strcpy(subject, buf + 9);
                            subc = 1;
                        }
                        
                        memset(buf, 0, sizeof(buf));
                    }
                    memset(buf, 0, sizeof(buf));
                    strcpy(buf, "#");
                    send(new_sock, buf, strlen(buf), 0);
                }
                else if(strncmp(buf,"RETR",4)==0)
                {
                    // extract the mail number
                    int mailno;
                    while(1)
                    {
                        sscanf(buf, "RETR %d", &mailno);

                        // check the range of the mail 
                        if(mailno > s || mailno < 1)
                        {
                            char *invalid_msg = "-ERR Invalid mail number or no mails are there\r\n";
                            send(new_sock, invalid_msg, strlen(invalid_msg), 0);
                            continue;
                        }
                        else break;
                    }

                    // send ok along with the mail 
                    char msg[100];
                    // find the mail in the mailbox
                    fseek(mailbox, 0, SEEK_SET);
                    int c = 1;
                    int sz = 0;
                    char mail[4096];
                    // memset(mail, 0, sizeof(mail));
                    while(fgets(buf, sizeof(buf), mailbox))
                    {
                        if(c == mailno)
                        {
                            sz+=strlen(buf);
                            strcat(mail, buf);

                        }
                        if(strncmp(buf, ".\r\n", 3) == 0)
                        {
                            c++;
                        }
                    }
                    sz/=8;
                    snprintf(msg, sizeof(msg), "+OK %d octets\r\n", sz);
                    send(new_sock, msg, strlen(msg), 0);
                    printf("%s %s", msg,mail);

                    send(new_sock, mail, strlen(mail), 0);
                    memset(mail, 0, sizeof(mail));
                    memset(buf, 0, sizeof(buf));
                    strcpy(buf, "#");
                    send(new_sock, buf, strlen(buf), 0);


                    
                }


            }

            // unlock file
            if (flock(fileno(mailbox), LOCK_UN) == -1)
            {
                perror("Error in unlocking the mailbox\n");
                close(new_sock);
                exit(1);
            }

            close(new_sock);
            printf("Connection closed\n");
            exit(0);
        }

        close(new_sock);
    }
}