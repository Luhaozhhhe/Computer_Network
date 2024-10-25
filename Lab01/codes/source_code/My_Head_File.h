#include<iostream>
#include<WinSock2.h>
#include <ws2tcpip.h>
#include<mutex>
#include<string>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

string Get_Last_Error_Details() {
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

	return string("Error code is: ") + to_string(error_code) + ", and the Details: " + errorMsg;
}


string Get_Random_Name() {//如果用户输入换行，则自动分配随机用户名
	srand(time(0));
	int name_length = rand() % 5 + 1;
	const string charpool = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string temp_name;
	for (int i = 0; i < name_length; i++) {
		int Index_Of_Name = rand() % charpool.length();
		temp_name = temp_name + charpool[Index_Of_Name];
	}

	return temp_name;

}

//判断用户是否自愿退出
bool Is_Quit(char* text) {
	int len = 0;
	while (text[len] != '\0') {
		len++;
	}
	if (len == 4 && text[0] == 'Q' && text[1] == 'U' && text[2] == 'I' && text[3] == 'T') {
		return true;
	}
	else {
		return false;
	}
}

//系统中断，强制退出
bool Is_System_Quit(char* text) {
	int len = 0;
	while (text[len] != '\0') {
		len++;
	}
	if (len == 29 && text[25] == 'Q' && text[26] == 'U' && text[27] == 'I' && text[28] == 'T') {
		return true;
	}
	else {
		return false;
	}
}

class Client {
private:
	SOCKET Client_Socket;
	string Client_Name;

public:
	Client() {

	}
	SOCKET getClientSocket() const {
		return Client_Socket;
	}
	void setClientSocket(SOCKET socket_name) {
		Client_Socket = socket_name;
	}
	string getClientName() {
		return Client_Name;
	}
	void setClientName(string temp) {
		if (temp == "\0") //若输入进来的是串尾符，则没有正常输入名字
			Client_Name = Get_Random_Name();
		else
			Client_Name = temp;
	}

};