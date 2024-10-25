#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#include<mutex>
#include<string>
#include"My_Head_File.h"
#pragma comment(lib, "ws2_32.lib")
using namespace std;


char sendbuf[255] = { 0 };
char recvbuf[255] = { 0 };
SOCKET Client_Socket;
WSAData Wsa_Data;
sockaddr_in Client_Addr;
string Client_Name;
bool reconnect = true;
bool quit_flag = false;


DWORD WINAPI Recv_Message_Thread(LPVOID lpParamter) {
	SOCKET recv_temp = (SOCKET)lpParamter;
	int log;

	bool recv_flag = true;
	while (recv_flag == true || (log != SOCKET_ERROR && log != 0)) {
		recv_flag = false;
		memset(recvbuf, 0, sizeof(recvbuf));
		log = recv(recv_temp, recvbuf, 255, 0);
		if (log == SOCKET_ERROR) {
			if (quit_flag) {
				return 0;
			}
			else {
				cout << endl;
				cout << "Server unexpectedly closed. Press ENTER to reconnect..." << endl;
				reconnect = true;
				return 0;
			}
		}
		if (string(recvbuf) == "Please enter:")
			cout << string(recvbuf);
		else
			cout << string(recvbuf) << endl;
	}

	return 0;
}

int main() {
	cout << "Welcome To Luhaozhhhe's Chatting Room!" << endl;

	do {
		cout << "-----------Initializing the Environment...-----------" << endl;

		if (WSAStartup(MAKEWORD(2, 0), &Wsa_Data) != 0) {
			cout << "Something Wrong!Failed to Initialize the Environment!" << endl;
			cout << Get_Last_Error_Details() << endl;
			cout << "You Should Try Again!" << endl;
			return 0;

		}

		else {
			cout << "Congratulations!Successfully Initialized the Environment!" << endl;
		}

		//创建socket套接字
		cout << "-----------Creating Socket...-----------" << endl;
		Client_Socket = socket(AF_INET, SOCK_STREAM, 0);

		if (Client_Socket == INVALID_SOCKET) {
			cout << "Something Wrong!Failed to Create the Socket!" << endl;
			cout << Get_Last_Error_Details() << endl;
			cout << "You Should Try Again!" << endl;
			WSACleanup();
			return 0;

		}

		else {
			cout << "Congratulations!Successfully Initialized the Socket!" << endl;

		}

		cout << "-----------Setting Client Address...-----------" << endl;
		Client_Addr.sin_family = AF_INET;
		Client_Addr.sin_port = htons(8000);
		inet_pton(AF_INET, "127.0.0.1", &(Client_Addr.sin_addr));

		cout << "Congratulations!Successfully Set Client Address!" << endl;

		//连接服务器端
		cout << "-----------Connecting Server-----------" << endl;

		if (connect(Client_Socket, (SOCKADDR*)&Client_Addr, sizeof(Client_Addr)) == SOCKET_ERROR) {
			cout << "Oops!Fail to Connect the Server!" << endl;
			cout << Get_Last_Error_Details << endl;
			cout << "Wait for 5 Seconds and Please Try Again!" << endl;
			WSACleanup();
			Sleep(5000);//等待5秒，重新尝试

			continue;

		}
		else {
			cout << "Congratulations!Successfully Connect the Server!" << endl;
			reconnect = false;//修改连接状态的bool值，防止再一次进入循环
		}

		//创建用户
		memset(sendbuf, 0, sizeof(sendbuf));
		cout << "Please Enter your Name(No more than 255 words):" << endl;

	LLL:bool create_condition = true;

		while (create_condition) {//判定创建用户是否成功
			getline(cin, Client_Name);
			if (Client_Name.length() > 255) {
				cout << "the name is too long!" << endl;
				goto LLL;
			}

			create_condition = false;
		}


		send(Client_Socket, Client_Name.data(), Client_Name.length(), 0);

		HANDLE ThreadHandle = CreateThread(NULL, 0, Recv_Message_Thread, (LPVOID)Client_Socket, 0, NULL);

		CloseHandle(ThreadHandle);

		int log = 0;
		bool log_send_condition = true;
		while (log_send_condition == true || (log != SOCKET_ERROR && log != 0)) {
			log_send_condition = false;
			memset(sendbuf, 0, sizeof(sendbuf));
			cin.getline(sendbuf, 255);
			if (Is_Quit(sendbuf) == true) {
				quit_flag = true;
				shutdown(Client_Socket, 0x02);
				closesocket(Client_Socket);
				WSACleanup();
				return 0;

			}

			log = send(Client_Socket, sendbuf, 255, 0);

		}

	} while (reconnect == true);

	closesocket(Client_Socket);
	WSACleanup();

	return 0;
}
