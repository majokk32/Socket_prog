#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>

using namespace std;

#define SERVER_M_IP "127.0.0.1" // IP of Server M
#define SERVER_M_PORT 25471     // Port of Server M


// Function to connect to the server and return the socket
int connectToServer() {
    int sockfd;
    struct sockaddr_in serverAddr;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server information
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_M_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_M_IP);

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection to server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}


// Process server response
void processResponse(const string &response) {
    cout << "The client received the response from the main server using TCP." << endl;
    cout << response << endl;
    cout << "-----Start a new request-----" << endl;
}

bool Auth(int sockfd, const string &username, const string &password) {
    // Special case for guest authentication
    if (username == "guest" && password == "guest") {
        cout << "You have been granted guest access." << endl;
        return true;
    }

    // For other users, authenticate through the server
    string authMessage = "auth " + username + " " + password;
    cout << "Sending authentication message: " << authMessage << endl; // Debug message
    send(sockfd, authMessage.c_str(), authMessage.size(), 0);

    char buffer[1024];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        string response(buffer);
        cout << "Received response: " << response << endl; // Debug message

        if (response == "AUTH_SUCCESS") {
            cout << "You have been granted member access." << endl;
            return true;
        } else {
            cout << "The credentials are incorrect. Please try again." << endl;
            return false;
        }
    } else {
        cout << "No response received from the server." << endl; // Debug message
    }
    return false;
}


void MemberLookup(int sockfd, const string &username, const string &command) {
    string lookupCommand;

    if (command == "lookup") {
        // Case 1: No username specified, lookup the member's own repository
        lookupCommand = "lookup " + username;
        cout << "Username is not specified. Will lookup " << username << "'s repository." << endl;
    } else if (command.find("lookup ") == 0) {
        // Case 2: Username specified, extract it
        string lookupTarget = command.substr(7); // Extract <username> from "lookup <username>"
        lookupCommand = "lookup " + lookupTarget;
        cout << username << " sent a lookup request to the main server for " << lookupTarget << "'s repository." << endl;
    } else {
        cout << "Invalid lookup command. Please try again. -----Start a new request-----" << endl;
        return;
    }

    
    // Send the lookup command to the server
    send(sockfd, lookupCommand.c_str(), lookupCommand.size(), 0);

    // Wait for the response from the server
    char buffer[1024];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        string response(buffer);

        // Process the response from the server
        if (response.find("does not exist") != string::npos) {
            cout << "The client received the response from the main server using TCP over port <client port number>." << endl;
            cout << response << endl;
            cout << "Please try again. -----Start a new request-----" << endl;
        } else if (response.find("is empty") != string::npos) {
            cout << "The client received the response from the main server using TCP over port <client port number>." << endl;
            cout << "Empty repository." << endl;
            cout << "-----Start a new request-----" << endl;
        } else {
            cout << "The client received the response from the main server using TCP over port <client port number>." << endl;
            cout << "Files in repository:\n" << response << endl;
            cout << "-----Start a new request-----" << endl;
        }
    } else {
        cout << "No response received from the server." << endl;
        cout << "-----Start a new request-----" << endl;
    }
}



// Push
void push(int sockfd, const string &username, const string &command) {
    if (command.find("push") == 0 && command.length() <= 5) {
        cout << "Error: Filename is required. Please specify a filename to push." << endl;
        return;
    }

    send(sockfd, command.c_str(), command.size(), 0);
    cout << username << " sent a push request to the main server." << endl;

    char buffer[1024];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        processResponse(string(buffer));
    }
}

// Remove
void removeFile(int sockfd, const string &username, const string &command) {
    if (command.find("remove") == 0 && command.length() <= 7) {
        cout << "Error: Filename is required. Please specify a filename to remove." << endl;
        return;
    }

    send(sockfd, command.c_str(), command.size(), 0);
    cout << username << " sent a remove request to the main server." << endl;

    char buffer[1024];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        processResponse(string(buffer));
    }
}

// Deploy
void deploy(int sockfd, const string &username) {
    string command = "deploy " + username;

    send(sockfd, command.c_str(), command.size(), 0);
    cout << username << " sent a deploy request to the main server." << endl;

    char buffer[1024];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        processResponse(string(buffer));
    }
}


void handleGuestCommands(int sockfd) {
    while (true) {
        cout << "Please enter the command: <lookup <username>>" << endl;

        string command;
        getline(cin, command);

        // Validate command
        if (command.find("lookup") == 0) {
            if (command.length() <= 7) { // No username provided
                cout << "Error: Username is required. Please specify a username to lookup. -----Start a new request-----" << endl;
                continue;
            }
            cout << "Guest sent a lookup request to the main server." << endl;

            // Send command to server
            send(sockfd, command.c_str(), command.size(), 0);

            // Receive response
            char buffer[1024];
            int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                cout << "The client received the response from the main server using TCP." << endl;
                cout << string(buffer) << endl;
                cout << "-----Start a new request-----" << endl;
            }
        } else {
            cout << "Guests can only use the lookup command." << endl;
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: ./client <username> <password>" << endl;
        exit(EXIT_FAILURE);
    }

    string username = argv[1];
    string password = argv[2];

    cout << "The client is up and running." << endl;


    // Use different sockets for guest and member models
    int guestSockfd = -1, memberSockfd = -2;
    

    if (username == "guest" && password == "guest") {
        // Guest model connection
        guestSockfd = connectToServer();

        handleGuestCommands(guestSockfd);
        close(guestSockfd);

    } else {

        // Member model connection
        memberSockfd = connectToServer();
        if(Auth(memberSockfd, username, password)){
            // Member-specific command handling
            while (true) {
                cout << "Please enter the command: <lookup <username>> , <push <filename> > , <remove <filename> > , <deploy> , <log>." << endl;

                string command;
                getline(cin, command);
                
                if (command.find("lookup") == 0) {
                    MemberLookup(memberSockfd, username, command);
                } else if (command.find("push") == 0) {
                    push(memberSockfd, username, command);
                } else if (command.find("remove") == 0) {
                    removeFile(memberSockfd, username, command);
                } else if (command.find("deploy") == 0) {
                    deploy(memberSockfd, username);
                } else {
                    cout << "Invalid command. Please try again." << endl;
                }
            }
        }else{
            return 0;
        }
        close(memberSockfd);
    }

     
    return 0;
}

