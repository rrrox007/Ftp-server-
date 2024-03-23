
//Microsoft Visual Studio Solution File, Format Version 12.00
//Visual Studio Version 17

#include <iostream> // For standard input and output
#include <fstream> // For file input and output
#include <vector> // For vector container
#include <boost/asio.hpp> // For Boost Asio library
#include <boost/asio/ssl.hpp> // For Boost Asio SSL support
#include <filesystem> // For filesystem operations

namespace fs = std::filesystem; // Alias for filesystem namespace

using boost::asio::ip::tcp; // TCP protocol namespace

// Function to create a file with given filename and content
void createFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename); // Create output file stream with given filename
    if (file.is_open()) { // Check if file opened successfully
        file << content; // Write content to the file
        std::cout << "File created successfully." << std::endl; // Print success message
        file.close(); // Close the file
    }
    else {
        std::cerr << "Error creating file." << std::endl; // Print error message if file creation failed
    }
}

int main() {
    try {
        boost::asio::io_context io_context; // Create Boost Asio I/O context
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::sslv23); // Create SSL context for client

        const char* certificate_file = "certificate.crt"; //edit to the Path to certificate file
        ssl_context.load_verify_file(certificate_file); // Load certificate file for SSL verification

        boost::asio::ssl::stream<tcp::socket> socket(io_context, ssl_context); // Create SSL socket

        tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 2121); // Define server endpoint
        socket.lowest_layer().connect(endpoint); // Connect to the server

        socket.handshake(boost::asio::ssl::stream_base::client); // Perform SSL handshake with the server

        std::cout << "Connected to server." << std::endl; // Print connection success message

        bool leave = false; // Flag to control loop execution
        while (!leave) { // Main loop to interact with the server
            std::string command; // Variable to store user command
            std::cout << "Enter command (S for send file, L for leave, W for create file): "; // Prompt user for command
            std::cin >> command; // Read user command

            if (command == "S") { // If user wants to send a file
                std::vector<std::string> file_list; // Vector to store list of files in directory
                int index = 1; // Index counter for file list
                for (const auto& entry : fs::directory_iterator("C:\\cn")) { // Iterate through files in directory
                    std::cout << index << ". " << entry.path().filename() << std::endl; // Print file index and name
                    file_list.push_back(entry.path().string()); // Add file path to the list
                    index++; // Increment index counter
                }

                if (file_list.empty()) { // If no files found in directory
                    std::cerr << "No files found in directory." << std::endl; // Print error message
                    continue; // Continue to next iteration
                }

                int file_index; // Variable to store user-selected file index
                std::cout << "Enter the index of the file to send: "; // Prompt user to select a file
                std::cin >> file_index; // Read user input

                if (file_index < 1 || file_index > file_list.size()) { // If invalid file index
                    std::cerr << "Invalid file index." << std::endl; // Print error message
                    continue; // Continue to next iteration
                }

                std::ifstream file(file_list[file_index - 1], std::ios::binary); // Open selected file for reading
                if (!file) { // If failed to open file
                    std::cerr << "Error opening file." << std::endl; // Print error message
                    continue; // Continue to next iteration
                }

                boost::asio::write(socket, boost::asio::buffer(command + "\n")); // Send command to server

                std::string line; // Variable to store line read from file
                while (std::getline(file, line)) { // Read file line by line
                    boost::asio::write(socket, boost::asio::buffer(line + "\n")); // Send file content to server
                }
                file.close(); // Close the file

                boost::asio::write(socket, boost::asio::buffer("TRANSFER_COMPLETE\n")); // Notify server that file transfer is complete

                std::cout << "File sent successfully." << std::endl; // Print success message
            }
            else if (command == "L") { // If user wants to leave
                boost::asio::write(socket, boost::asio::buffer(command + "\n")); // Send leave command to server
                std::cout << "Disconnected from server." << std::endl; // Print disconnection message
                leave = true; // Set leave flag to true to exit the loop
            }
            else if (command == "W") { // If user wants to create a file
                std::string filename, content; // Variables to store filename and content
                std::cout << "Enter filename to create: "; // Prompt user for filename
                std::cin >> filename; // Read user input
                std::cout << "Enter content for the file: "; // Prompt user for file content
                std::cin.ignore(); // Clear the input buffer
                std::getline(std::cin, content); // Read file content
                createFile("C:\\cn\\" + filename, content); // Create the file with given filename and content
            }
            else { // If invalid command
                std::cerr << "Invalid command." << std::endl; // Print error message
            }
        }

        socket.shutdown(); // Shutdown the socket
        socket.lowest_layer().close(); // Close the socket
    }
    catch (std::exception& e) { // Exception handling
        std::cerr << "Exception: " << e.what() << std::endl; // Print exception message
    }
    return 0; // Return from main function
}
