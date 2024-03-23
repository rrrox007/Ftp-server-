﻿
//Microsoft Visual Studio Solution File, Format Version 12.00
// Visual Studio Version 17

#include <iostream> // For standard input and output
#include <fstream> // For file input and output
#include <boost/asio.hpp> // For Boost Asio library
#include <boost/asio/ssl.hpp> // For Boost Asio SSL support
#include <openssl/ssl.h> // For OpenSSL SSL support

using namespace boost::asio; // Boost Asio namespace
using ip::tcp; // TCP protocol namespace

// Paths to certificate and private key files
const char* certificate_file = "certificate.crt";// edit to the path of the .crt
const char* private_key_file = "private.key";//edit to the path of the .key

// Function to authenticate user
bool authenticateUser(const std::string& username, const std::string& password) {
    return username == "admin" && password == "admin";
}

// Class for handling SSL clients
class SSLClientHandler : public std::enable_shared_from_this<SSLClientHandler> {
public:
    using Ptr = std::shared_ptr<SSLClientHandler>;

    // Constructor
    SSLClientHandler(io_context& io_context, ssl::stream<tcp::socket> socket)
        : io_context_(io_context), socket_(std::move(socket)) {}

    // Start handling client connection
    void start(const tcp::endpoint& endpoint) {
        endpoint_ = endpoint;
        std::cout << "Client connected from " << endpoint_.address().to_string() << " on port " << endpoint_.port() << "." << std::endl;
        handleHandshake();
    }

private:
    // Handle SSL handshake
    void handleHandshake() {
        auto self = shared_from_this(); // Capture the shared pointer to this handler
        socket_.async_handshake(ssl::stream_base::server, // Initiate asynchronous SSL handshake from server side
            [this, self](const boost::system::error_code& error) { // Asynchronous callback for handling handshake completion
                if (!error) { // If no error occurred during handshake
                    std::cout << "Client secure" << std::endl; // Inform that client connection is secure
                    handleRequest(); // Proceed to handle client requests
                }
                else { // If there was an error during handshake
                    std::cerr << "Handshake error: " << error.message() << std::endl; // Print the error message
                }
            });
    }

    // Handle client requests
    void handleRequest() {
        auto self = shared_from_this(); // Capture the shared pointer to this handler
        async_read_until(socket_, commandBuffer_, "\n", // Asynchronously read data from the socket until newline delimiter
            [this, self](const boost::system::error_code& error, std::size_t) { // Asynchronous callback for handling read completion
                if (!error) { // If no error occurred during read operation
                    std::istream commandStream(&commandBuffer_); // Create an input stream to read data from the command buffer
                    std::string command; // Variable to store the received command
                    std::getline(commandStream, command); // Read a line from the command buffer

                    // If client wants to send a file
                    if (command == "S") {
                        std::cout << "Client wants to send file. Y/N (y for yes and N for No): "; // Prompt user for file transfer confirmation
                        std::string response; // Variable to store user response
                        std::cin >> response; // Read user response

                        // If user agrees to receive the file
                        if (response == "y" || response == "Y") {
                            boost::asio::streambuf fileBuffer; // Buffer for receiving file data
                            std::ofstream outputFile("received_file.txt", std::ios::binary); // Create an output file stream to save received file
                            if (!outputFile) { // If failed to open output file
                                std::cerr << "Error opening file." << std::endl; // Print error message
                                return; // Return from function
                            }

                            // Receive file from client
                            while (true) {
                                boost::system::error_code readError; // Variable to store read operation error
                                size_t bytes_transferred = boost::asio::read_until(socket_, fileBuffer, "\n", readError); // Asynchronously read data from socket until newline delimiter
                                if (readError == boost::asio::error::eof || readError == boost::asio::error::connection_reset) { // If end of file or connection reset
                                    break; // Exit loop
                                }
                                else if (readError) { // If other read error occurred
                                    throw boost::system::system_error(readError); // Throw system error
                                }

                                std::istream fileStream(&fileBuffer); // Create an input stream to read data from the file buffer
                                std::string line; // Variable to store each line of file data
                                std::getline(fileStream, line); // Read a line from the file buffer
                                if (line == "TRANSFER_COMPLETE") { // If transfer is complete
                                    break; // Exit the loop
                                }
                                outputFile << line << std::endl; // Write line to the output file
                            }

                            std::cout << "File received successfully." << std::endl; // Inform about successful file reception

                            // Prompt user for further action
                            std::cout << "Enter a number:" << std::endl;
                            std::cout << "1: View file" << std::endl;
                            std::cout << "2: Do not view file" << std::endl;
                            std::cout << "3: Exit" << std::endl;

                            int choice; // Variable to store user choice
                            std::cin >> choice; // Read user choice

                            // Perform action based on user choice
                            switch (choice) {
                            case 1: // If user chooses to view file
                                displayFileContents(); // Display file contents
                                break;
                            case 2: // If user chooses not to view file
                                break; // Continue without displaying file contents
                            case 3: // If user chooses to exit
                                return; // Exit from function
                            default: // If user enters an invalid choice
                                std::cerr << "Invalid choice." << std::endl; // Inform about invalid choice
                            }
                        }
                    }
                    else { // If invalid command received from client
                        std::cerr << "Invalid command received from client: " << command << std::endl; // Print error message
                    }

                    handleRequest(); // Continue handling client requests
                }
                else { // If error occurred during read operation
                    std::cerr << "Read error: " << error.message() << std::endl; // Print error message
                }
            });
    }

