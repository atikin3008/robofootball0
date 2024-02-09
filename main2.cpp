#include "CppSockets/sockets/TcpServerSocket.hpp"
#include<iostream>
#include<future>
#include<thread>

bool check = true;

void Socket() {
    TcpServerSocket server("localhost", 5010);
    server.acceptConnection();
    char x[100];
    server.receiveData(x, 100);
    std::cout << x;
    check = false;

}


int main() {
    std::thread x(Socket);
    while (check) {
        std::cout << 1;
    };
    x.detach();

    std::cout << 2;

}
