#include"client.h"

int ack_num_time = 0;//全局计数器

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

int find_max(int a, int b) {
	if (a > b) {
		return a;
	}
	else {
		return b;
	}
}

int find_min(int a, int b) {//查找最小的窗口
	if (a > b) {
		return a;
	}
	else {
		return b;
	}
}


DWORD WINAPI thread_receive_messege(LPVOID lpParameter) {
	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode); // 将clientSocket设为非阻塞模式
	recv_buff = new char[sizeof(Header)];      // 接收缓冲区，只用于接收Header
	Header recv_header;

	sockaddr_in tempAddr;
	int temp_addr_length = sizeof(sockaddr_in);

	while (true) {
		// 模拟丢包和延迟，如有一定概率随机触发延迟调整
		int random_number = rand() % 100;
		if (random_number < Packet_loss_range) {
			lock_guard<mutex> log_queue_lock(log_queue_mutex);
			log_queue.push_back("------------RELATIVE DELAY TIME!-----------\n");
			// 动态调整超时时间: 延长超时时间，用于模拟网络延迟情况
			client_timer.set_timeout(Latency_param * client_timer.get_timeout());
		}

		// 若发送结束标志为true，则关闭套接字非阻塞模式，释放资源后退出线程
		if (send_over == true) {
			mode = 0;
			ioctlsocket(clientSocket, FIONBIO, &mode);
			delete[] recv_buff;
			return 0;
		}

		// 非阻塞接收数据包的循环
		while (recvfrom(clientSocket, recv_buff, sizeof(recv_header), 0, (sockaddr*)&tempAddr, &temp_addr_length) <= -1) {
			// 如果在此过程中，发送已结束，则退出线程
			if (send_over == true) {
				mode = 0;
				ioctlsocket(clientSocket, FIONBIO, &mode);
				delete[] recv_buff;
				return 0;
			}

			// 判断是否超时 超时重传
			if (client_timer.is_timeout()) { // 超时就重传在窗口内的所有的数据包
				// 设置慢启动阈值
				ssthresh = find_max(congestion_window / 2, 1);
				congestion_window = 1;
				in_slow_start = true;
				duplicate_ack_count = 0;

				// 记录重传相关日志，方便查看网络状态变化
				lock_guard<mutex> log_queue_lock(log_queue_mutex);
				log_queue.push_back("Timeout occurred, entering slow start phase and resent all datagrams in the window.\n");
				log_queue.push_back("New ssthresh: " + to_string(ssthresh) + ", New congestion_window: " + to_string(congestion_window) + "\n");

				// 重传窗口内所有数据包
				for (auto resend : send_buffer.get_slide_window()) {
					int log = sendto(
						clientSocket,
						(char*)resend,
						resend->header.get_data_length() + resend->header.get_header_length(),
						0,
						(sockaddr*)&serverAddr,
						sizeof(sockaddr_in)
					);

					if (log == SOCKET_ERROR) {
						std::cerr << "Failed to resend data packet! Error: " << GetLastErrorDetails() << std::endl;
						// 进行相应的错误处理，如重试或退出
					}
				}

				log_queue.push_back("Timeout, resent datagram to server.\n");
				client_timer.start(); // 重置定时器开始计时
			}
		}

		// 将接收到的数据拷贝到Header结构中
		memcpy(&recv_header, recv_buff, sizeof(recv_header));

		{
			// 打印接收到的数据包header信息
			lock_guard<mutex> log_queue_lock(log_queue_mutex);
			log_queue.push_back("Successfully received data---" +
				to_string(recv_header.get_data_length() + recv_header.get_header_length()) +
				"Bytes.\n");
			log_queue.push_back("Header---\n");
			log_queue.push_back("【seq】" + to_string(recv_header.get_seq()) +
				"【ack】" + to_string(recv_header.get_ack()) +
				"【flag】" + to_string(recv_header.get_flag()) +
				"【checksum】" + to_string(recv_header.get_checksum()) + "\n");
			log_queue.push_back("header length:" + to_string(recv_header.get_header_length()) +
				", data length:" + to_string(recv_header.get_data_length()) + "\n");
			log_queue.push_back("【congestion_window】" + to_string(congestion_window) + "\n");
		}

		// 校验和检查
		u_short cks = checksum(recv_buff, sizeof(recv_header));
		if (cks != 0) {
			// 数据包损坏，忽略（继续循环等待下一个包）
			continue;
		}

		// 如果是ACK包 ——累积确认
		if (recv_header.get_flag() & ACK) {

			ack_num_time = ack_num_time + 1;//全局计数，记录ack的数量

			if (recv_header.get_ack() >= send_buffer.get_send_base()) {
				int ack_num = recv_header.get_ack() + 1 - send_buffer.get_send_base();

				if (ack_num > 0) {
					// 新的ACK，滑动窗口
					for (int i = 0; i < ack_num; i++) {
						send_buffer.window_slide_pop();
					}

					// 更新发送基准
					send_buffer.set_send_base(recv_header.get_ack() + 1);

					// 调整拥塞窗口
					if (in_slow_start) {
						congestion_window = congestion_window + 1; // 指数增长
						if (congestion_window >= ssthresh) {
							in_slow_start = false;
							lock_guard<mutex> log_queue_lock(log_queue_mutex);
							log_queue.push_back("Exiting slow start phase, entering congestion avoidance phase.\n");
						}
					}
					else {
						if (ack_num_time == congestion_window) {
							congestion_window = congestion_window + 1;
							ack_num_time = 0;//重新开始计时
						}

						lock_guard<mutex> log_queue_lock(log_queue_mutex);
						log_queue.push_back("In congestion avoidance phase, updated congestion_window: " + to_string(congestion_window) + "\n");
					}

					// 记录拥塞窗口调整相关日志
					lock_guard<mutex> log_queue_lock(log_queue_mutex);
					log_queue.push_back("Received new ACK, updated congestion_window: " + to_string(congestion_window) + "\n");

					// 重置重复ACK计数
					duplicate_ack_count = 0;
				}
				else {
					// 重复ACK
					duplicate_ack_count++;
					if (duplicate_ack_count == 3) { // 触发快速重传
						ssthresh = find_max(congestion_window / 2, 1);
						congestion_window = ssthresh + 3;
						in_slow_start = false;
						fast_retransmit_triggered = true;

						// 重传丢失的数据包
						auto resend = send_buffer.get_slide_window().front();
						int log = sendto(
							clientSocket,
							(char*)resend,
							resend->header.get_data_length() + resend->header.get_header_length(),
							0,
							(sockaddr*)&serverAddr,
							sizeof(sockaddr_in)
						);

						if (log == SOCKET_ERROR) {
							std::cerr << "Failed to resend data packet during Fast Retransmit! Error: " << GetLastErrorDetails() << std::endl;
							// 进行相应的错误处理，如重试或退出
						}

						// 记录重传操作及相关状态变化日志
						lock_guard<mutex> log_queue_lock(log_queue_mutex);
						log_queue.push_back("Fast Retransmit: Resent datagram due to triple duplicate ACKs.\n");
						log_queue.push_back("Updated ssthresh: " + to_string(ssthresh) + ", Updated congestion_window: " + to_string(congestion_window) + "\n");
					}
				}


				if (fast_retransmit_triggered) {
					// 处于快速恢复阶段，每收到一个新的 ACK（非重复 ACK）
					if (recv_header.get_ack() > send_buffer.get_send_base()) {
						congestion_window = find_min(congestion_window + 1, ssthresh);  // 根据 Reno 算法调整拥塞窗口，这里原代码中假设MSS为1先按每次增加1处理，更准确的是根据实际定义的MSS值来计算增加量，比如 MSS * (MSS / congestion_window)，然后取和ssthresh的最小值
						// 如果拥塞窗口达到慢启动阈值，进入拥塞避免阶段
						if (congestion_window == ssthresh) {
							in_slow_start = false;
							fast_retransmit_triggered = false;
							// 记录进入拥塞避免阶段的日志
							lock_guard<mutex> log_queue_lock(log_queue_mutex);
							log_queue.push_back("Exiting fast recovery phase, entering congestion avoidance phase.\n");
						}
					}
				}

				// 记录发送窗口状态
				{
					lock_guard<mutex> lock(log_queue_mutex);
					log_queue.push_back("send_buffer:{ ");
					for (int i = 0; i < (int)send_buffer.get_slide_window().size(); i++) {
						log_queue.push_back("[" + to_string(send_buffer.get_slide_window()[i]->header.get_seq()) + "] ");
					}

					// 打印发送窗口中尚未使用的位置，以便直观了解窗口情况
					for (int i = send_buffer.get_next_seq_num(); i <= send_buffer.get_send_base() + send_buffer_size - 1; i++)
						log_queue.push_back("[ ] ");
					log_queue.push_back("}\n");
					log_queue.push_back("【congestion_window】" + to_string(congestion_window) + "\n");
				}

				// 调整定时器
				if (send_buffer.get_send_base() == send_buffer.get_next_seq_num()) {
					client_timer.stop();
				}
				else {
					client_timer.start();
				}
			}
		}
		// 处理RST包
		else if (recv_header.get_flag() & RST) {
			lock_guard<mutex> log_queue_lock(log_queue_mutex);
			log_queue.push_back("Server unexpectedly closed: Error in connection.\n");

			// 清理资源
			delete[] file_data_buffer;
			delete[] send_buff;
			delete[] recv_buff;
			closesocket(clientSocket);
			WSACleanup();
			exit(0);
		}

		// 其他处理逻辑
	}

	// 关闭套接字和清理资源
	mode = 0;
	ioctlsocket(clientSocket, FIONBIO, &mode);
	delete[] recv_buff;
	return 0;
}





