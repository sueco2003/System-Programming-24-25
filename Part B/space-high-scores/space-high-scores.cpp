#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include "../points.pb.h"
#include "stdlib.h"

int main() {
    // Create the ZeroMQ context and REP socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_PULL);
    socket.bind("tcp://*:5559");

    while (true) {
        // Receive the message from the client
        zmq::message_t request;
        auto recv_result = socket.recv(request, zmq::recv_flags::none);
        if (!recv_result) {
            std::cerr << "Failed to receive message." << std::endl;
            continue;
        }

        // Deserialize the received protobuf message
        Simple_message msg;
        if (!msg.ParseFromArray(request.data(), request.size())) {
            std::cerr << "Failed to parse message." << std::endl;
            continue;
        }
        std::cout << "\033[2J\033[H";
        std::cout << "High Scores:" << std::endl;
        for (int i = 0; i < 8; i++) {
            const Player& player = msg.players(i);
            if (player.id().empty()) {
                continue;
            }
            std::cout << "Player ID: " << player.id() << ", Score: " << player.score() << std::endl;
        }
    }
}
