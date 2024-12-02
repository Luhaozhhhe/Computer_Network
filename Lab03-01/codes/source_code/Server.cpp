#include "Server.h"

/*
服务器端总体的流程：

1.三次握手阶段
首先，客户端给服务器发送SYN包，然后服务器收到之后就给客户端发送SYN+ACK包，然后客户端再发送ACK包。
所以，服务器端需要编写的函数有：发送SYN+ACK包，接收SYN包，接收ACK包
会有一些异常处理，都写在函数中了

2.数据传输阶段+二次挥手阶段
首先，正常的接收，异常的接收，都分别写了对应的函数
二次挥手的FIN包也写在了这里


*/


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
            cout << "Failed to receive FIN from client." << endl;
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

            int randomNumber = rand() % 2;  // 模拟数据包丢失（0或1）
            if (randomNumber == 0) {
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

// 发送ACK包的函数，根据发送结果返回是否成功发送的状态
bool send_ACK(Header ack_header, sockaddr_in clientAddr, int serverSocket) {
    char* send_buff = new char[sizeof(Header)];
    memset(send_buff, 0, sizeof(Header));
    memcpy(send_buff, (char*)&ack_header, sizeof(ack_header));

    u_short cks = checksum(send_buff, sizeof(ack_header));
    ((Header*)send_buff)->checksum = cks;

    int log = sendto(serverSocket, send_buff, sizeof(ack_header), 0, (sockaddr*)&clientAddr, sizeof(sockaddr_in));
    if (log == SOCKET_ERROR) {
        std::cout << "Oops!Failed to send ACK. Error details is: " << GetLastErrorDetails() << std::endl;
        std::cout << "Please try again later!" << std::endl;
        delete[] send_buff;
        return false;
    }

    delete[] send_buff;
    return true;
}

// 处理校验和错误的数据包的函数
void handle_Corrupt_Packet(Header& recv_header, sockaddr_in& clientAddr, int serverSocket) {
    std::cout << "----------New Data Begin----------" << std::endl;
    std::cout << "successfully received data---" << recv_header.get_data_length() + recv_header.get_header_length() << "bytes." << std::endl;
    std::cout << "【seq】" << recv_header.get_seq() << ",【ack】 " << recv_header.get_ack() << ",【flag】 " << recv_header.get_flag() << ",【checksum】 " << recv_header.get_checksum() << std::endl;
    std::cout << "【header length】 " << recv_header.get_header_length() << ",【data length】 " << recv_header.get_data_length() << std::endl;
    std::cout << "----------New Data End----------" << std::endl;
    Header ack_header(0, (sequence_num ^ 1), ACK, 0, 0, sizeof(Header));
    int times = 5;
    for (int i = 0; i < times; ++i) {
        if (send_ACK(ack_header, clientAddr, serverSocket)) {
            break;
        }

        if (i == times - 1) {
            std::cout << "Failed to send ACK on corruptied pkg from client too many times." << std::endl;
            std::cout << "------------Connection Lost!-----------" << std::endl;
            return;
        }
    }

    std::cout << "Corruptied Package from client, checksum went wrong!" << std::endl;
    std::cout << "Ack on last package sent." << std::endl;
}

// 处理FIN包的函数
void handle_FIN_Packet(Header& recv_header, sockaddr_in& clientAddr, int serverSocket, bool& waved) {
    std::cout << "-----------Begin to Waving Hands for Two Times!-----------" << std::endl;
    std::cout << "-----the first hands-waving-----" << std::endl;
    std::cout << "Successfully received FIN pkg from client!" << std::endl;
    std::cout << "-----the second hands-waving-----" << std::endl;

    Header ack_header(0, (recv_header.get_seq() + 1) % MAX_SEQ, ACK, 0, 0, sizeof(Header));
    if (!send_ACK(ack_header, clientAddr, serverSocket)) {
        std::cout << "Failed to send ACK to client on Fin pkg!" << std::endl;
        std::cout << "Please try again later!" << std::endl;
        return;
    }

    std::cout << "Successfully sent ACK pkg in respond to FIN pkg from client." << std::endl;

    std::cout << "-----------Finished Waving Hands-----------" << std::endl;
    waved = true;
}

// 主接收函数，负责接收各种类型的数据包并进行相应处理
void Server_Receive_Message(char* data_buff, int* curr_pos, bool& waved) {
    bool finished = false;
    char* send_buff = new char[sizeof(Header)];
    char* recv_buff = new char[sizeof(Header) + MSS];
    memset(recv_buff, 0, MSS + sizeof(Header));
    memset(send_buff, 0, sizeof(Header));
    int addr_Client_length = sizeof(sockaddr_in);
    int log;
    Header recv_header;

    u_long mode = 1;
    ioctlsocket(serverSocket, FIONBIO, &mode);
    clock_t start = clock();
    std::cout << "-----------Waiting for File or Waving hands-----------" << std::endl;

    while (true) {
        int result;

        while ((result = recvfrom(serverSocket, recv_buff, MSS + sizeof(Header), 0, (sockaddr*)&clientAddr, &addr_Client_length)) <= 0) {
            if (clock() - start > LONG_WAIT_TIME) {
                std::cout << "LONG_WAIT_TIME has run out, connection dismissed." << endl;
                std::cout << "------------Connection Lost!-----------" << endl;
                closesocket(serverSocket);
                WSACleanup();
                exit(0);
            }
        }

        memcpy(&recv_header, recv_buff, sizeof(recv_header));

        u_short cks = checksum(recv_buff, result);
        if (cks != 0) {
            handle_Corrupt_Packet(recv_header, clientAddr, serverSocket);
        }
        else if (recv_header.get_flag() == FIN) {
            handle_FIN_Packet(recv_header, clientAddr, serverSocket, waved);
            break;
        }
        else if (recv_header.get_flag() & RST) {
            std::cout << "Recieved RST request from client!" << std::endl;
            std::cout << "------------Connection Lost!-----------" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            exit(0);
        }
        else {
            std::cout << "----------New Data Begin----------" << std::endl;
            std::cout << "successfully received data---" << recv_header.get_data_length() + recv_header.get_header_length() << "bytes in length." << std::endl;
            std::cout << "【seq】 " << recv_header.get_seq() << ", 【ack】 " << recv_header.get_ack() << ", 【flag】 " << recv_header.get_flag() << ", 【checksum】 " << recv_header.get_checksum() << std::endl;
            std::cout << "【header length】 " << recv_header.get_header_length() << ",【data length】 " << recv_header.get_data_length() << std::endl;
            std::cout << "----------New Data End----------" << std::endl;

            int randomNumber = rand() % 100;
            if (randomNumber < Packet_loss_range) {
                std::cout << "------------simulate to drop the data for server!-----------" << std::endl;
            }
            else {
                if (Latency_mill_seconds) {
                    Sleep(Latency_mill_seconds);
                }

                Header ack_header(0, recv_header.get_seq(), ACK, 0, 0, sizeof(Header));
                if (!send_ACK(ack_header, clientAddr, serverSocket)) {
                    std::cout << "Failed to send ACK for pkg from client" << std::endl;
                    std::cout << "------------Connection Lost!-----------" << std::endl;
                    mode = 0;
                    ioctlsocket(serverSocket, FIONBIO, &mode);
                    return;
                }

                std::cout << "Successfully sent ACK pkg:" << std::endl;
                std::cout << "【seq】 " << recv_header.get_seq() << ",【ack】 " << recv_header.get_ack() << ",【flag】 " << recv_header.get_flag() << ",【checksum】 " << recv_header.get_checksum() << std::endl;
                std::cout << "【header length】 " << recv_header.get_header_length() << ",【data length】 " << recv_header.get_data_length() << std::endl;
            }

            if (sequence_num == recv_header.get_seq()) {
                sequence_num ^= 1;
                memcpy(data_buff + *curr_pos, recv_buff + sizeof(Header), recv_header.get_data_length());
                *curr_pos += recv_header.get_data_length();
                std::cout << "Successfully received Data." << std::endl;
            }
            else {
                std::cout << "Received repeated data, DROP it away." << std::endl;
            }
            if (recv_header.get_flag() & LAS) {
                finished = true;
                start = clock();
                std::cout << "Finished Receiving File！" << std::endl;
                break;
            }
        }
    }

    mode = 0;
    ioctlsocket(serverSocket, FIONBIO, &mode);
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