void Client_Send_Message(char* data_buff, int pkg_length, bool last_pkg) {
	// 等待直到发送窗口有空位（基于拥塞窗口和发送缓冲区大小）
	while (send_buffer.get_next_seq_num() >= send_buffer.get_send_base() + find_min(congestion_window, send_buffer_size)) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	// 构建发送包头部
	int flag = last_pkg ? LAS : 0;
	Header send_header(send_buffer.get_next_seq_num(), 0, flag, 0, pkg_length, sizeof(Header));
	Datagram* datagram = new Datagram(send_header, data_buff);

	// 计算校验和
	u_short cks = checksum((char*)datagram, pkg_length + sizeof(send_header));
	datagram->header.checksum = cks;

	// 启动定时器，如果发送缓冲区处于空闲状态
	if (send_buffer.get_send_base() == send_buffer.get_next_seq_num())
		client_timer.start();

	// 将数据包添加到发送缓冲区
	send_buffer.window_slide_add_data(datagram);

	// 模拟丢包
	int randomNumber = rand() % 100;
	if (randomNumber < Packet_loss_range) {
		// 模拟丢包，不实际发送，只记录日志
		lock_guard<mutex> log_queue_lock(log_queue_mutex);
		log_queue.push_back("------------simulate to drop the data for server!-----------\n");
	}
	else {
		// 实际发送数据包
		int log = sendto(
			clientSocket,
			(char*)datagram,
			pkg_length + sizeof(send_header),
			0,
			(sockaddr*)&serverAddr,
			sizeof(sockaddr_in)
		);

		if (log == SOCKET_ERROR) {
			std::cerr << "Failed to send data packet! Error: " << GetLastErrorDetails() << std::endl;
			// 进行相应的错误处理，如重试或退出
			delete datagram;
			return;
		}

		// 记录发送日志
		{
			lock_guard<mutex> lock(log_queue_mutex);
			log_queue.push_back("-----New Data Begin-----\n");
			log_queue.push_back("Successfully sent data---"
				+ to_string(send_header.get_data_length() + send_header.get_header_length())
				+ "Bytes.\n");
			log_queue.push_back("Header---\n");
			log_queue.push_back("【seq】" + to_string(send_header.get_seq())
				+ "【ack】" + to_string(send_header.get_ack())
				+ "【flag】" + to_string(send_header.get_flag())
				+ "【checksum】" + to_string(send_header.get_checksum()) + "\n");
			log_queue.push_back("header length:" + to_string(send_header.get_header_length())
				+ ", data length:" + to_string(send_header.get_data_length()) + "\n");
			log_queue.push_back("【congestion_window】" + to_string(congestion_window) + "\n");
			log_queue.push_back("-----New Data End-----\n");

			// 输出当前发送缓冲区（滑动窗口）的状态
			log_queue.push_back("send_buffer:{ ");
			for (int i = 0; i < (int)send_buffer.get_slide_window().size(); i++) {
				log_queue.push_back("[" + to_string(send_buffer.get_slide_window()[i]->header.get_seq()) + "] ");
			}

			// 打印发送窗口中尚未使用的位置，以便直观了解窗口情况
			for (int i = send_buffer.get_next_seq_num(); i <= send_buffer.get_send_base() + send_buffer_size - 1; i++)
				log_queue.push_back("[ ] ");
			log_queue.push_back("}\n");
			log_queue.push_back("【congestion_window】" + to_string(congestion_window) + "\n");
		}
	}
}




