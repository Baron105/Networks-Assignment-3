# SMTP Server-Client Implemenation

## Description

This is a simple SMTP server and client implementation in C. The server takes a single command line argument, the port number to listen on. The client takes three command line arguments, the server address, the SMTP port number and the POP3 port number (not used in this part).

The server waits for a connection from a client, then waits for a HELO command. It then waits for a MAIL FROM command, a RCPT TO command, a DATA command, and finally a QUIT command. The server then closes the connection and waits for another client. It communicates with the client using the SMTP protocol and relevant codes. It can handle multiple clients at once.

The client connects to the server, sends a HELO command, a MAIL FROM command, a RCPT TO command, a DATA command, and a QUIT command. It then closes the connection.

First of all, client gives a list of 3 options to the user. Option 1 is for retrieving the mail from the server (POP3 part so skipped here). Option 2 is for sending the mail to the server (SMTP part). Option 3 is for exiting the client. If the user chooses option 2, then the client asks for the sender's mail address, the recipient's mail address and then asks for the message to be sent. The client then sends the message to the server and closes the connection (if the message is valid else it prints an error message and closes the connection). The server then prints the message on the screen as well as in the mailbox file. The client then asks for the next option.

Note that the client does proper error checking in the message part with checks in the "From:", "To:", and "Subject:" fields. It also ensures that the mail address is valid of the form "user@domain". The termination of the message is done with a single period on a line by itself. After that, the client sends a QUIT command to the server and closes the connection.

## Usage

### Server

run `gcc smtpmail.c -o smtpmail` to compile the server. Then run `./smtpmail <port>` to run the server on the specified port. Note that the server requires no further input from the user.

### Client

run `gcc mailclient.c -o mailclient` to compile the client. Then run `./mailclient <server address> <SMTP port> <POP3 port>` to run the client. Then follow the instructions on the screen to send a mail to the server. Note that while username and password are asked, they are not used in this part of the assignment.

After one message is sent, the client again asks for the next option. If the user chooses option 3, then the client exits.

Note that for the receiver to get the message in their mailbox, the required mailbox file should be present inside a subdirectory with their name in the server directory. For example, if the receiver's mail address is "test@test", then the mailbox file will be saved in the file "mailbox" inside the directory "test" in the server directory. Here we have provided some sample mailbox files for testing purposes. If the user requires any other mailbox file, then they have to create it manually (mentioned in the assignment).
