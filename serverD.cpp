#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <set>

using namespace std;

#define PORT 23471 // USC ID: 4779562471

// Write deployed filenames to deployed.txt
void writeDeployedFiles(const string &username, const set<string> &files) {
    ofstream file("deployed.txt", ios::app); // Append mode
    if (!file) {
        cerr << "Error: Could not create deployed.txt" << endl;
        return;
    }

    for (const auto &filename : files) {
        file << username << " " << filename << "\n";
    }
    file.close();
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[1024];

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket creation failed");
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

    // Booting up message
    cout << "Server D is up and running using UDP on port " << PORT << endl;

    while (true) {
        socklen_t addrlen = sizeof(clientAddr);
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientAddr, &addrlen);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            string request(buffer);
            istringstream iss(request);
            string command, username;

            // Check if the command starts with "deploy"
            if (request.rfind("deploy ", 0) == 0) { // Ensure the command starts with "deploy"
                iss >> command >> username;

                // Log deploy request received
                cout << "Server D has received a deploy request from the main server for user: " << username << endl;

                // Simulate fetching files from Server R response (assuming Server R response contains file list)
                set<string> files;
                string filename;
                while (iss >> filename) {
                    files.insert(filename);
                }

                if (files.empty()) {
                    // If no files are found for deployment
                    string response = "No files found for user: " + username;
                    sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, addrlen);
                    continue;
                }

                // Write deployed files to deployed.txt in the desired format
                writeDeployedFiles(username, files);

                // Log deployment completion
                cout << "Server D has deployed the user " << username << "’s repository." << endl;

                // Send success response
                string response = "Deployment successful for user: " + username;
                sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, addrlen);
            } else {
                // Handle invalid command scenario
                string response = "Invalid command received by Server D.";
                sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, addrlen);
            }
        }
    }

    close(sockfd);
    return 0;
}
