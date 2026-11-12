![CI Build Status](https://github.com)


<!-- Usage -->
Because your implementation sets the default factory lambda to instantiate RealConnection("localhost", 8080) automatically, your production code continues to work out of the box without any extra setup:


// Production code just calls the singleton methods normally
AA::OpenTestSingleton::Instance().add(myTest);
// Under the hood, sendDueDate() uses the default RealConnection factory automatically.


but if you ever need to change your production server host or port dynamically (e.g., pulling from an environment variable or config file), you can set the factory once at the very beginning of your application's main() function:

// In your main.cpp (Production Entry Point)
#include "OpenTestSingleton.h"
#include "RealConnection.h"
#include <cstdlib> // for getenv

int main(int argc, char* argv[]) {
    // Optional: Configure production host/port from environment or config
    std::string host = "127.0.0.1";
    int port = 8080;

    if (const char* env_p = std::getenv("TEST_SERVER_PORT")) {
        port = std::stoi(env_p);
    }

    // Bind the factory once at startup with your custom production settings
    AA::OpenTestSingleton::SetConnectionFactory([host, port](const std::string&, int) {
        return std::make_unique<RealConnection>(host, port);
    });

    // ... start the rest of your application ...
    return 0;
}

