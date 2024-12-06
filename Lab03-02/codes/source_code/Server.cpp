#include "server.h"

// 发送合成的SYN & ACK包的函数，尝试多次发送直到成功或达到最大重试次数
bool send_synt_ack(Header& send_header) {

	int retries = max_retries_times;
	while (retries--) {
		// 发送SYN & ACK包到客户端
		int log = sendto(serverSocket, send_buff, sizeof(send_header), 0, (sockaddr*)&clientAddr, sizeof(sockaddr_in));
		if (log != SOCKET_ERROR) {
			// 如果发送成功，返回true
			return true;
		}
		cout << "Failed to send SYN & ACK to client. Please try again!" << endl;
		Sleep(1000);
	}
	cout << "Max retries times reached for sending SYN & ACK." << endl;
	// 如果达到最大重试次数仍未成功发送，返回false
	return false;
}

// 等待客户端发送ACK的函数，设置套接字为非阻塞模式并等待ACK或超时处理
bool wait_for_ack() {
	u_long mode = 1;
	ioctlsocket(serverSocket, FIONBIO, &mode);
	clock_t start = clock();
	Header recv_header;
	while (true) {
		int result;
		int addr_client_length = sizeof(sockaddr_in);
		// 循环接收，直到接收到有效数据或超时
		while ((result = recvfrom(serverSocket, recv_buff, sizeof(Header), 0, (sockaddr*)&clientAddr, &addr_client_length)) <= 0) {
			if (clock() - start > 1.2 * udp_2msl) {
				cout << "Waiting for ACK for too long time, sending SYN & ACK again!" << endl;
				if (!send_synt_ack(*(Header*)send_buff)) {
					// 如果重新发送SYN & ACK失败，返回false
					return false;
				}
				start = clock();  // 重新发送后重置定时器
			}
		}

		// 成功接收到数据包，检查是否为ACK
		memcpy(&recv_header, recv_buff, sizeof(recv_header));
		u_short cks = checksum(recv_buff, result);
		if (cks == 0 && (recv_header.get_flag() & ACK)) {
			cout << "Successfully received ACK from Client!" << endl;
			// 如果是ACK，返回true表示握手成功
			return true;
		}
		else if (cks == 0 && (recv_header.get_flag() & SYN)) {
			// 如果收到的是SYN，重新发送SYN & ACK
			cout << "Received SYN again, sending SYN & ACK again!" << endl;
			if (!send_synt_ack(*(Header*)send_buff)) {
				// 如果重新发送失败，返回false
				return false;
			}
		}
		else if (cks == 0 && recv_header.get_flag() == 0) {
			// 处理任何无效或意外的数据包，返回false
			cout << "Receive a invalid response, closing connection!" << endl;
			return false;
		}
	}
}

