#include"client.h"

/*
客户端的总体流程：
1.三次握手
三次握手的总体流程是：首先，客户端给服务器发送SYN包，然后服务器收到之后就给客户端发送SYN+ACK包，然后客户端再发送ACK包。
所以，客户端要编写的功能为：发送SYN包，接收和处理SYN+ACK包，发送ACK包

2.数据传输

3.两次挥手



*/

// 发送数据包的通用函数，返回是否发送成功
bool sendPacket(char* send_buff, int buff_size, SOCKET socket, sockaddr_in& addr, const char* error_msg_prefix) {
	int log = sendto(socket, send_buff, buff_size, 0, (sockaddr*)&addr, sizeof(sockaddr_in));
	if (log == SOCKET_ERROR) {
		std::cout << error_msg_prefix << "Failed to send packet!" << std::endl;
		std::cout << GetLastErrorDetails() << std::endl;
		std::cout << "Please try again later!" << std::endl;
		return false;
	}
	return true;
}

// 发送SYN包的函数
bool sendSYN(Header& syn_header, SOCKET clientSocket, sockaddr_in& serverAddr) {
	char* send_buff = new char[sizeof(Header)];
	memset(send_buff, 0, sizeof(Header));
	memcpy(send_buff, (char*)&syn_header, sizeof(syn_header));

	u_short cks = checksum(send_buff, sizeof(syn_header));
	((Header*)send_buff)->checksum = cks;

	return sendPacket(send_buff, sizeof(syn_header), clientSocket, serverAddr, "Oops! Failed to send SYN to server.");
}

// 发送ACK包的函数
bool sendACK(Header& ack_header, SOCKET clientSocket, sockaddr_in& serverAddr) {
	char* send_buff = new char[sizeof(Header)];
	memset(send_buff, 0, sizeof(Header));
	memcpy(send_buff, (char*)&ack_header, sizeof(ack_header));

	u_short cks = checksum(send_buff, sizeof(ack_header));
	((Header*)send_buff)->checksum = cks;

	return sendPacket(send_buff, sizeof(ack_header), clientSocket, serverAddr, "Oops! Failed to send ACK to server.");
}

// 处理发送SYN包失败的重试逻辑
void handleSendSYNFailure(int& times, SOCKET clientSocket, Header& syn_header, sockaddr_in& serverAddr) {
	if (times == 0) {
		std::cout << "Failed to send SYN pkg to server too many times!" << std::endl;
		std::cout << "------------Connection Lost!-----------" << std::endl;
		exit(-1);
	}
	times--;
}

// 处理接收SYN + ACK包超时的逻辑
void handleReceiveTimeoutSYNACK(clock_t& start, int& max_retries_times, SOCKET clientSocket, Header& syn_header, sockaddr_in& serverAddr, int& udp_2msl) {
	if (max_retries_times <= 0) {
		std::cout << "Reached max times on resending SYN!" << std::endl;
		std::cout << "Shaking Hands Failed!" << std::endl;
		std::cout << "-----------Stop Shaking Hands!-----------" << endl;
		exit(-1);
	}

	int retry_times = 5;
	while (retry_times > 0) {
		if (!sendSYN(syn_header, clientSocket, serverAddr)) {
			handleSendSYNFailure(retry_times, clientSocket, syn_header, serverAddr);
		}
		else {
			std::cout << "Timeout, resent SYN to server!" << std::endl;
			break;
		}
	}

	max_retries_times--;
	udp_2msl += MSL; 
	start = clock();
	std::cout << "-----the second hand-shaking-----" << std::endl;
}