void send_data(string file_path) {
	// 函数作用：读取指定文件，将文件名和文件内容打包后通过Client_Send_Message函数分片发送给服务器，
	//           使用一个接收线程和一个日志线程处理服务器的反馈与日志输出。最后统计发送时间和吞吐量。

	ifstream file(file_path.c_str(), ifstream::binary);

	// 定位文件指针到文件末尾以获取文件长度
	file.seekg(0, file.end);
	file_length = file.tellg(); // 获取文件长度（字节数）
	file.seekg(0, file.beg);    // 文件指针回到开头

	// total_length = 文件名长度 + 文件内容长度 + 1个特殊标记字节('?')
	int total_length = file_path.length() + file_length + 1;

	memset(file_data_buffer, 0, sizeof(char) * total_length);

	memcpy(file_data_buffer, file_path.c_str(), file_path.length());

	file_data_buffer[file_path.length()] = '?';

	file.read(file_data_buffer + file_path.length() + 1, file_length);
	file.close();

	cout << "-----------Start Sending File...-----------" << endl;
	clock_t start = clock(); // 记录开始发送的时间

	// 创建接收线程和日志线程
	HANDLE recv_handle = CreateThread(NULL, 0, thread_receive_messege, NULL, 0, NULL);
	HANDLE log_handle = CreateThread(NULL, 0, thread_log, NULL, 0, NULL);

	int curr_pos = 0;
	int log;

	while (curr_pos < total_length) {
		int pkg_length = (total_length - curr_pos >= MSS) ? MSS : (total_length - curr_pos);
		bool last = (total_length - curr_pos <= MSS) ? true : false;

		Client_Send_Message(file_data_buffer + curr_pos, pkg_length, last);
		curr_pos += MSS;
	}

	while (send_buffer.get_slide_window().size() != 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待所有数据包被确认
	}

	// 标记发送结束，此时接收线程和日志线程会感知结束条件
	send_over = true;

	// 等待接收线程结束
	WaitForSingleObject(recv_handle, INFINITE);
	WaitForSingleObject(log_handle, INFINITE);

	clock_t end = clock(); // 记录结束时间

	cout << "-----------Finished Sending File!-----------" << endl;
	cout << "Successfully sent file: " + file_path + " to server!" << endl;
	cout << "-----------Result:----------" << endl;
	cout << "Total length:" << total_length << " Bytes." << endl;
	cout << "Total time:" << (end - start) * 1000 / (double)CLOCKS_PER_SEC << " ms." << endl;

	// 根据总时间计算吞吐量
	if (!(end - start))
		cout << "Flash! Time is too short to compute a throughput." << endl;
	else
		cout << "Throughput:" << total_length / ((end - start) * 1000 / (double)CLOCKS_PER_SEC) << " Bytes/ms." << endl;

	return;
}