// 执行握手操作的函数，包括接收SYN包、发送SYN & ACK包以及等待ACK确认
int shake_hand() {

	max_retries_times = 10;
	udp_2msl = 2 * MSL;
	send_buff = new char[sizeof(Header)];
	recv_buff = new char[sizeof(Header) + MSS];

	int addr_client_length = sizeof(sockaddr_in);
	int log;  // 用于记录日志

	while (true) {
		// 接收客户端发送的SYN包
		log = recvfrom(serverSocket, recv_buff, sizeof(Header), 0, (sockaddr*)&clientAddr, &addr_client_length);
		if (log == SOCKET_ERROR) {
			cout << "Failed to receive FIN from client!" << endl;
			cout << GetLastErrorDetails() << endl;
			cout << "Please try again later!" << endl;
			Sleep(1000);
			continue;
		}

		cout << "-----------Begin to shake hands for three times!-----------" << endl;
		cout << "-----the first hand-shaking-----" << endl;

		// 成功接收到包，检查是否为SYN包
		Header recv_header;
		memcpy(&recv_header, recv_buff, sizeof(recv_header));
		u_short cks = checksum(recv_buff, sizeof(recv_header));  // 检查SYN包是否损坏
		if (cks == 0 && (recv_header.get_flag() & SYN)) {
			cout << "Successfully received connecting request (SYN pkg) from Client!" << endl;

			// 发送SYN & ACK包给客户端
			cout << "-----the second hand-shaking-----" << endl;
			Header send_header(0, 0, ACK + SYN, 0, 0, sizeof(Header));
			memcpy(send_buff, (char*)&send_header, sizeof(send_header));
			u_short cks = checksum(send_buff, sizeof(send_header));
			((Header*)send_buff)->checksum = cks;

			int randomNumber = rand() % 100;  // 模拟数据包丢失（0或1）
			if (randomNumber > Packet_loss_range) {
				cout << "------------simulate to drop the data for server!-----------" << endl;
			}
			else {
				// 发送SYN & ACK包
				if (!send_synt_ack(send_header)) {
					return -1;  // 如果发送失败，返回失败标志 -1
				}
				cout << "Successfully sent SYN & ACK pkg to Client!" << endl;

				// 等待客户端发送ACK
				cout << "-----the third hand-shaking-----" << endl;
				if (wait_for_ack()) {
					cout << "-----------Successfully finish the hand-shaking for three times!-----------" << endl;
					return 1;  // 握手成功，返回1
				}
				else {
					cout << "Oops.Shaking Hands Failed!" << endl;
					return -1;  // 握手失败，返回 -1
				}
			}
		}
		else {
			cout << "Receive a invalid packet, please try again..." << endl;
			continue;  // 如果收到无效包，重试握手操作
		}
	}
}