// 执行握手操作的主函数
int shake_hand() {
	max_retries_times = UDP_SHAKE_RETRIES;
	udp_2msl = 2 * MSL;
	char* send_buff = new char[sizeof(Header)];
	char* recv_buff = new char[sizeof(Header) + MSS];
	int log; 

	std::cout << "-----------Start Shaking Hands...-----------" << std::endl;
	std::cout << "-----the first hand-shaking-----" << std::endl;

	Header syn_header(0, 0, SYN, 0, 0, sizeof(Header));

	int randomNumber = rand() % 1;
	if (randomNumber == 0) {
		std::cout << "------------simulate to drop the data for server!-----------" << std::endl;
	}
	else {
		int times = 5;
		// 发送SYN包
		while (times > 0) {
			if (!sendSYN(syn_header, clientSocket, serverAddr)) {
				handleSendSYNFailure(times, clientSocket, syn_header, serverAddr);
			}
			else {
				std::cout << "Successfully sent SYN pkg to server, a request of connection!" << std::endl;
				break;
			}
		}
		std::cout << "-----the second hand-shaking-----" << std::endl;
	}

	clock_t start = clock();
	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);
	sockaddr_in tempAddr;
	int temp_addr_length = sizeof(sockaddr_in);

	while (true) {
		// 等待接收SYN + ACK包
		while (recvfrom(clientSocket, recv_buff, sizeof(Header), 0, (sockaddr*)&tempAddr, &temp_addr_length) <= 0) {
			if (clock() - start > 1.2 * udp_2msl) {
				handleReceiveTimeoutSYNACK(start, max_retries_times, clientSocket, syn_header, serverAddr, udp_2msl);
			}
		}

		// 检查接收到的包是否为有效的SYN & ACK包
		Header recv_header;
		memcpy(&recv_header, recv_buff, sizeof(recv_header));
		u_short cks = checksum(recv_buff, sizeof(recv_header));
		if (cks == 0 && (recv_header.get_flag() & ACK) && (recv_header.get_flag() & SYN)) {
			std::cout << "Successfully received SYN & ACK pkg from server!" << std::endl;
			std::cout << "-----the third hand-shaking-----" << std::endl;

			Header ack_header(0, 0, ACK, 0, 0, sizeof(Header));
			if (!sendACK(ack_header, clientSocket, serverAddr)) {
				int ack_retries = 5;
				while (ack_retries > 0) {
					if (!sendACK(ack_header, clientSocket, serverAddr)) {
						std::cout << "Oops! Failed to send ACK to server!" << std::endl;
						std::cout << GetLastErrorDetails() << std::endl;
						std::cout << "Please try again later!" << std::endl;
						ack_retries--;
						if (ack_retries == 0) {
							std::cout << "Failed to send ACK to server too many times!" << std::endl;
							return -1;
						}
					}
					else {
						break;
					}
				}
			}

			// 切换到阻塞模式
			mode = 0;
			ioctlsocket(clientSocket, FIONBIO, &mode);

			std::cout << "Successfully sent ACK pkg to Server!" << std::endl;
			std::cout << "-----------Finished Shaking Hands!-----------" << endl;
			return 1;
		}
	}
}

bool Client_Send_Message(char* data_buff, int pkg_length, bool last_pkg) {
	int flag = last_pkg ? LAS : 0;
	int log;
	bool result = true;
	Header send_header(sequence_num, 0, flag, 0, pkg_length, sizeof(Header));
	send_buff = new char[sizeof(send_header) + pkg_length];
	recv_buff = new char[sizeof(Header)];
	memcpy(send_buff, (char*)&send_header, sizeof(send_header));
	memcpy(send_buff + sizeof(send_header), data_buff, pkg_length);	
	u_short cks = checksum(send_buff, pkg_length + sizeof(send_header));
	((Header*)send_buff)->checksum = cks;

	if (Latency_mill_seconds) {

		Sleep(Latency_mill_seconds);
	}

	int randomNumber = rand() % 100; 

	if (randomNumber < Packet_loss_range) {
		cout << "------------simulate to drop the data for server!-----------" << endl;
	}
	else {
		cout << "-----New Data Begin-----" << endl;
		log = sendto(
			clientSocket,
			send_buff,
			pkg_length + sizeof(send_header),
			0,
			(sockaddr*)&serverAddr,
			sizeof(sockaddr_in)
		);


		cout << "Successfully sent data---" << send_header.get_data_length() + send_header.get_header_length() << "Bytes." << endl;
		cout << "【seq】" << send_header.get_seq() << " ,【ack】" << send_header.get_ack() << ",【flag】" << send_header.get_flag() << ",【checksum】" << send_header.get_checksum() << endl;
		cout << "【header length】" << send_header.get_header_length() << ",【data length】" << send_header.get_data_length() << endl;
	}

	clock_t start = clock();

	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);
	while (true) {
		sockaddr_in tempAddr;
		Header recv_header;
		int temp_addr_length = sizeof(sockaddr_in);
		while (recvfrom(clientSocket,recv_buff,sizeof(recv_header),0,(sockaddr*)&tempAddr,&temp_addr_length) <= 0) {
			if (clock() - start > 1.2 * udp_2msl) {

				int random = rand() % 100;
				if (random < Packet_loss_range) {
					cout << "------------simulate to drop the data for server!-----------" << endl;
				}
				else {
					log = sendto(
						clientSocket,
						send_buff,
						pkg_length + sizeof(send_header),
						0,
						(sockaddr*)&serverAddr,
						sizeof(sockaddr_in)
					);

					cout << "Timeout, resent datagram to server！" << endl;
				}
				start = clock();
			}
		}


		int random_number = rand() % 100;
		if (random_number < Packet_loss_range) {
			cout << "------------RELATIVE DELAY TIME-----------" << endl;
			udp_2msl = udp_2msl * Latency_param;

		}
		memcpy(&recv_header, recv_buff, sizeof(recv_header));
		cout << "Successfully receive data---" << recv_header.get_data_length() + recv_header.get_header_length() << "Bytes." << endl;
		cout << "【seq】" << recv_header.get_seq() << " ,【ack】" << recv_header.get_ack() << ",【flag】" << recv_header.get_flag() << ",【checksum】" << recv_header.get_checksum() << endl;
		cout << "【header length】" << recv_header.get_header_length() << ",【data length】" << recv_header.get_data_length() << endl;
		u_short cks = checksum(recv_buff, sizeof(recv_header));
		if (cks == 0
			&&
			(recv_header.get_flag() & ACK)
			&&
			(recv_header.get_ack() == sequence_num)
			) {
			cout << "Server has acknowleged the data！" << endl;
			break;
		}
		else if (
			cks == 0
			&&
			recv_header.get_flag() & RST
			) {
			cout << "Server unexpected closed:Error in connection..." << endl;

			result = false;
			break;
		}
		else if (
			cks != 0
			)
			continue;

	}
	mode = 0;
	ioctlsocket(clientSocket, FIONBIO, &mode);
	sequence_num ^= 1;
	return result;
}

