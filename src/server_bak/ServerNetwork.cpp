#include "ServerNetwork.h"
#include <string>
#include <sstream>
#include "PacketType.h"

ServerNetwork::ServerNetwork(unsigned short port, bool rawMode = false) : listen_port(port) {
    logl("Server: Server has begun on port " + std::to_string(port));

    this->rawMode = rawMode;
    if (rawMode) logl("Warning > Raw mode is not recommended.");

    if (listener.listen(listen_port) != sf::Socket::Done) {
        logl("Error > Could not establish listener.");
    }
}

void ServerNetwork::connectClients(std::vector<sf::TcpSocket*>* client_array) {
    while (true) {
        sf::TcpSocket* new_client = new sf::TcpSocket();
        if (listener.accept(*new_client) == sf::Socket::Done) {
            new_client->setBlocking(false);
            client_array->push_back(new_client);
            logl("Server > Client " << new_client->getRemoteAddress() << ":" << new_client->getRemotePort() << " on client slot " << client_array->size());
            
            // New code; subject to change.
            sf::Packet joinPacket; 
            joinPacket << static_cast<unsigned short>(PacketType::JoinPacket) << 
            static_cast<unsigned short>(MessageType::NotMessageType) <<    
            new_client->getRemoteAddress().toString() + ":" + std::to_string(new_client->getRemotePort());
            broadcastPacket(joinPacket,new_client->getRemoteAddress(),new_client->getRemotePort());
        }
        else {
            logl("Server failed to accept new sockets\n\trestart the server");
            delete(new_client);
            break;
        }
    }
}

void ServerNetwork::disconnectClient(sf::TcpSocket* socket_pointer, size_t position) {
    logl("Server > Client " << socket_pointer->getRemoteAddress() << ":" << socket_pointer->getRemotePort() << " disconnected, removing");

    // New code; subject to change.
    // Not message type: Not a message type, another type of packet
    sf::Packet leavePacket;
    leavePacket << static_cast<unsigned short>(PacketType::LeavePacket) <<
    static_cast<unsigned short>(MessageType::NotMessageType)
    << socket_pointer->getRemoteAddress().toString() + ":" + std::to_string(socket_pointer->getRemotePort());

    broadcastPacket(leavePacket, socket_pointer->getRemoteAddress(), socket_pointer->getRemotePort());


    socket_pointer->disconnect();
    delete(socket_pointer);
    client_array.erase(client_array.begin() + position);
}

void ServerNetwork::sendPacket(sf::Packet& packet,sf::IpAddress receiver, unsigned short port) {
        for (size_t iterator = 0; iterator < client_array.size(); iterator++) {
        sf::TcpSocket* client = client_array[iterator];
        if (client->getRemoteAddress() == receiver || client->getRemotePort() == port) {
            if (client->send(packet) != sf::Socket::Done) {
                logl("Could not send packet on broadcast");
            }
        }
    }
}

void ServerNetwork::broadcastPacket(sf::Packet& packet, sf::IpAddress exclude_address, unsigned short port) {
    for (size_t iterator = 0; iterator < client_array.size(); iterator++) {
        sf::TcpSocket* client = client_array[iterator];
        if (client->getRemoteAddress() != exclude_address || client->getRemotePort() != port) {
            if (client->send(packet) != sf::Socket::Done) {
                logl("Could not send packet on broadcast");
            }
        }
    }
}

void ServerNetwork::broadcastRawData(const char* data, sf::IpAddress exclude_address, unsigned short port) {
    for (size_t iterator = 0; iterator < client_array.size(); iterator++) {
        sf::TcpSocket* client = client_array[iterator];
        if (client->getRemoteAddress() != exclude_address || client->getRemotePort() != port) {
            if (client->send(data, sizeof(data)) != sf::Socket::Done) {
                logl("Could not send packet on broadcast");
            }
        }
    }
}

void ServerNetwork::receiveRawData(sf::TcpSocket* client, size_t iterator) {
    char received_data[MAX_RAW_DATA]; size_t received_bytes;
    memset(received_data, 0, sizeof(received_data));
    if (client->receive(received_data, sizeof(received_data), received_bytes) == sf::Socket::Disconnected) {
        disconnectClient(client, iterator);
    }
    else if (received_bytes > 0) {
        broadcastRawData(received_data, client->getRemoteAddress(), client->getRemotePort());
        logl(client->getRemoteAddress().toString() << ":" << client->getRemotePort() << " '" << received_data << "'");
    }
}

void ServerNetwork::receivePacket(sf::TcpSocket* client, size_t iterator) {
    sf::Packet packet;
    if (client->receive(packet) == sf::Socket::Disconnected) {
        disconnectClient(client, iterator);
    }
    else {
        if (packet.getDataSize() > 0) {
            std::string received_message,log_packettype;
            PacketType type;
            unsigned short type_int, messagetype_int;
            packet >> type_int >> messagetype_int >> received_message;
            if (received_message == "") {
                logl("Declined someone to send null message.");
                sf::Packet failPacket;
                failPacket << static_cast<unsigned short>(PacketType::MessagePacket) << static_cast<unsigned short>(MessageType::DirectMessage) <<
                "Bruh stop sending empty shit, its annoying asf"
                << "Server"
                << listener.getLocalPort();
                sendPacket(failPacket, client->getRemoteAddress(),client->getRemotePort());
                return;
            }
            packet.clear();

            type = static_cast<PacketType>(type_int);

            // Handle packet type deprartment (beta code)
            if (type == PacketType::MessagePacket) {
                packet << static_cast<unsigned short>(PacketType::MessagePacket)
                    << messagetype_int
                    << received_message
                    << client->getRemoteAddress().toString()
                    << client->getRemotePort();
                log_packettype = "MessagePacket";
                
            }
            if (type == PacketType::IdentifyPacket) {
                clientid_array[client_array.size()] = received_message;
                log_packettype = "IdentityPacket";
            }
            broadcastPacket(packet, client->getRemoteAddress(), client->getRemotePort());
            logl(client->getRemoteAddress().toString() << ":" << client->getRemotePort() << " sent " << log_packettype << ": \"" << received_message << "\"");
        }
    }
}

void ServerNetwork::managePackets() {
    while (true) {
        for (size_t iterator = 0; iterator < client_array.size(); iterator++) {
            if (rawMode == true) receiveRawData(client_array[iterator], iterator);
            else receivePacket(client_array[iterator], iterator);
        }

        std::this_thread::sleep_for((std::chrono::milliseconds)100);
    }
}

void ServerNetwork::run() {
    std::thread connetion_thread(&ServerNetwork::connectClients, this, &client_array);

    managePackets();
}