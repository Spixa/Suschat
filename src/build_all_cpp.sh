#!/bin/bash
echo "Compiling server and client"
g++ server/ServerNetwork.cpp server/ServerNetwork.h server/PacketType.h server/server.cpp server/ServerObject.cpp server/ServerObject.h -o susserver -lpthread -lsfml-system -lsfml-network
g++ clients/cpp/ClientNetwork.cpp clients/cpp/ClientNetwork.h clients/cpp/PacketType.h clients/cpp/client.cpp clients/cpp/Utils.hpp -o susclient -lpthread -lsfml-system -lsfml-network 
