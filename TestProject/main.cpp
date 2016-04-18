#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <Windows.h>
#include <time.h>
#include "EZSockets.h"

#define PING_TIMEOUT		2000
#define PING_FREQ			650

#pragma comment(lib,"ws2_32.lib")

HWND window = GetActiveWindow();

EZSocket conn;
HANDLE thread;
clock_t ping, pong;
bool b_running;

void ClientThread(){
	Packet packtype;
	int buflength;
	while (true){
		conn.ReceivePacket(&packtype, &buflength);
		if (packtype == Packet_ERROR){
			break;
		}
		if (packtype == Packet_String){
			char* buffer = new char[buflength + 1];
			buffer[buflength] = '\0';
			conn.ReceiveData(buffer, buflength);

			Beep(2800, 300);
			std::cout << "    " << buffer << std::endl;

			delete[] buffer;
		}
		if (packtype == Packet_PING) {
			pong = clock();
		}
	}
}

void ClientPing(){
	while (true){
		if (clock() >= ping + PING_FREQ) {//Time to send ping
			conn.SendPing();
			ping = clock();
		}
		if (clock() >= pong + PING_TIMEOUT) {//If timeout
			std::cout << "Connection timed out" << std::endl;
			conn.CloseConnection();
			CloseHandle(thread);
			b_running = false;
			break;
		}
	}
}

int main(int argc, char** argv){
	std::string temp;
	std::cout << "Name: ";
	std::getline(std::cin, temp);
	system("CLS");

	InitWSA();

	if (!conn.Connect("69.23.218.202", 333)){
		std::cout << "Failed to connect" << std::endl;
		system("pause");
		conn.CloseConnection();
		WSACleanup();
		return -1;
	}
	std::cout << "Connected!" << std::endl;
	conn.SendString(temp);
	ping = clock();
	pong = clock();
	thread = MakeThread(ClientThread);
	MakeThread(ClientPing);
	conn.SendPing();

	b_running = true;
	while (b_running){
		std::getline(std::cin, temp);
		conn.SendString(temp);
		Sleep(10);
	}

	system("pause");
	WSACleanup();
	return 0;
}