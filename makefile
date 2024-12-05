# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -g

# Executables
EXECUTABLES = serverM serverA serverR serverD client

# Source files
SERVER_M_SRC = serverM.cpp
SERVER_A_SRC = serverA.cpp
SERVER_R_SRC = serverR.cpp
SERVER_D_SRC = serverD.cpp
CLIENT_SRC = client.cpp

# Object files
SERVER_M_OBJ = $(SERVER_M_SRC:.cpp=.o)
SERVER_A_OBJ = $(SERVER_A_SRC:.cpp=.o)
SERVER_R_OBJ = $(SERVER_R_SRC:.cpp=.o)
SERVER_D_OBJ = $(SERVER_D_SRC:.cpp=.o)
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)

# Targets
all: $(EXECUTABLES)

serverM: $(SERVER_M_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

serverA: $(SERVER_A_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

serverR: $(SERVER_R_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

serverD: $(SERVER_D_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

client: $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Clean-up
clean:
	rm -f *.o $(EXECUTABLES)

# Open terminals and run components
run_all:
	gnome-terminal -- bash -c "./serverM; exec bash" &
	gnome-terminal -- bash -c "./serverA; exec bash" &
	gnome-terminal -- bash -c "./serverR; exec bash" &
	gnome-terminal -- bash -c "./serverD; exec bash" &
	gnome-terminal -- bash -c "./client guest guest; exec bash" &
	gnome-terminal -- bash -c "./client <USERNAME> <PASSWORD>; exec bash" &

# Phony targets
.PHONY: all clean run_all
