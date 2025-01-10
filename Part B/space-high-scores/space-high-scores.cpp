#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include "../points.pb.h"
#include "stdlib.h"

#define PUBLISHER_ADDRESS "tcp://127.0.0.1:5554"
#define MSG_SERVER "Server_terminate"

/**
 * Main function that connects to a ZMQ publisher to receive and display high scores.
 *
 * This function sets up a ZMQ subscriber socket to connect to a predefined publisher
 * address. It subscribes to specific topics related to high scores and continuously
 * listens for incoming messages. Upon receiving a message, it deserializes the data
 * using Protocol Buffers and displays the high scores of players. The loop continues
 * until a specific termination message is received.
 *
 * The function handles errors in receiving and parsing messages, ensuring robust
 * communication with the publisher.
 */
int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_SUB);
    socket.connect(PUBLISHER_ADDRESS);

    // Subscribe to the "highscores" topic
    socket.set(zmq::sockopt::subscribe, MSG_SERVER);
    socket.set(zmq::sockopt::subscribe, "MSG_SCORES");
    


    while (true) {
        // Receive the topic
        zmq::message_t topic_msg;
        auto topic_recv_result = socket.recv(topic_msg, zmq::recv_flags::none);
        if (!topic_recv_result) {
            std::cerr << "Failed to receive topic." << std::endl;
            continue;
        }

        std::string topic(static_cast<char *>(topic_msg.data()), topic_msg.size());
        if (topic == MSG_SERVER) {
            break;
        }

        // Receive the message
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
        for (int i = 0; i < msg.players_size(); i++) {
            const Player &player = msg.players(i);
            if (player.id().empty()) {
                continue;
            }
            std::cout << "Player ID: " << player.id() << ", Score: " << player.score() << std::endl;
        }
    }

    socket.close();
    context.close();

    return 0;
}