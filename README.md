# Project Title
Networked Multi-Server System

## Author Information
Yike Yang
4779562471

## Description
This project implements a multi-server system comprising one main server and three backend servers (A, R, and D). A client can connect to these servers to perform operations.

### Files
- **serverM.cpp:** Main server code that orchestrates communication between the client and backend servers.
- **serverA.cpp:** Backend server A handling specific operations.
- **serverR.cpp:** Backend server R handling specific operations.
- **serverD.cpp:** Backend server D handling specific operations.
- **client.cpp:** Client program to connect to the server.

### Message Format
All messages exchanged are in plain text format with the following conventions:
- Message from client to server: `<USERNAME>:<PASSWORD>:<REQUEST>`
- Message from server to client: `<STATUS>:<DATA>`


### Reused Code
No reused code.

## How to Compile and Run
1.  make all
2.  make run_all
3.  make clean

