#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string>
#include <unordered_map>

using namespace std;

#define TCP_PORT "25471"     // Port for client communication
#define UDP_PORT "24471"     // Port for backend server communication
#define SERVER_A_PORT "21471" // Port for Server A (Authentication)
#define SERVER_R_PORT "22471" // Port for Server R (Repository Management)
#define SERVER_D_PORT "23471" // Port for Server D (Deployment)
#define BACKLOG 10000         // Pending connections queue size

unordered_map<int, string> authenticatedClients; // Tracks authenticated clients
string UserName;

// Helper function to get sockaddr (IPv4 or IPv6)
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// Send command to backend servers using UDP
void sendToServer(const string &serverPort, const string &command, string &response) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP

    if ((rv = getaddrinfo("127.0.0.1", serverPort.c_str(), &hints, &servinfo)) != 0) {
        cerr << "getaddrinfo error: " << gai_strerror(rv) << endl;
        return;
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("serverM: socket");
            continue;
        }
        break;
    }

    if (p == nullptr) {
        cerr << "serverM: failed to create socket for backend server" << endl;
        return;
    }

    // Send command to the backend server
    if (sendto(sockfd, command.c_str(), command.size(), 0, p->ai_addr, p->ai_addrlen) == -1) {
        perror("serverM: sendto");
        close(sockfd);
        freeaddrinfo(servinfo);
        return;
    }

    // Receive response from the backend server
    char buffer[1024];
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    int bytes_received = recvfrom(sockfd, buffer, sizeof buffer - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
    if (bytes_received == -1) {
        perror("serverM: recvfrom");
    } else {
        buffer[bytes_received] = '\0';
        response = string(buffer);
    }

    close(sockfd);
    freeaddrinfo(servinfo);
}


// Lookup function
void handleLookup(const string &command, string &response,const string &clientUsername) {
    string lookupUsername = command.substr(7); // Extract <username> from "lookup <username>"
    cout << "The main server has received a lookup request from " 
         << clientUsername
         << " to lookup " << lookupUsername
         << "’s repository using TCP over port " << TCP_PORT << "." << endl;

    sendToServer(SERVER_R_PORT, command, response);

    cout << "The main server has sent the lookup request to Server R." << endl;
    cout << "The main server has received the response from Server R using UDP over " << UDP_PORT << "." << endl;

    if (response.find("No files found") == string::npos) {
        response = "Files in " + lookupUsername + "'s repository:\n" + response;
    } else {
        response = lookupUsername + "'s repository is empty.\n";
    }
}




string encryptPassword(const string& password) {
    string encrypted = password;

    for (size_t i = 0; i < password.size(); ++i) {
        char ch = password[i];

        if (ch >= 'a' && ch <= 'z') {
            // Lowercase letter encryption
            encrypted[i] = 'a' + (ch - 'a' + 3) % 26;
        } else if (ch >= 'A' && ch <= 'Z') {
            // Uppercase letter encryption
            encrypted[i] = 'A' + (ch - 'A' + 3) % 26;
        } else if (ch >= '0' && ch <= '9') {
            // Digit encryption
            encrypted[i] = '0' + (ch - '0' + 3) % 10;
        } else {
            // Special character (unchanged)
            encrypted[i] = ch;
        }
    }

    return encrypted;
}