void send_data(string file_path) {
	ifstream file(file_path.c_str(), ifstream::binary);
	file.seekg(0, file.end);
	file_length = file.tellg();
	file.seekg(0, file.beg);
	int total_length = file_path.length() + file_length + 1;
	memset(file_data_buffer, 0, sizeof(char) * total_length);

	memcpy(file_data_buffer, file_path.c_str(), file_path.length());

	file_data_buffer[file_path.length()] = '?';

	file.read(file_data_buffer + file_path.length() + 1, file_length);
	file.close();
	cout << "-----------Start Sending File...-----------" << endl;
	clock_t start = clock();

	int curr_pos = 0;
	int log;

	while (curr_pos < total_length) {
		int pkg_length = total_length - curr_pos >= MSS ? MSS : total_length - curr_pos;
		bool last = total_length - curr_pos <= MSS ? true : false;
		log = Client_Send_Message(file_data_buffer + curr_pos, pkg_length, last);
		if (!log) {

			delete[] file_data_buffer;
			delete[] send_buff;
			delete[] recv_buff;
			closesocket(clientSocket);
			WSACleanup();
			exit(0);
		}
		curr_pos += MSS;
	}
	clock_t end = clock();
	cout << "-----------Finished Sending File!-----------" << endl;
	cout << "Successfully sent file: " + file_path + " to server!" << endl;
	cout << "-----------Result:----------" << endl;
	cout << "Total sending length:" << total_length << " Bytes." << endl;
	cout << "Total time:" << (end - start) * 1000 / (double)CLOCKS_PER_SEC << " ms." << endl;
	if (!(end - start))
		cout << "Flash!Time is too short to compute a throughput!" << endl;
	else
		cout << "Throughput:" << total_length / ((end - start) * 1000 / (double)CLOCKS_PER_SEC) << "Bytes/ms." << endl;

	return;
}

// 构建并准备FIN数据包
void buildAndPrepareFINPacket(Header& fin_header, char*& send_buff) {
	u_short random_seq = rand() % MAX_SEQ;
	fin_header = Header(random_seq, 0, FIN, 0, 0, sizeof(Header));

	send_buff = new char[sizeof(Header)];
	memset(send_buff, 0, sizeof(Header));
	memcpy(send_buff, (char*)&fin_header, sizeof(fin_header));

	u_short cks = checksum(send_buff, sizeof(fin_header));
	((Header*)send_buff)->checksum = cks;
}

// 处理发送FIN包前的模拟丢包逻辑
void handleSendFINPacketDrop(int& randomNumber) {
	randomNumber = rand() % 1;
	if (randomNumber == 0) {
		std::cout << "------------simulate to drop the data for server!-----------" << std::endl;
	}
	else {
		std::cout << "-----New Data Begin-----" << endl;
	}
}

// 发送FIN数据包并打印相关日志
void sendFINPacketAndLog(Header& fin_header, char* send_buff, SOCKET clientSocket, sockaddr_in& serverAddr) {
	int log;
	log = sendto(
		clientSocket,
		send_buff,
		sizeof(fin_header),
		0,
		(sockaddr*)&serverAddr,
		sizeof(sockaddr_in)
	);
	if (log == SOCKET_ERROR) {

	}
	else {
		std::cout << "Successfully sent FIN pkg to server!" << std::endl;
		std::cout << "Sequence Number:" << fin_header.get_seq() << ", expects acknowledge number:" << fin_header.get_seq() + 1 << "." << endl;
	}
}