    // Method to display contents of received file
    void displayFileContents() {
        std::ifstream inputFile("received_file.txt"); // Open received file for reading
        if (inputFile) { // If file is successfully opened
            std::string fileContent((std::istreambuf_iterator<char>(inputFile)), // Read file content
                (std::istreambuf_iterator<char>()));
            std::cout << "File content:" << std::endl; // Print file content
            std::cout << fileContent << std::endl;
            inputFile.close(); // Close file
        }
        else { // If failed to open file
            std::cerr << "Error opening file." << std::endl; // Print error message
        }
    }

    io_context& io_context_; // Reference to io_context for asynchronous operations
    ssl::stream<tcp::socket> socket_; // SSL socket for communication with client
    boost::asio::streambuf commandBuffer_; // Buffer for storing received commands
    tcp::endpoint endpoint_; // Endpoint information of the client
};

// Class for SSL file server
class SSLFileServer {
public:
    // Constructor
    SSLFileServer(io_context& io_context, unsigned short port, ssl::context& context)
        : io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        context_(context) {
        acceptConnection(); // Start accepting connections
    }

private:
    // Method to accept client connections
    void acceptConnection() {
        auto newSocket = std::make_shared<ssl::stream<tcp::socket>>(io_context_, context_); // Create a new SSL socket
        acceptor_.async_accept(newSocket->next_layer(), // Asynchronously accept incoming connection
            [this, newSocket](const boost::system::error_code& error) { // Asynchronous callback for connection acceptance
                if (!error) { // If no error occurred
                    auto endpoint = newSocket->next_layer().remote_endpoint(); // Get remote endpoint details
                    std::cout << "Client connected from " << endpoint.address().to_string() << " on port " << endpoint.port() << "." << std::endl; // Print client connection details
                    auto handler = std::make_shared<SSLClientHandler>(io_context_, std::move(*newSocket)); // Create a new SSL client handler
                    handler->start(endpoint); // Start handling client
                    clientHandlers_.push_back(handler); // Add handler to list
                }
                else { // If error occurred during connection acceptance
                    std::cerr << "Accept error: " << error.message() << std::endl; // Print error message
                }
                acceptConnection(); // Continue to accept connections
            });
    }

    io_context& io_context_; // Reference to io_context for asynchronous operations
    ssl::context& context_; // SSL context for configuring SSL parameters
    tcp::acceptor acceptor_; // Acceptor for accepting incoming connections
    std::vector<SSLClientHandler::Ptr> clientHandlers_; // List of SSL client handlers
};

// Main function
int main() {
    std::string username; // Variable to store username
    std::string password; // Variable to store password

    std::cout << "Enter username: "; // Prompt user for username
    std::cin >> username; // Read username
    std::cout << "Enter password: "; // Prompt user for password
    std::cin >> password; // Read password

    if (!authenticateUser(username, password)) { // If user authentication fails
        std::cerr << "Authentication failed. Exiting..." << std::endl; // Inform about authentication failure
        return 1; // Exit program with error status
    }

    std::cout << "Authentication successful. Starting server..." << std::endl; // Inform about successful authentication

    try {
        io_context io_context; // Create Boost Asio io_context object
        ssl::context context(ssl::context::sslv23); // Create SSL context object

        // Configure SSL context
        context.set_options(ssl::context::default_workarounds |
            ssl::context::no_sslv2 |
            ssl::context::single_dh_use);
        context.use_certificate_file(certificate_file, ssl::context::pem); // Load certificate file
        context.use_private_key_file(private_key_file, ssl::context::pem); // Load private key file
        SSLFileServer server(io_context, 2121, context);

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
