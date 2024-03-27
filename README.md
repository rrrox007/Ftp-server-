The Folowing code is a simple Secure File Transfer System (SFTS) which is a client-server application developed using C++ and Boost Asio library with SSL support. It allows users to securely transfer files between a client and a server over a TCP/IP connection.
Features

Server
SSL Encryption: Communication between the server and clients is encrypted using SSL/TLS protocols, ensuring data security.
Authentication: Users are required to provide a username and password for authentication before accessing the server.
File Reception: The server can receive files sent by clients and save them locally.
Client Interaction: Provides options for clients to view received files, skip viewing, or exit the connection.

Client
SSL Encryption: Communication between the client and server is encrypted using SSL/TLS protocols, ensuring data security.
File Sending: Clients can send files to the server for storage.
File Creation: Allows users to create files with custom content locally before sending them to the server.
User Interaction: Provides a command-line interface for users to interact with the server, including sending files, leaving the connection, and creating file

Requirements
C++ compiler with C++17 support
Boost libraries (Boost Asio, Boost Filesystem)
OpenSSL library

Installation
Follow these steps to set up and run the SFTS:
Clone or download the SFTS repository to your local machine.
Install Microsoft Visual Studio and configure it for C++ development.
Install Boost Libraries (version 1.84 or above) and OpenSSL Library, ensuring they are properly configured and accessible in your development environment.
Build the server and client executables using Microsoft Visual Studio.
Launch the server executable first, followed by the client executable.
Follow the on-screen prompts to authenticate, transfer files, and manage connections.