// 处理接收ACK超时并重发FIN包的逻辑
void handleReceiveACKTimeout(clock_t& start, int& max_retries_times, char* send_buff, SOCKET clientSocket, sockaddr_in& serverAddr, int& udp_2msl) {
	if (clock() - start > 1.2 * udp_2msl) {
		if (max_retries_times <= 0) {
			std::cout << "Reached max times on resending FIN." << std::endl;
			std::cout << "Waving Hands Failed!" << std::endl;
			std::cout << "-----------Stop Waving Hands...-----------" << endl;
			u_long mode = 0;
			ioctlsocket(clientSocket, FIONBIO, &mode);
			return;
		}
		int log;
		log = sendto(
			clientSocket,
			send_buff,
			sizeof(Header),
			0,
			(sockaddr*)&serverAddr,
			sizeof(sockaddr_in)
		);

		max_retries_times--;

		start = clock();
		std::cout << "Timeout, resent FIN pkg to server." << std::endl;
		std::cout << "Sequence Number:" << ((Header*)send_buff)->get_seq() << ", expects acknowledge number:" << ((Header*)send_buff)->get_seq() + 1 << "." << endl;

		std::cout << "-----the second hand-waving-----" << endl;
	}
}

// 检查接收到的ACK包是否有效
bool checkReceivedACKValidity(Header& recv_header, u_short random_seq) {
	u_short cks = checksum(recv_buff, sizeof(recv_header));
	if (
		cks == 0
		&&
		recv_header.get_flag() == ACK
		&&
		recv_header.get_ack() == (random_seq + 1) % MAX_SEQ
		) {
		return true;
	}
	return false;
}

// 执行挥手操作的主函数
void wave_hand() {
	max_retries_times = UDP_WAVE_RETRIES;
	udp_2msl = 2 * MSL;
	char* send_buff = nullptr;
	char* recv_buff = new char[sizeof(Header)];
	int log;

	std::cout << "-----------Start Waving Hands...-----------" << std::endl;
	std::cout << "-----the first hand-waving-----" << endl;

	// 构建并准备FIN数据包
	Header fin_header;
	buildAndPrepareFINPacket(fin_header, send_buff);

	// 处理发送FIN包前的模拟丢包逻辑
	int randomNumber;
	handleSendFINPacketDrop(randomNumber);
	if (randomNumber == 0) {
		std::cout << "------------simulate to drop the data for server!-----------" << std::endl;
	}

	// 发送FIN数据包并打印相关日志
	sendFINPacketAndLog(fin_header, send_buff, clientSocket, serverAddr);

	std::cout << "-----the second hand-waving-----" << endl;

	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);
	sockaddr_in tempAddr;
	int temp_addr_length = sizeof(sockaddr_in);

	Header recv_header;

	clock_t start = clock();

	while (true) {
		while (recvfrom(
			clientSocket,
			recv_buff,
			sizeof(recv_header),
			0,
			(sockaddr*)&tempAddr,
			&temp_addr_length
		) <= 0) {
			// 处理接收ACK超时并重发FIN包的逻辑
			handleReceiveACKTimeout(start, max_retries_times, send_buff, clientSocket, serverAddr, udp_2msl);
		}

		memcpy(&recv_header, recv_buff, sizeof(recv_header));
		if (checkReceivedACKValidity(recv_header, fin_header.get_seq())) {
			std::cout << "Successfully received ACK with acknowledge:" << recv_header.get_ack() << ", in respond to Fin pkg" << endl;
			std::cout << "-----------Finished Waving Hands!-----------" << endl;
			mode = 0;
			ioctlsocket(clientSocket, FIONBIO, &mode);
			return;
		}
		else {
			continue;
		}
	}
}

// 初始化Winsock
bool initializeWinsock() {
	std::cout << "-----------Initializing Winsock...-----------" << std::endl;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		std::cout << "Oops!Failed to initialize Winsock!" << std::endl;
		std::cout << GetLastErrorDetails() << std::endl;
		std::cout << "Please try again!" << std::endl;
		return false;
	}
	std::cout << "Successfully initialized Winsock!" << std::endl;
	return true;
}

