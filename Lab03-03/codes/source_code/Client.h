#pragma once
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <time.h>
#include <ws2tcpip.h>
#include <WinSock2.h>
#include <deque>
#include<mutex>
using namespace std;

#pragma comment(lib,"ws2_32.lib")

#define SYN 1
#define ACK 2
#define FIN 4
#define LAS 8
#define RST 16

#define MSS 14600
#define UDP_SHAKE_RETRIES 10
#define UDP_WAVE_RETRIES 10
#define MSL 1000
# define MAX_SEQ 256
#define LONG_WAIT_TIME 1000000

#pragma pack(push)
#pragma pack(1)

/*
header设计：
 0              7 8             15 16                          31
+---------------------------------------------------------------+
| 				          Sequence Number                       |
+---------------------------------------------------------------+
|                       Acknowledgment Number                   |
+---------------------------------------------------------------+
|                              Flags   							|
+---------------------------------------------------------------+
|                             Checksum							|
+---------------------------------------------------------------+
|                          	Data  Length                        |
+---------------------------------------------------------------+
|                           Header Length                       |
+---------------------------------------------------------------+
|                                                               |
|							  	                                |
|                               Data                            |
|								                       	        |
|                                                               |
+---------------------------------------------------------------+
*/
class Header {
public:
	u_short seq;
	u_short ack;
	u_short flag;
	u_short checksum;
	u_short data_length;
	u_short header_length;
public:
	Header() {};
	Header(u_short seq, u_short ack, u_short flag, u_short checksum, u_short data_length, u_short header_length) :
		seq(seq), ack(ack), flag(flag), checksum(checksum), data_length(data_length), header_length(header_length) {}
	u_short get_seq() {
		return seq;
	}
	u_short get_ack() {
		return ack;
	}
	u_short get_flag() {
		return flag;
	}
	u_short get_checksum() {
		return checksum;
	}
	u_short get_data_length() {
		return data_length;
	}
	u_short get_header_length() {
		return header_length;
	}

};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)

class Datagram {//设计的数据报的类
public:
	Header header;
	char data[MSS];

	Datagram() = default;//默认初始化
	Datagram(Header header, const char* data) : header(header) {
		std::memcpy(this->data, data, header.data_length);
	}
};

class Sendbuffer {//新建一个发送端的类，方便发送数据
private:
	u_short send_base = 1;//发送窗口的基序号，发送滑动窗口的起点
	u_short next_seq_num = 1;//下一个将被分配给新发送数据报的序列号
	std::deque<Datagram*> slide_window;//滑动窗口
	std::mutex buffer_lock;//添加了互斥锁的机制

public:
	u_short get_send_base() { return send_base; }
	void set_send_base(u_short new_base) {
		std::lock_guard<std::mutex> lock(buffer_lock);
		send_base = new_base;
	}

	void window_slide_pop() {//实现滑动窗口的前端滑动（即当收到ACK后，将窗口从前部弹出一个元素）
		std::lock_guard<std::mutex> lock(buffer_lock);
		slide_window.pop_front();
		send_base++;
	}

	void window_slide_add_data(Datagram* datagram) {//将新待发送的数据报加入窗口尾部，并更新next_seq_num的值
		std::lock_guard<std::mutex> lock(buffer_lock);
		slide_window.push_back(datagram);
		next_seq_num++;
	}

	std::deque<Datagram*>& get_slide_window() { return slide_window; }
	u_short get_next_seq_num() { return next_seq_num; }
	void set_next_seq_num(u_short new_seq_num) {
		std::lock_guard<std::mutex> lock(buffer_lock);
		next_seq_num = new_seq_num;
	}
};


class Timer {
private:
	clock_t start_time = 0;    // 记录定时器启动的时间点（使用clock()返回的CPU时钟计数）
	bool started = false;      // 标志定时器是否已启动
	int timeout = 1.2 * 2 * MSL;// 初始超时时间（默认值），单位视MSL定义而定
	std::mutex timer_lock;     // 互斥锁，用于在多线程环境下保护定时器操作的线程安全

public:
	// 默认构造函数
	Timer() = default;

