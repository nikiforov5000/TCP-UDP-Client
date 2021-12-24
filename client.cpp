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

void Print(std::map<int,int>& map) {
	if (map.empty()) { 
		std::cout << "tcpClient Confirmations empty" << std::endl;
		return;
	}
	std::cout << "id:" << (*map.rbegin()).first << " " << "size" << (*map.rbegin()).second << std::endl;
}

class UDPClient {
	int				m_UDPPort{};
	std::string		m_ipAddress{};
	std::ifstream	m_ifs{};
	sockaddr_in		m_server{};
	SOCKET			m_out{};
	std::string		m_fileName{};
	int				m_timeout{};
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
	//void sendPack(int id, std::string pack) {
	//	while (true) {
	//		size_t packSize{ (std::to_string(id) + " " + pack).length() };
	//		std::string sendBuf{ std::to_string(id) + " " + pack };
	//		std::cout << "about to sendFile.." << std::endl;
	//
	//		int sendOk = sendto(m_out, sendBuf.c_str(), sendBuf.length() + 1, 0, (sockaddr*)&m_server, sizeof(m_server));
	//
	//		std::this_thread::sleep_for(std::chrono::milliseconds(m_timeout));
	//		Print(Confirmations);
	//		if (
	//			!Confirmations.empty() &&
	//			Confirmations.find(id) != Confirmations.end() &&
	//			(*Confirmations.find(id)).second == packSize
	//			) {
	//			break;
	//		}
	//	}
	//}
	//void sendFile() {
	//	std::stringstream buffer;
	//	buffer << m_ifs.rdbuf();
	//	std::this_thread::sleep_for(std::chrono::milliseconds(100)); //// Wait
	//	for (size_t i = 0; i * 1024 < buffer.str().length();) {
	//		std::string pack{ buffer.str().substr(i * 1024, 1024) };
	//		auto result = std::async(std::launch::async, &UDPClient::sendPack, this, i++, pack);
	//		//sendPack(i++, pack);
	//	}
	//	std::cout << "END OF FILE" << std::endl;
	//	sendto(m_out, "-1", 3, 0, (sockaddr*)&m_server, sizeof(m_server));
	//}
	void sendPack(int id, std::string pack) {
		while (true) {
			size_t packSize{ (std::to_string(id) + " " + pack).length() };
			std::string sendBuf{ std::to_string(id) + " " + pack };
			std::cout << "about to sendFile.." << std::endl;
			
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); //// Wait
			int sendOk = sendto(m_out, sendBuf.c_str(), sendBuf.length() + 1, 0, (sockaddr*)&m_server, sizeof(m_server));
			
			auto timepoint{ std::chrono::system_clock::now() + std::chrono::milliseconds(m_timeout) };
			while (timepoint > std::chrono::system_clock::now()) {
				//Print(Confirmations);
				if (
					!Confirmations.empty() &&
					Confirmations.find(id) != Confirmations.end() &&
					(*Confirmations.find(id)).second == packSize
					) {
					return;
				}
			}
		}
	}
	void sendFile() {
		std::stringstream buffer;
		buffer << m_ifs.rdbuf();
		std::this_thread::sleep_for(std::chrono::milliseconds(50)); //// Wait
		for (size_t i = 0; i * 1024 < buffer.str().length();) {
			std::string pack{ buffer.str().substr(i * 1024, 1024) };
			sendPack(i++, pack);
			//auto result = std::async(std::launch::async, &UDPClient::sendPack, this, i++, pack);
		}
		std::cout << "END OF FILE" << std::endl;
		sendto(m_out, "-1", 3, 0, (sockaddr*)&m_server, sizeof(m_server));
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
	SOCKET m_sock;
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
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (m_sock == INVALID_SOCKET) {
			std::cerr << "Can't create socket, ERROR# " << WSAGetLastError() << std::endl;
			WSACleanup();
		}
		// Fill hint
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(TCPPort);
		inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);
		// Connect to server
		int connResult = connect(m_sock, (sockaddr*)&hint, sizeof(hint));
		if (connResult == SOCKET_ERROR) {
			std::cerr << "Can't connect to server. ERROR# " << WSAGetLastError() << std::endl;
			closesocket(m_sock);
			WSACleanup();
			//return 0;
		}
	}
	void sendInfo(std::string info) {
		int sendResult = send(m_sock, info.c_str(), info.length() + 1, 0);
	}
	std::string receiveInfo() {
		char buf[1024];
		int bytesRceived = recv(m_sock, buf, 16, 0);
		if (bytesRceived > 0) {
			return std::string(buf);
		}
		return "";
	}
	~TCPClient() {
		closesocket(m_sock);
		// Shutdown winsock
		WSACleanup();
	}	
	void receiveConfs() {
		while (m_sock != INVALID_SOCKET) {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			std::string stringBuf{ receiveInfo() };
			if (stringBuf.empty()) { continue; }
			int idConf{ std::stoi((stringBuf.substr(0, stringBuf.find(' ')))) };
			int packSizeConf{ std::stoi(stringBuf.substr(stringBuf.find(' ') + 1, stringBuf.length())) };
			Confirmations.emplace(idConf, packSizeConf);
		}
		this->~TCPClient();
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
	std::future<void> resultUdpClient{ std::async(std::launch::async, &UDPClient::connectUdp, &udpClient) };
	tcpClient.receiveConfs();
	resultUdpClient.get();
	
	tcpClient.~tcpClient();
}