// 创建套接字
bool createSocket() {
	std::cout << "-----------Creating Socket...-----------" << std::endl;
	// 使用IPv4地址族，数据报套接字（UDP）
	clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		std::cout << "Oops!Failed to create socket!" << std::endl;
		std::cout << GetLastErrorDetails() << std::endl;
		std::cout << "Please try again!" << std::endl;
		WSACleanup(); // 释放资源
		return false;
	}
	std::cout << "Successfully created socket!" << std::endl;
	return true;
}

// 配置服务器地址信息
void configureServerAddress() {
	serverAddr.sin_family = AF_INET; // 设置为IPv4
	inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr)); // 设置IPv4地址为127.0.0.1
	serverAddr.sin_port = htons(2333); // 设置端口为2333
}

// 配置客户端地址信息并绑定套接字
bool bindSocket() {
	std::cout << "-----------Binding Socket-----------" << std::endl;
	clientAddr.sin_family = AF_INET; // 设置为IPv4
	clientAddr.sin_port = htons(4399); // 设置端口为4399
	inet_pton(AF_INET, "127.0.0.1", &(clientAddr.sin_addr)); // 设置IPv4地址为127.0.0.1

	if (bind(clientSocket, (SOCKADDR*)&clientAddr, sizeof(clientAddr)) == SOCKET_ERROR) {
		std::cout << "Oops!Failed to bind socket!" << std::endl;
		std::cout << GetLastErrorDetails() << std::endl;
		std::cout << "Please try again!" << std::endl;
		WSACleanup(); // 释放资源
		return false;
	}
	std::cout << "Successfully binded socket!" << std::endl;
	return true;
}

// 获取用户输入的数据包丢失率
void getPacketLossRate() {
	std::cout << "-----------The Loss of Packet-----------" << std::endl;
	std::cout << "Please input the loss of packet in transfer(0-100):" << std::endl;
	std::cin >> Packet_loss_range;
}

// 获取用户输入的相对延迟参数
void getLatencyParam() {
	std::cout << "Please input the latency of time in transfer(0-1):" << std::endl;
	std::cin >> Latency_param;
	while (Latency_param <= 0 || Latency_param > 1) {
		std::cout << "out of range, please input again." << std::endl;
		std::cin >> Latency_param;
	}
}

// 获取用户输入的绝对延迟时间（毫秒）
void getLatencyMillSeconds() {
	std::cout << "Please input the latency of time in transfer(0-1000ms):" << std::endl;
	std::cin >> Latency_mill_seconds;
	while (Latency_mill_seconds < 0 || Latency_mill_seconds > 1000) {
		std::cout << "out of range, please input again." << std::endl;
		std::cin >> Latency_mill_seconds;
	}
}

// 获取用户输入的文件路径并检查文件是否可打开
std::string getInputFilePath() {
	std::string input_path;
	std::cout << "-----------Please input a file path:-----------" << std::endl;
	std::cin >> input_path;

	while (true) {
		std::ifstream file(input_path.c_str());
		if (!file.is_open()) {
			std::cout << "Unable to open file, please start over and chose another input path!" << endl;
			std::cin >> input_path;
		}
		else {
			file.close();
			break;
		}
	}

	return input_path;
}

// 询问用户是否继续发送文件或退出程序
bool askUserForNextAction() {
	bool flag = false;
	std::cout << "Would you like to send aother file or exit? " << endl;
	std::cout << "1:Send another file         2:Exit the Client" << endl;
	std::cout << "input:";
	int choice;
	std::cin >> choice;

	if (choice == 1) {
		restart = true;
		flag = true;
	}
	else if (choice == 2) {
		restart = false;
		flag = true;
	}
	else {
		std::cout << "You can only choose Send another file or Exit the Client, please chose again." << endl;
	}

	return flag;
}

int main() {
	std::cout << "-----------Client UDP----------- " << std::endl;

	if (!initializeWinsock()) {
		return 0;
	}

	if (!createSocket()) {
		return 0;
	}

	configureServerAddress();

	if (!bindSocket()) {
		return 0;
	}

	if (shake_hand() == -1) {
		std::cout << "Unable to shake hands, please start over client and server." << endl;
	}
	else {
		while (true) {
			if (restart == false)
				break;

			getPacketLossRate();
			getLatencyParam();
			getLatencyMillSeconds();

			std::string input_path = getInputFilePath();
			send_data(input_path);

			std::cout << "-----------Successfully Finish the file sending!-----------" << endl;

			if (askUserForNextAction()) {
				continue;
			}
			else {
				break;
			}
		}
	}

	wave_hand();

	delete[] file_data_buffer;
	delete[] send_buff;
	delete[] recv_buff;
	closesocket(clientSocket);
	system("pause");
	WSACleanup();

	return 0;
}