void buildAndPrepareFINPacket(Header& fin_header, char*& send_buff) {
	// 获取下一个序列号
	u_short fin_seq = send_buffer.get_next_seq_num();

	// 直接赋值给引用参数 fin_header，而不是重新定义一个局部变量
	fin_header = Header(fin_seq, 0, FIN, 0, 0, sizeof(Header));

	// 分配发送缓冲区
	send_buff = new char[sizeof(Header)];
	memset(send_buff, 0, sizeof(Header));
	memcpy(send_buff, (char*)&fin_header, sizeof(fin_header));

	// 计算校验和
	u_short cks = checksum(send_buff, sizeof(fin_header));
	((Header*)send_buff)->checksum = cks;
}


// 处理发送FIN包前的模拟丢包逻辑
void handleSendFINPacketDrop(int& randomNumber) {
	randomNumber = rand() % 100;
	if (randomNumber < Packet_loss_range) {
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


void Get_Windows_Size() {//获取窗口大小

	int new_buffer_size;
	while (true) {
		cout << "-----------Buffer Size-----------" << endl;
		cout << "Please input the size of send buffer:" << endl;
		cout << "Size range[20-32]:";
		cin >> new_buffer_size;
		if (new_buffer_size < 20 || new_buffer_size > 32) {
			cout << "Size out of range, please input again." << endl;
			continue;
		}
		else {
			send_buffer_size = new_buffer_size;
			break;
		}
	}

	send_over = false;
	client_timer.stop();
	send_buffer.set_send_base(1);
	send_buffer.set_next_seq_num(1);
	send_buffer.get_slide_window().clear();//滑动窗口初始化
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
			Get_Windows_Size();//added!

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