// Function to send and receive authentication requests to Server A
string handleAuth(const string &command, string &response) {
    
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    char buffer[1024];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // UDP

    // Resolve Server A address
    if ((rv = getaddrinfo("127.0.0.1", SERVER_A_PORT, &hints, &servinfo)) != 0) {
        cerr << "getaddrinfo: " << gai_strerror(rv) << endl;
        response = "AUTH_FAIL";
        
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("serverM: socket");
            continue;
        }
        break;
    }

    if (p == nullptr) {
        cerr << "serverM: failed to create socket" << endl;
        response = "AUTH_FAIL";
        return "";
    }

    
    // Extract the password and encrypt it
    size_t first_space = command.find(' ');
    if (first_space == string::npos) {
        cerr << "Invalid command format." << endl;
        response = "AUTH_FAIL";
        close(sockfd);
        freeaddrinfo(servinfo);
        return "";
    }

    size_t second_space = command.find(' ', first_space + 1);
    if (second_space == string::npos) {
        cerr << "Invalid command format." << endl;
        response = "AUTH_FAIL";
        close(sockfd);
        freeaddrinfo(servinfo);
        return "";
    }

    string auth = command.substr(0, first_space);              // Extract auth
    string username = command.substr(first_space + 1, second_space - first_space - 1); // Extract username
    string password = command.substr(second_space + 1);        // Extract password
    string encryptedPassword = encryptPassword(password);      // Encrypt password


    if(username =="guest"&&password == "guest"){
        return "guest";
    }

    // Construct the encrypted command
    string encryptedCommand = auth + " " + username + " " + encryptedPassword;

    // Send auth command to Server A
    if (sendto(sockfd, encryptedCommand.c_str(), encryptedCommand.size(), 0, p->ai_addr, p->ai_addrlen) == -1) {
        perror("serverM: sendto");
        response = "AUTH_FAIL";
        close(sockfd);
        freeaddrinfo(servinfo);
        return "";
    }


    // Receive response from Server A
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;

    int bytes_received = recvfrom(sockfd, buffer, sizeof buffer - 1, 0, (struct sockaddr *)&their_addr, &addr_len);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        response = string(buffer);
        cout << "serverM: received response from Server A: " << response << endl;
    } else {
        perror("serverM: recvfrom");
        response = "AUTH_FAIL";
    }

    close(sockfd);
    freeaddrinfo(servinfo);
    return username;
}


void handlePush(const string &command, string &response, const string &clientUsername) {
    // 1. Upon receiving push request from a client
    cout << "The main server has received a push request from " << clientUsername
         << ", using TCP over port " << TCP_PORT << "." << endl;

    // 2. Forward the push request to Server R
    sendToServer(SERVER_R_PORT, command, response);

    // 3. Check Server R's response
    if (response == "DUPLICATE") {
        // Upon receiving response from server R asking for overwrite confirmation
        cout << "The main server has received the response from server R using UDP over " 
             << UDP_PORT << ", asking for overwrite confirmation." << endl;

        // Forward overwrite confirmation request to client
        cout << "The main server has sent the overwrite confirmation request to the client." << endl;

        // Here, simulate receiving the overwrite confirmation response from the client
        cout << "Please confirm overwrite (Y/N): ";
        string overwriteResponse;
        cin >> overwriteResponse;

        // Forward the overwrite confirmation response back to Server R
        cout << "The main server has received the overwrite confirmation response from " << clientUsername
             << " using TCP over port " << TCP_PORT << "." << endl;

        sendToServer(SERVER_R_PORT, overwriteResponse, response);

        cout << "The main server has sent the overwrite confirmation response to server R." << endl;

    } else if (response == "OVERWRITE_SUCCESS" || response == "PUSH_SUCCESS") {
        // Upon receiving a push request response from Server R (excluding overwrite confirmation requests)
        cout << "The main server has received the response from server R using UDP over " << UDP_PORT << "." << endl;

        // After forwarding the response to the client
        cout << "The main server has sent the response to the client." << endl;
    } else if (response == "OVERWRITE_DENIED") {
        // Special handling for overwrite denial
        cout << "The main server has received the response from server R using UDP over " << UDP_PORT << "." << endl;
        cout << "The main server has sent the response to the client." << endl;
    }
}




// Remove function
// Remove function
void handleRemove(const string &command, string &response, const string &clientUsername) {
    // Log the receipt of the remove request
    cout << "The main server has received a remove request from member " << clientUsername
         << " using TCP over port " << TCP_PORT << "." << endl;

    // Forward the remove request to Server R
    sendToServer(SERVER_R_PORT, command, response);

    // Log the confirmation received from Server R
    cout << "The main server has received confirmation of the remove request done by the server R." << endl;
}



