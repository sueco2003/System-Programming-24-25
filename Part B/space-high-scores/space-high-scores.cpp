#include <zmq.hpp>
#include "astronauts.pb.h"

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5559");

    while (true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);

        // Deserialize the data
        Astronauts astronauts_msg;
        astronauts_msg.ParseFromArray(request.data(), request.size());

        // Process astronauts (e.g., print them)
        for (const auto &astronaut : astronauts_msg.astronauts()) {
            std::cout << "Astronaut ID: " << astronaut.id() << ", Score: " << astronaut.score() << std::endl;
        }

        // Respond with an acknowledgment
        zmq::message_t reply(5);
        memcpy(reply.data(), "ACK", 4);
        socket.send(reply, zmq::send_flags::none);
    }

    return 0;
}
