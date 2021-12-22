#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


class UDPclient {
	SOCKET out{};
	sockaddr_in server{};
	int UDPport;
	std::string ipAdress;
public:
	UDPclient(int udp, std::string ip) : UDPport(udp), ipAdress(ip) {
		WSADATA data;
		WORD version = MAKEWORD(2, 2);
		int wsOk = WSAStartup(version, &data);
		if (wsOk != 0) {
			std::cout << "Can't start Winsock! " << wsOk;
			return;
		}

		server.sin_family = AF_INET;
		server.sin_port = htons(UDPport);
		inet_pton(AF_INET, ipAdress.c_str(), &server.sin_addr);

		// Socket creation, note that the socket type is datagram
		out = socket(AF_INET, SOCK_DGRAM, 0);
		if (out == SOCKET_ERROR) {

		}
	}

	// Write out to that socket
	int sendPack(size_t id, std::string& pack) {
		std::string sendBuf{ std::to_string(id) + " " + pack };
		int sendOk = sendto(out, sendBuf.c_str(), sendBuf.length() + 1, 0, (sockaddr*)&server, sizeof(server));
		std::cout << "sendto " << std::endl;

		if (sendOk == SOCKET_ERROR) {
			std::cout << "That didn't work! " << WSAGetLastError() << std::endl;
		}
		return sendOk - 1;
	}

	void closeConn() {
		closesocket(out);
		WSACleanup();
	}
	~UDPclient() {

	}
};

///////////// C L I E N T
/////////////

int main(int argc, char* argv[]) {
	
	std::string ipAddress{ argv[1] };
	int TCPport{ atoi(argv[2]) };
	int UDPport{ atoi(argv[3]) };
	std::string fileName{ argv[4] };
	int timeout{ atoi(argv[5]) };

///////////// Connect to server via TCP

	// Init winsock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Can't start winsock, ERROR# " << wsResult << std::endl;
		return 0;
	}

	// Create Winsock
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Can't create socket, ERROR# " << WSAGetLastError() << std::endl;
		WSACleanup();
	}
	
	// Fill hint
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(TCPport);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		std::cerr << "Can't connect to server. ERROR# " << WSAGetLastError() << std::endl;
		closesocket(sock);
		WSACleanup();
		return 0;
	}

	///////////// Open file
	std::ifstream ifs(fileName);
	while (!ifs.is_open()) {
		std::cout << fileName << " not found." << std::endl; /// ENTER NEW NAME
		std::cin >> fileName;
	}

	///////////// Send file name and UDP port via TCP
	char buf[1024];
	std::string fileNameAndUDPport{ fileName + " " + std::to_string(UDPport) };
	int sendResult = send(sock, fileNameAndUDPport.c_str(), fileNameAndUDPport.size() + 1, 0);
	// if (sendResult != SOCKET_ERROR) Check ERROR

	///////////// Prepare UDP

	std::stringstream buffer;
	buffer << ifs.rdbuf();
	size_t totalPacks{ (size_t)std::ceil(buffer.str().size() / 1024) };

	for (size_t i = 0; i < totalPacks;) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		UDPclient* udpConn = new UDPclient(UDPport, ipAddress);
		std::string pack{ buffer.str().substr(i * 1024, 1024) };

		///////////// Send UDP pack with id
		int conf = udpConn->sendPack(i, pack);
		///////////// Wait for TCP confirmation:

		ZeroMemory(buf, 16);

		std::cout << "about to recv " << std::endl;
		int bytesRceived = recv(sock, buf, 16, 0);
		std::cout << "bytesRceived " << bytesRceived << std::endl;
		///////////// if confirmation: next
		if (bytesRceived > 0 && conf == atoi(buf)) {
			++i;
		}
		///////////// if no confirmation within timeout: send UDP pack again
		else { continue; }
	}
///////////// Send notification to close session
	sendResult = send(sock, "EXIT", 5, 0);
	// if (sendResult != SOCKET_ERROR) Check ERROR

///////////// Close socket, exit program

	// Close
	closesocket(sock);
	WSACleanup();
	return 0;
}