	// 可传入自定义超时时间的构造函数
	Timer(int timeout) : timeout(timeout) {}

	// 获取当前超时时间（读操作简单，可不加锁，但严格起见也可加锁）
	int get_timeout() { return timeout; }

	// 设置新的超时时间（需加锁以保证多线程安全）
	void set_timeout(int new_timeout) {
		std::lock_guard<std::mutex> lock(timer_lock);
		timeout = new_timeout;
	}

	// 启动定时器，同时设定超时时间为udp_2msl
	void start(int udp_2msl) {
		std::lock_guard<std::mutex> lock(timer_lock);
		start_time = clock();   // 记录当前时刻的CPU时间计数
		started = true;         // 标记定时器已启动
		timeout = udp_2msl;     // 设置新的超时时间
	}

	// 启动定时器（保留原有的timeout值）
	void start() {
		std::lock_guard<std::mutex> lock(timer_lock);
		start_time = clock();   // 记录当前时刻
		started = true;         // 标记定时器已启动
	}

	// 判断是否超时
	// 定时器启动后，如果当前clock()与start_time的差值超过timeout，即表示已超时
	bool is_timeout() {
		std::lock_guard<std::mutex> lock(timer_lock);
		if (started && (clock() - start_time > timeout) == true) {
			return true;
		}
		else {
			return false;
		}
	}

	// 停止定时器（将started置为false）
	void stop() {
		std::lock_guard<std::mutex> lock(timer_lock);
		started = false;
	}
};






string GetLastErrorDetails() {
	int error_code = WSAGetLastError();

	char errorMsg[256] = { 0 };
	FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorMsg,
		sizeof(errorMsg) - 1,
		NULL
	);

	return string("Error code: ") + to_string(error_code) + ", Details: " + errorMsg;
}


WSAData wsadata;
SOCKET clientSocket;

sockaddr_in clientAddr;
SOCKADDR_IN serverAddr;


char* recv_buff;
char* send_buff;
int max_retries_times = 10;
int udp_2msl = 2 * MSL;
//int sequence_num = 0;


char* file_data_buffer = new char[INT_MAX];
int file_length = 0;
bool restart = true;

Timer client_timer;
Sendbuffer send_buffer;

int send_buffer_size;



bool send_over = false;//标识发送过程是否已完成



int Packet_loss_range;
double Latency_param;
int Latency_mill_seconds;

u_short checksum(char* data, int length) {

	int size = length % 2 ? length + 1 : length;
	int count = size / 2;
	char* buf = new char[size];
	memset(buf, 0, size);
	memcpy(buf, data, length);
	u_long sum = 0;
	u_short* buf_iterator = (u_short*)buf;
	while (count--) {
		sum += *buf_iterator++;
		if (sum & 0xffff0000) {
			sum &= 0xffff;
			sum++;
		}
	}
	delete[]buf;
	return ~(sum & 0xffff);
}

deque<string> log_queue;
mutex log_queue_mutex;
mutex log_lock;

DWORD WINAPI thread_log(LPVOID lpParameter) {
	// 线程持续运行，直到send_over为true时才退出
	while (true) {
		// 如果发送已经结束（send_over == true），则返回0结束该线程
		if (send_over == true)
			return 0;

		// 使用unique_lock上锁，以确保对log_queue的访问是线程安全的
		unique_lock<mutex> log_queue_lock(log_queue_mutex);

		// 如果日志队列不为空，则打印最前面的日志，然后弹出它
		if (!log_queue.empty()) {
			cout << log_queue.front();
			log_queue.pop_front();
		}

		// 手动解锁，以允许其他线程访问log_queue
		log_queue_lock.unlock();
	}
}

//新增全局变量
int congestion_window = 1;          // 拥塞窗口，初始为1个MSS
int ssthresh = 16;                   // 慢启动阈值，初始值可以设为16个MSS
int duplicate_ack_count = 0;        // 重复ACK计数
//const int MAX_SEQ_NUM = 65535;       // 最大序列号（根据需要调整）
bool fast_retransmit_triggered = false; // 是否触发快速重传
bool in_slow_start = true;           // 是否处于慢启动阶段