// Deployment function
void handleDeployment(const string &command, string &response, const string &clientUsername) {
    cout << "The main server has received a deploy request from member " << clientUsername
         << " using TCP over port " << TCP_PORT << "." << endl;

    // Step 1: Forward the deploy request to Server R to lookup files for the user
    string lookupCommand = "lookup " + clientUsername;  // Create a command to lookup files
    string lookupResponse;
    sendToServer(SERVER_R_PORT, lookupCommand, lookupResponse);

    cout << "The main server has sent the lookup request to Server R." << endl;
    cout << "The main server received the lookup response from Server R: " << lookupResponse << endl;

    // Step 2: Check the lookup response
    if (lookupResponse.find("No files found") != string::npos) {
        response = "No files found in the repository for deployment.";
        cout << response << endl;
        return;  // Exit the function as there are no files to deploy
    }

    // Step 3: Forward the response from Server R to Server D for deployment
    string deployCommand = "deploy " + clientUsername + " " + lookupResponse;  // Append all filenames to deploy command
    sendToServer(SERVER_D_PORT, deployCommand, response);

    cout << "The main server has sent the deploy request to server D." << endl;

    // Step 4: Confirm deployment success or failure
    if (response.find("Deployment success") != string::npos) {
        cout << "The user " << clientUsername << "'s repository has been deployed at server D." << endl;
    } else {
        cout << "Deployment failed: " << response << endl;
    }
}



int main() {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    char s[INET6_ADDRSTRLEN];
    int rv;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Use my IP

    // Get server address information for TCP
    if ((rv = getaddrinfo(nullptr, TCP_PORT, &hints, &servinfo)) != 0) {
        cerr << "getaddrinfo error: " << gai_strerror(rv) << endl;
        return 1;
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("serverM: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            close(sockfd);
            return 1;
        }

        if (::bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("serverM: bind");
            continue;
        }

        break;
    }

    if (p == nullptr) {
        cerr << "serverM: failed to bind" << endl;
        return 2;
    }

    freeaddrinfo(servinfo);


    // Start listening for client connections
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        return 1;
    }

    cout << "Server M is up and running using UDP on port " << UDP_PORT << "." << endl;

    while (true) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        cout << "serverM: got connection from " << s << endl;

        if (!fork()) { // Child process to handle the connection
            close(sockfd);

            // Authentication Phase
            char buffer[1024];
            int bytes_received = recv(new_fd, buffer, sizeof buffer - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                string command(buffer);

                string response;
                string clientUsername;

                cout << "serverM: Received command: " << command << endl;

                if (command.find("auth") == 0) {
                    clientUsername = handleAuth(command, response);
                    if (response == "AUTH_FAIL") {
                        send(new_fd, response.c_str(), response.size(), 0);
                        close(new_fd);
                        exit(0); // Authentication failed, close connection
                    }
                    cout << "The main server has sent the authentication request to Server A." << endl;
                }else{
                    if (command.find("lookup") == 0) {
                    handleLookup(command, response, UserName);
                    cout << "The main server has sent the response to the client." << endl;
                    }
                }

                // Send authentication response
                if (send(new_fd, response.c_str(), response.size(), 0) == -1) {
                    perror("serverM: send");
                    close(new_fd);
                    exit(0);
                }

                // If authenticated, enter request handling loop
                while (true) {
                    bytes_received = recv(new_fd, buffer, sizeof(buffer) - 1, 0);
                    if (bytes_received > 0) {
                        buffer[bytes_received] = '\0';
                        command = string(buffer);

                        response.clear();

                        if (command.find("lookup") == 0) {
                            handleLookup(command, response, clientUsername);
                            cout << "The main server has sent the response to the client." << endl;
                        } else if (command.find("push") == 0) {
                            handlePush(command, response, clientUsername);
                        } else if (command.find("remove") == 0) {
                            handleRemove(command, response, clientUsername);
                        } else if (command.find("deploy") == 0) {
                            handleDeployment(command, response, clientUsername);
                        } else {
                            response = "Invalid command.";
                        }

                        // Send response to client
                        if (send(new_fd, response.c_str(), response.size(), 0) == -1) {
                            perror("serverM: send");
                            break;
                        }
                    } else if (bytes_received == 0) {
                        cout << "serverM: Client disconnected." << endl;
                        break; // Client closed the connection
                    } else {
                        perror("recv");
                        break; // Error receiving data
                    }
                }
            }

            close(new_fd);
            exit(0);
        }
        close(new_fd); // Parent doesn't need this connection
    }
    
        close(new_fd); // Parent doesn't need this connection
    
            

    return 0;
}