void Server_Receive_Message(char* data_buff, int* curr_pos, bool& waved) {

	bool finished = false;
	char* send_buff = new char[sizeof(Header)];
	char* recv_buff = new char[sizeof(Header) + MSS];
	memset(recv_buff, 0, MSS + sizeof(Header));
	memset(send_buff, 0, sizeof(Header));

	Header* send_header = reinterpret_cast<Header*>(send_buff);
	send_header->flag = ACK;
	send_header->ack = expected_sequence_num - 1;//ack的位置设置为期望收到的num减一
	send_header->header_length = sizeof(Header);
	send_header->checksum = checksum(send_buff, sizeof(Header));

	int addr_Client_length = sizeof(sockaddr_in);
	int log;
	Header recv_header;

	u_long mode = 1;
	ioctlsocket(serverSocket, FIONBIO, &mode);
	clock_t start = clock();
	std::cout << "-----------Waiting for File or Waving hands...-----------" << std::endl;

	while (true) {
		int result;

		while ((result = recvfrom(serverSocket, recv_buff, MSS + sizeof(Header), 0, (sockaddr*)&clientAddr, &addr_Client_length)) <= 0) {
			if (clock() - start > LONG_WAIT_TIME) {
				std::cout << "LONG_WAIT_TIME has run out, connection dismissed..." << std::endl;
				std::cout << "------------Connection Lost!-----------" << std::endl;
				closesocket(serverSocket);
				WSACleanup();
				exit(0);
			}
		}

		memcpy(&recv_header, recv_buff, sizeof(recv_header));

		std::cout << "-----New Data Begin-----" << std::endl;
		std::cout << "Successfully received data---" << recv_header.get_data_length() + recv_header.get_header_length() << " bytes." << std::endl;
		std::cout << "【seq】" << recv_header.get_seq() << ",【ack】 " << recv_header.get_ack() << ",【flag】 " << recv_header.get_flag() << ",【checksum】 " << recv_header.get_checksum() << std::endl;
		std::cout << "【header length】 " << recv_header.get_header_length() << ",【data length】 " << recv_header.get_data_length() << std::endl;
		std::cout << "-----New Data End-----" << std::endl;

		u_short cks = checksum(recv_buff, result);
		if (cks != 0) {

			int retries = 5;
			bool ack_sent = false;
			while (retries > 0 && !ack_sent) {//发送ACK
				log = sendto(serverSocket, send_buff, sizeof(Header), 0, (sockaddr*)&clientAddr, sizeof(sockaddr_in));
				if (log == SOCKET_ERROR) {
					std::cout << "Oops! Failed to send ACK to client for corrupted packet." << std::endl;
					std::cout << GetLastErrorDetails() << std::endl;
					std::cout << "Please try again later!" << std::endl;
					retries--;
				}
				else {
					ack_sent = true;
				}
			}

			if (!ack_sent) {//如果发送ACK失败，就直接失败
				std::cout << "Failed to send ACK on corrupted packet from client too many times." << std::endl;
				std::cout << "------------Connection Lost!-----------" << std::endl;
				return;
			}

			std::cout << "Corrupted Package from client, checksum went wrong!" << std::endl;
			std::cout << "Ack on last correctly received package sent." << std::endl;
		}
		else if (recv_header.get_flag() == FIN) {//如果收到FIN，就说明进行二次挥手
			std::cout << "-----------Begin to Waving Hands for Two Times!-----------" << std::endl;
			std::cout << "-----the first hands-waving-----" << std::endl;
			std::cout << "Successfully received FIN packet from client." << std::endl;
			std::cout << "-----the second hands-waving-----" << std::endl;

			Header ack_header(0, recv_header.seq + 1, ACK, 0, 0, sizeof(Header));//两次挥手，发送ACK和下一个序列号
			memcpy(send_buff, (char*)&ack_header, sizeof(ack_header));

			u_short new_cksum = checksum(send_buff, sizeof(ack_header));
			send_header->checksum = new_cksum;

			int retries = 5;
			bool fin_ack_sent = false;
			while (retries > 0 && !fin_ack_sent) {
				log = sendto(serverSocket, send_buff, sizeof(ack_header), 0, (sockaddr*)&clientAddr, sizeof(sockaddr_in));
				if (log == SOCKET_ERROR) {
					std::cout << "Oops! Failed to send ACK to client on FIN packet!" << std::endl;
					std::cout << GetLastErrorDetails() << std::endl;
					std::cout << "Please try again later..." << std::endl;
					retries--;
				}
				else {
					fin_ack_sent = true;
				}
			}

			if (fin_ack_sent) {//ack发送成功
				std::cout << "Successfully sent ACK packet in response to FIN packet from client!" << std::endl;
			}
			else {//发送失败
				std::cout << "Failed to send ACK on FIN packet from client too many times." << std::endl;
				std::cout << "------------Connection Lost！-----------" << std::endl;
				return;
			}

			std::cout << "-----------Finished Waving Hands-----------" << std::endl;
			waved = true;
			break;
		}
		else if (recv_header.get_flag() & RST) {
			std::cout << "Received RST request from client." << std::endl;
			std::cout << "------------Connection Lost!-----------" << std::endl;
			closesocket(serverSocket);
			WSACleanup();
			exit(0);
		}
		else {//否则就是正常的数据包，按照GBN协议进行处理
			if (recv_header.get_seq() == expected_sequence_num) {//收到了expected_sequence_num数据包
				std::cout << "GBN expected data has been received!" << std::endl;

				Header ack_header(0, expected_sequence_num, ACK, 0, 0, sizeof(Header));//先发送ack
				memcpy(send_buff, (char*)&ack_header, sizeof(ack_header));

				expected_sequence_num++;//然后将期望的数据包的序号加一

				std::cout << "receiver buffer:{ 【" << expected_sequence_num << "】 }" << std::endl;

				u_short new_cksum = checksum(send_buff, sizeof(ack_header));
				send_header->checksum = new_cksum;

				int randomNumber = rand() % 100;
				if (randomNumber <= Packet_loss_range) {
					std::cout << "------------simulate to drop the data for server!-----------" << std::endl;
				}
				else {

					if (Latency_mill_seconds) {
						Sleep(Latency_mill_seconds);
					}

					int retries = 5;
					bool ack_sent = false;
					while (retries > 0 && !ack_sent) {
						log = sendto(serverSocket, send_buff, sizeof(ack_header), 0, (sockaddr*)&clientAddr, sizeof(sockaddr_in));
						if (log == SOCKET_ERROR) {
							std::cout << "Oops! Failed to send ACK to client on datagram." << std::endl;
							std::cout << GetLastErrorDetails() << std::endl;
							std::cout << "Please try again later..." << std::endl;
							retries--;
						}
						else {
							ack_sent = true;
						}
					}

					if (!ack_sent) {
						std::cout << "Failed to send ACK for packet from client too many times." << std::endl;
						std::cout << "------------Connection Lost!-----------" << std::endl;
						mode = 0;
						ioctlsocket(serverSocket, FIONBIO, &mode);
						return;
					}

					std::cout << "Successfully sent ACK packet:" << std::endl;
					std::cout << "【seq】 " << recv_header.get_seq() << ",【ack】 " << recv_header.get_ack() << ",【flag】 " << recv_header.get_flag() << ",【checksum】 " << recv_header.get_checksum() << std::endl;
					std::cout << "【header length】 " << recv_header.get_header_length() << ",【data length】 " << recv_header.get_data_length() << std::endl;
				}

				memcpy(data_buff + *curr_pos, recv_buff + sizeof(recv_header), recv_header.get_data_length());
				*curr_pos += recv_header.get_data_length();

				if (recv_header.get_flag() & LAS) {//如果收到LAS，就说明传输最后一个包了
					finished = true;
					start = clock();
					std::cout << "Finished receiving file!" << std::endl;
					break;
				}
			}
			else {//如果不是期望的序号，就对其重传ack

				int retries = 5;
				bool ack_sent = false;
				while (retries > 0 && !ack_sent) {
					log = sendto(serverSocket, send_buff, sizeof(Header), 0, (sockaddr*)&clientAddr, sizeof(sockaddr_in));
					if (log == SOCKET_ERROR) {
						std::cout << "Oops! Failed to send ACK to client on unexpected datagram." << std::endl;
						std::cout << GetLastErrorDetails() << std::endl;
						std::cout << "Please try again later..." << std::endl;
						retries--;
					}
					else {
						ack_sent = true;
					}
				}

				if (!ack_sent) {
					std::cout << "Failed to send ACK on unexpected packet from client too many times." << std::endl;
					std::cout << "------------Connection Lost!-----------" << std::endl;
					mode = 0;
					ioctlsocket(serverSocket, FIONBIO, &mode);
					return;
				}

				std::cout << "Received unexpected datagram, DROPPING it." << std::endl;
				std::cout << "receiver buffer:{ 【" << expected_sequence_num << "】 }" << std::endl;
				std::cout << "Ack on last correctly received package sent." << std::endl;
			}
		}
	}

	mode = 0;
	ioctlsocket(serverSocket, FIONBIO, &mode);

	delete[] send_buff;
	delete[] recv_buff;
	return;
}



