#include <iostream>
#include <string>
#include <WinSock2.h>
#include <Windows.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"ws2_32.lib")

#define MAX_CONNECTIONS		45

HWND window = GetActiveWindow();

SOCKET connections[MAX_CONNECTIONS]; std::string names[MAX_CONNECTIONS];
unsigned int connectCount = 0;

enum Packet{
	Packet_ERROR,
	Packet_String,
	Packet_Int
};

void SendToClient(const char* msg, int length, int index, Packet packet = Packet_String){
	if (send(connections[index], (char*)&packet, sizeof(Packet), NULL) == SOCKET_ERROR){
		return;
	}
	send(connections[index], (char*)&length, sizeof(int), NULL);
	send(connections[index], msg, length, NULL);
}

void SendToAll(const char* msg, int length, int except = -1){
	for (int i = 0; i < MAX_CONNECTIONS; i++){
		if (connections[i] == 0){ continue; }
		if (i == except){ continue; }
		SendToClient(msg, length, i);
	}
}

int ReceiveSize(int index){
	int length;
	recv(connections[index], (char*)&length, sizeof(int), NULL);
	return length;
}

void ReceiveData(char* msg, int length, int index){
	recv(connections[index], msg, length, NULL);
}

void ClientHandlerThread(int index){
	int bufferlength;
	Packet packtype;
	std::string temp;
	while (true){
		temp.clear();

		if (recv(connections[index], (char*)&packtype, sizeof(Packet), NULL) == SOCKET_ERROR){
			temp.append(names[index] + " has disconnected.");
			std::cout << temp << std::endl;
			SendToAll(temp.c_str(), temp.size(), index);
			break;
		}
		if (packtype != Packet_String){
			break;
		}

		bufferlength = ReceiveSize(index);
		char* buffer = new char[bufferlength + 1];
		buffer[bufferlength] = '\0';
		ReceiveData(buffer, bufferlength, index);

		if (names[index] == ""){
			names[index] = buffer;
			temp.append(names[index] + " has joined the chat");
			std::cout << names[index] << " has joined the chat (" << connectCount << "/100)" << std::endl;
			SendToAll(temp.c_str(), temp.size(), index);
			continue;
		}
		temp.append(names[index] + ": " + buffer);
		std::cout << temp << std::endl;
		SendToAll(temp.c_str(), temp.size(), index);

		delete[] buffer;
	}
	connectCount--;
	closesocket(connections[index]);
	connections[index] = 0;
	names[index] = "";
}

int main(int argc, char** argv){
	WORD sockV;
	WSADATA wsaData;
	sockV = MAKEWORD(2, 1);
	WSAStartup(sockV, &wsaData);

	SOCKADDR_IN partner;
	int partnerlen = sizeof(partner);
	partner.sin_family = AF_INET;
	partner.sin_addr.s_addr = inet_addr("192.168.0.4");
	partner.sin_port = htons(333);

	SOCKET listenSoc = socket(AF_INET, SOCK_STREAM, NULL);
	bind(listenSoc, (SOCKADDR*)&partner, sizeof(partner));
	listen(listenSoc, SOMAXCONN);

	SOCKET newConn;

	ZeroMemory(connections, sizeof(SOCKET)*MAX_CONNECTIONS);
	std::cout << "Running..." << std::endl;
	while (true){
		newConn = accept(listenSoc, (SOCKADDR*)&partner, &partnerlen);
		if (newConn == 0){
			std::cout << "Connection failed" << std::endl;
			system("pause");
			closesocket(listenSoc);
			closesocket(newConn);
			WSACleanup();
			return -1;
		}
		int index = -1;
		for (int i = 0; i < 100; i++){
			if (connections[i] == 0){
				index = i;
				break;
			}
		}
		if (index == -1){
			std::cout << "Chatroom full: a connection has been denied." << std::endl;
			continue;
		}
		connections[index] = newConn;
		connectCount++;

		std::string MOTD;
		MOTD.append("(" + std::to_string(connectCount) + "/100)");
		SendToClient(MOTD.c_str(), MOTD.size(), index);

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandlerThread, (LPVOID)(index), NULL, NULL);
	}
	system("pause");
	closesocket(listenSoc);
	closesocket(newConn);
	WSACleanup();
}