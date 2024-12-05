#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

#define PORT 21471 // USC ID: 4779562471

// Load members from members.txt into an unordered_map
void loadMembers(unordered_map<string, string> &members) {
    ifstream file("members.txt");
    if (!file) {
        cerr << "Error: Could not open members.txt" << endl;
        exit(EXIT_FAILURE);
    }
    string line, username, password;
    while (getline(file, line)) {
        istringstream iss(line);
        iss >> username >> password;
        members[username] = password; // Store the encrypted password
    }
    file.close();
}

// Handle authentication requests
void handleAuthRequest(const string &request, const unordered_map<string, string> &members, 
                       struct sockaddr_in &clientAddr, int sockfd, socklen_t addrlen) {
    stringstream iss(request);
    string username, password,command;
    iss >> command >> username >> password;


    // Log receipt of authentication request
    cout << "ServerA received username " << username << " and password ******" << endl;

    string response;
    if (members.count(username) && members.at(username) == password) { // Compare with encrypted password
        // Log success for authenticated member
        cout << "Member " << username << " has been authenticated" << endl;
        response = "AUTH_SUCCESS";
    } else {
        // Log failure for incorrect username/password
        cout << "The username " << username << " or password ****** is incorrect" << endl;
        response = "AUTH_FAIL";
    }

    // Send response to the main server
    sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr *)&clientAddr, addrlen);
}

int main() {
    int sockfd;
    struct sockaddr_in serverAddr, clientAddr;
    char buffer[1024];
    unordered_map<string, string> members;

    // Load members from file
    loadMembers(members);

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setup server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket to the address
    if (::bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Booting up message
    cout << "Server A is up and running using UDP on port " << PORT << endl;

    while (true) {
        socklen_t addrlen = sizeof(clientAddr);
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&clientAddr, &addrlen);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            string request(buffer);


            
            // Delegate authentication handling
            handleAuthRequest(request, members, clientAddr, sockfd, addrlen);
        }
    }

    close(sockfd);
    return 0;
}
