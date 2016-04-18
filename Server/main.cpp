#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <Windows.h>
#include <time.h>
#include "EZSockets.h"

#define PING_TIMEOUT		2000
#define PING_FREQ			650

#pragma comment(lib,"ws2_32.lib")

#define MAX_CONNECTIONS		45

HWND window = GetActiveWindow();

EZSocket connections[MAX_CONNECTIONS]; std::string names[MAX_CONNECTIONS];
HANDLE threads[MAX_CONNECTIONS]; clock_t pings[MAX_CONNECTIONS], pongs[MAX_CONNECTIONS];
unsigned int connectCount = 0;

void SendToAll(std::string msg, int except = -1){
	for (int i = 0; i < MAX_CONNECTIONS; i++){
		if (*(connections[i].GetSocket()) == 0){ continue; }
		if (i == except){ continue; }
		connections[i].SendString(msg);
	}
}

void ClientHandlerThread(int index){
	int bufferlength;
	Packet packtype;
	std::string temp;
	while (true){
		temp.clear();
		connections[index].ReceivePacket(&packtype, &bufferlength);
		if (packtype == Packet_ERROR){
			break;
		}
		if (packtype == Packet_PING){
			pings[index] = clock();
		}
		if (packtype == Packet_String){
			char* buffer = new char[bufferlength + 1];
			buffer[bufferlength] = '\0';
			connections[index].ReceiveData(buffer, bufferlength);

			if (names[index] == ""){
				names[index] = buffer;
				temp.append(names[index] + " has joined the chat");
				std::cout << names[index] << " has joined the chat (" << connectCount << "/" << MAX_CONNECTIONS << ")" << std::endl;
				SendToAll(temp, index);
				continue;
			}
			temp.append(names[index] + ": " + buffer);
			std::cout << temp << std::endl;
			SendToAll(temp, index);

			delete[] buffer;
		}
	}
}

void ClientPing(int index){
	while (true){
		if (clock() >= pongs[index] + PING_FREQ) {//Time to send pong
			connections[index].SendPing();
			pongs[index] = clock();
		}
		if (clock() >= pings[index] + PING_TIMEOUT) {//If timeout
			break;
		}
	}
	std::string temp = names[index] + " has disconnected.";
	std::cout << temp << std::endl;
	SendToAll(temp, index);
	connectCount--;
	connections[index].CloseConnection();
	names[index] = "";
	CloseHandle(threads[index]);
}

int main(int argc, char** argv){
	InitWSA();

	EZSocket server, newConn2;
	server.Listen("192.168.0.4", 333);

	ZeroMemory(connections, sizeof(EZSocket)*MAX_CONNECTIONS);
	std::cout << "Running..." << std::endl;
	while (true){
		newConn2.SetSocket(server.Accept());

		if (*(newConn2.GetSocket()) == 0){
			std::cout << "Connection failed" << std::endl;
			system("pause");
			server.CloseConnection();
			newConn2.CloseConnection();
			WSACleanup();
			return -1;
		}
		int index = -1;
		for (int i = 0; i < MAX_CONNECTIONS; i++){
			if (*(connections[i].GetSocket()) == 0){
				index = i;
				break;
			}
		}
		if (index == -1){
			std::cout << "Chatroom full: a connection has been denied." << std::endl;
			return 0;
		}
		connections[index] = newConn2;
		connectCount++;

		std::string MOTD;
		MOTD.append("(" + std::to_string(connectCount) + "/" + std::to_string(MAX_CONNECTIONS) + ")");

		newConn2.SendString(MOTD);
		pings[index] = clock();
		pongs[index] = clock();
		threads[index] = MakeThread(ClientHandlerThread, index);
		MakeThread(ClientPing, index);
	}
	system("pause");
	server.CloseConnection();
	newConn2.CloseConnection();
	WSACleanup();
	return 1;
}