// 初始化Winsock库的函数，返回初始化是否成功的状态
bool initialize_winsock() {
	cout << "-----------Begin to Initializing the Winsock...-----------" << endl;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		cout << "Oops! Failed to initialize Winsock." << endl;
		return false;
	}
	cout << "Successfully initialized Winsock!" << endl;
	return true;
}

// 创建UDP套接字的函数，返回创建的套接字描述符，如果创建失败则返回INVALID_SOCKET
SOCKET create_socket() {
	cout << "-----------Creating Socket...-----------" << endl;
	SOCKET socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_fd == INVALID_SOCKET) {
		cout << "Oops! Failed to create socket." << endl;
		return INVALID_SOCKET;
	}
	cout << "Successfully created socket!" << endl;
	return socket_fd;
}

// 绑定套接字到指定本地地址和端口的函数，返回绑定是否成功的状态
bool bind_socket(SOCKET serverSocket) {
	cout << "-----------Binding Socket...-----------" << endl;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(2333);
	inet_pton(AF_INET, "127.0.0.1", &(serverAddr.sin_addr));

	if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		cout << "Oops! Failed to bind socket." << endl;
		return false;
	}
	cout << "Successfully binded socket!" << endl;
	return true;
}

// 此函数用于等待与客户端进行握手操作
bool wait_for_shakehand() {
	cout << "-----------Waiting for Shaking hands...-----------" << endl;
	if (shake_hand() != -1) {
		return true;
	}
	return false;
}


