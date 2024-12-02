#pragma once
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <time.h>
#include <ws2tcpip.h>
#include <WinSock2.h>
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
SOCKET serverSocket;

sockaddr_in clientAddr;
sockaddr_in serverAddr;

char* recv_buff;
char* send_buff;
int max_retries_times = 10;
int udp_2msl = 2 * MSL;
int sequence_num = 0;

char* file_data_buffer = new char[INT_MAX];
int file_length = 0;

int Packet_loss_range;

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
