#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

#define PORT 22471 // USC ID: 4779562471

// Load repository data from filenames.txt into memory
void loadRepository(unordered_map<string, set<string>> &repository) {
    ifstream file("filenames.txt");
    if (!file) {
        cerr << "Error: Could not open filenames.txt" << endl;
        return;
    }
    string line, username, filename;
    while (getline(file, line)) {
        istringstream iss(line);
        iss >> username >> filename;
        repository[username].insert(filename);
    }
    file.close();
}

// Save repository data back to filenames.txt
void saveRepository(const unordered_map<string, set<string>> &repository) {
    ofstream file("filenames.txt");
    for (const auto &entry : repository) {
        for (const auto &filename : entry.second) {
            file << entry.first << " " << filename << "\n";
        }
    }
    file.close();
}

// Check if the client is a guest
bool isGuest(const string &username) {
    return username == "guest";
}

// Handle push request
void handlePushRequest(const string &username, const string &filename, unordered_map<string, set<string>> &repository, int sockfd, struct sockaddr_in &clientAddr, socklen_t addrlen) {
    cout << "Server R has received a push request from the main server." << endl;

    if (repository[username].count(filename)) {
        cout << filename << " exists in " << username << "'s repository; requesting overwrite confirmation." << endl;
        sendto(sockfd, "DUPLICATE", strlen("DUPLICATE"), 0, (struct sockaddr *)&clientAddr, addrlen);

        // Wait for overwrite confirmation
        char buffer[1024];
        int confirm_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&clientAddr, &addrlen);
        if (confirm_received > 0) {
            buffer[confirm_received] = '\0';
            if (string(buffer) == "Y") {
                repository[username].insert(filename);
                saveRepository(repository);
                cout << "User requested overwrite; overwrite successful." << endl;
                sendto(sockfd, "OVERWRITE_SUCCESS", strlen("OVERWRITE_SUCCESS"), 0, (struct sockaddr *)&clientAddr, addrlen);
            } else {
                cout << "Overwrite denied." << endl;
                sendto(sockfd, "OVERWRITE_DENIED", strlen("OVERWRITE_DENIED"), 0, (struct sockaddr *)&clientAddr, addrlen);
            }
        }
    } else {
        repository[username].insert(filename);
        saveRepository(repository);
        cout << filename << " uploaded successfully." << endl;
        sendto(sockfd, "PUSH_SUCCESS", strlen("PUSH_SUCCESS"), 0, (struct sockaddr *)&clientAddr, addrlen);
    }
}

// Handle lookup request
void handleLookupRequest(const string &username, unordered_map<string, set<string>> &repository, int sockfd, struct sockaddr_in &clientAddr, socklen_t addrlen) {
    cout << "Server R has received a lookup request from the main server." << endl;
    ostringstream response;
    if (repository.count(username)) {
        for (const auto &file : repository[username]) {
            response << file << " ";
        }
    } else {
        response << "No files found.";
    }
    sendto(sockfd, response.str().c_str(), response.str().size(), 0, (struct sockaddr *)&clientAddr, addrlen);
    cout << "Server R has finished sending the response to the main server." << endl;
}


// Handle remove request
void handleRemoveRequest(const string &username, const string &filename, unordered_map<string, set<string>> &repository, int sockfd, struct sockaddr_in &clientAddr, socklen_t addrlen) {
    // Log message for receiving a remove request
    cout << "Server R has received a remove request from the main server." << endl;

    // Check if the file exists in the repository and remove it
    if (repository[username].erase(filename)) {
        saveRepository(repository);
        sendto(sockfd, "REMOVE_SUCCESS", strlen("REMOVE_SUCCESS"), 0, (struct sockaddr *)&clientAddr, addrlen);

        // Log success processing message
        cout << "Server R has finished processing the remove request." << endl;
    } else {
        // If file not found, send failure response
        sendto(sockfd, "REMOVE_FAIL", strlen("REMOVE_FAIL"), 0, (struct sockaddr *)&clientAddr, addrlen);

        // Log file-not-found message
        cout << "File not found in the repository." << endl;
    }
}


void handleDeployRequest(const string &username, unordered_map<string, set<string>> &repository, int sockfd, struct sockaddr_in &clientAddr, socklen_t addrlen) {
    cout << "Server R has received a deploy request from the main server." << endl;
    ostringstream response;

    // Check if the user has files in the repository
    if (repository.count(username)) {
        for (const auto &file : repository[username]) {
            response << file << " ";
        }
    } else {
        response << "No files found.";
    }

    // Send the response back to serverM
    sendto(sockfd, response.str().c_str(), response.str().size(), 0, (struct sockaddr *)&clientAddr, addrlen);

    // Display message after finishing response
    cout << "Server R has finished sending the response to the main server." << endl;
}


int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[1024];
    unordered_map<string, set<string>> repository;

    // Load the repository into memory
    loadRepository(repository);

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the specified port
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (::bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    cout << "Server R is up and running using UDP on port " << PORT << endl;

    while (true) {
        socklen_t addrlen = sizeof(clientAddr);
        int bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&clientAddr, &addrlen);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            string request(buffer);
            istringstream iss(request);
            string command, username, filename;
            iss >> command >> username;

            cout << "Server R: Received lookup command: " << command << " " << username << endl;

            if (command == "lookup") {
                handleLookupRequest(username, repository, sockfd, clientAddr, addrlen);
            } else if (command == "push") {
                iss >> filename;
                handlePushRequest(username, filename, repository, sockfd, clientAddr, addrlen);
            } else if (command == "remove") {
                iss >> filename;
                handleRemoveRequest(username, filename, repository, sockfd, clientAddr, addrlen);
            } else if (command == "deploy") {
                handleDeployRequest(username, repository, sockfd, clientAddr, addrlen);
            }
        }
    }

    close(sockfd);
    return 0;
}