// 此函数用于处理文件传输的整个过程
void process_file_transfer(SOCKET serverSocket) {
	while (true) {
		// 动态分配足够大的缓冲区用于存储接收到的文件数据
		char* file_data_buffer = new char[INT_MAX];
		int file_length = 0;
		bool waved = false;

		// 提示用户输入文件传输过程中的数据包丢失率
		cout << "-----------the Loss of Packet-----------" << endl;
		cout << "Please input the loss of packet in transfer(0-100):" << endl;
		cin >> Packet_loss_range;

		// 提示用户输入确认应答（ACK）的延迟时间
		cout << "-----------Latency Test-----------" << endl;
		cout << "Please input the latency of time in transfer(0-3000ms):" << endl;

		while (true) {
			cin >> Latency_mill_seconds;
			if (Latency_mill_seconds < 0 || Latency_mill_seconds > 3000) {
				cout << "Latency mill seconds out of range, please input again!" << endl;
				continue;
			}
			break;
		}

		// 调用Server_Receive_Message函数接收文件数据，将接收到的数据存储到file_data_buffer中
		// 并更新文件长度file_length，同时根据接收情况更新waved标志
		Server_Receive_Message(file_data_buffer, &file_length, waved);
		// 如果waved标志为true，表示文件接收完成（可能是接收到了文件结束标志等情况），则退出循环
		if (waved == true) {
			break;
		}

		// 从接收到的文件数据中提取文件路径信息
		string file_path = "";
		int pos;
		for (pos = 0; pos < file_length; pos++) {
			if (file_data_buffer[pos] == '?')
				break;
			else
				file_path += file_data_buffer[pos];
		}

		// 输出文件接收的结果信息，包括从哪里接收到文件以及文件的总长度（去除文件路径部分后的长度）
		cout << "-----------Result:-----------" << endl;
		cout << "Successfully received file from: " + file_path << ", with length of " << file_length - (pos + 1) << " Bytes!" << endl;

		// 提示用户输入输出文件的路径，用于将接收到的文件数据保存到本地磁盘
		cout << "-----------Enter the output file path:----------" << endl;
		while (true) {
			string output_path;
			cin >> output_path;
			// 尝试以二进制模式打开用户指定的输出文件路径
			ofstream file(output_path.c_str(), ofstream::binary);
			// 如果文件无法打开，提示用户重新输入路径
			if (!file.is_open()) {
				cout << "Unable to open file, please choose another output path!" << endl;
				continue;
			}
			else {
				// 如果文件成功打开，将接收到的文件数据（去除文件路径部分）写入到输出文件中
				file.write(file_data_buffer + pos + 1, file_length - pos - 1);
				file.close();
				cout << "Successfully output the file in this path: " + output_path << "." << endl;
				break;
			}
		}
	}
}

// 此函数用于清理资源，包括关闭套接字和释放Winsock库相关资源
void cleanup(SOCKET serverSocket) {
	closesocket(serverSocket);
	WSACleanup();
}


int main() {

	cout << "-----------Server UDP----------- " << endl;

	if (!initialize_winsock()) {
		cout << "Failed to initialize Winsock!" << endl;
		return 0;
	}

	serverSocket = create_socket();
	if (serverSocket == INVALID_SOCKET) {
		cout << "Failed to create socket!" << endl;
		cleanup(serverSocket);
		return 0;
	}

	if (!bind_socket(serverSocket)) {
		cout << "Failed to bind socket!" << endl;
		cleanup(serverSocket);
		return 0;
	}

	if (wait_for_shakehand()) {
		process_file_transfer(serverSocket);
	}
	else {
		cout << "Unable to shake hands, please restart the client and server." << endl;
	}


	cleanup(serverSocket);
	return 0;
}