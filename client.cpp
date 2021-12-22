#include <chrono>
#include <iostream>
#include <fstream>
#include <future>
#include <map>
#include <string>
#include <sstream>
#include <thread>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

std::map<int, int> Confirmations;

class UDPClient {
	int m_UDPPort;
	std::string m_ipAddress;
	std::ifstream m_ifs;
	sockaddr_in m_server;
	SOCKET m_out;
	std::string m_fileName;
	int m_timeout;
public:
	UDPClient(std::string& ip, int port, std::string& filename, int timeout) 
	  : m_UDPPort(port), 
		m_ipAddress(ip),
		m_fileName(filename), 
		m_timeout(timeout)
	{};
	void openFile() {
		m_ifs.open(m_fileName);
		while (!m_ifs.is_open()) {
			std::cout << m_fileName << " not found. Enter new name:" << std::endl;
			std::cin >> m_fileName;
		}
	}
	void sendPack(int id, std::string pack) {
		while (true) {
			size_t packSize{ (std::to_string(id) + " " + pack).length() };
			std::string sendBuf{ std::to_string(id) + " " + pack };
			int sendOk = sendto(m_out, sendBuf.c_str(), sendBuf.length() + 1, 0, (sockaddr*)&m_server, sizeof(m_server));
			std::this_thread::sleep_for(std::chrono::milliseconds(m_timeout));
			if ((*Confirmations.find(id)).second == sendOk) { break; }
		}
	}
	void sendFile() {
		std::stringstream buffer;
		buffer << m_ifs.rdbuf();
		size_t totalPacks{ (size_t)std::ceil(buffer.str().size() / 1024) };
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //// Wait
		for (size_t i = 0; i < totalPacks;) {
			std::string pack{ buffer.str().substr(i * 1024, 1024) };
			//auto result = std::async(std::launch::async, &UDPClient::sendPack, this, i, pack);
			sendPack(i, pack);
		}
	}
	void connectUdp() {
		WSADATA data;
		WORD version = MAKEWORD(2, 2);
		int wsOk = WSAStartup(version, &data);
		if (wsOk != 0) {
			std::cout << "Can't start Winsock! " << wsOk;
			return;
		}
		m_server.sin_family = AF_INET;
		m_server.sin_port = htons(m_UDPPort);
		inet_pton(AF_INET, m_ipAddress.c_str(), &m_server.sin_addr);
		m_out = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_out == SOCKET_ERROR) {

		}
		openFile();
		sendFile();
	}
};

class TCPClient {
	SOCKET sock;
	std::string ipAddress;
	int TCPPort;
public:
	TCPClient(std::string& ip, int port) : ipAddress(ip), TCPPort(port){
		// Init winsock
		WSAData data;
		WORD ver = MAKEWORD(2, 2);
		int wsResult = WSAStartup(ver, &data);
		if (wsResult != 0) {
			std::cerr << "Can't start winsock, ERROR# " << wsResult << std::endl;
		}
		// Create Winsock
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) {
			std::cerr << "Can't create socket, ERROR# " << WSAGetLastError() << std::endl;
			WSACleanup();
		}
		// Fill hint
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(TCPPort);
		inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
		// Connect to server
		int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
		if (connResult == SOCKET_ERROR) {
			std::cerr << "Can't connect to server. ERROR# " << WSAGetLastError() << std::endl;
			closesocket(sock);
			WSACleanup();
			//return 0;
		}
	}
	void sendInfo(std::string info) {
		int sendResult = send(sock, info.c_str(), info.length() + 1, 0);
	}
	void receiveInfo() {
		char buf[1024];
		int bytesRceived = recv(sock, buf, 16, 0);
		if (bytesRceived > 0) {
			std::string stringBuf{ std::string(buf) };
			int idConf{ std::stoi((stringBuf.substr(0, stringBuf.find(' ')))) };
			int packSizeConf { std::stoi(stringBuf.substr(stringBuf.find(' ') + 1, stringBuf.length())) };
			Confirmations.emplace(idConf, packSizeConf);
		}
	}
};

int main(int argc, char* argv[]) {

	std::string ip			{ argv[1] };
	int tcpPort				{ atoi(argv[2])};
	int udpPort				{ atoi(argv[3])};
	std::string fileName	{ argv[4]};
	int timeout				{ atoi(argv[5])};

	TCPClient tcpClient(ip, tcpPort);
	tcpClient.sendInfo(std::to_string(udpPort) + " " + fileName);
	UDPClient udpClient(ip, udpPort, fileName, timeout);
	//std::future<void> result{ std::async(std::launch::async, &UDPClient::connectUdp, &udpClient) };
	udpClient.connectUdp();
	//result.get();
	tcpClient.sendInfo("disconnect");
	tcpClient.~tcpClient();
}

