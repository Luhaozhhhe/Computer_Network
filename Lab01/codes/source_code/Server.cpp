#include <WinSock2.h>
#include <iostream>
#include <vector>
#include<string>
#include <mutex>
#include <ws2tcpip.h>
#include"My_Head_File.h"
#include<sstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;


WSAData Wsa_Data;
SOCKET Server_Socket;
sockaddr_in Server_Addr;
vector<Client> Clients;

const size_t bufsize = 256;
char sendbuf[bufsize - 1];
char recvbuf[bufsize - 1];

string str;
mutex Clients_Mutex;
int clientCount = 0;

//用于广播我们的信息
void Broad_Cast_Message(const string& message) {
    lock_guard<mutex> lock(Clients_Mutex);
    for (const auto& client : Clients) {
        send(client.getClientSocket(), message.c_str(), message.size(), 0);
    }

}

//用于传输我们的提示符
void Send_Prompt() {
    lock_guard<mutex> lock(Clients_Mutex);
    for (const auto& client : Clients) {
        send(client.getClientSocket(), "Please enter:", 13, 0);

    }
}

//用于记录日志和广播
void Log_And_Broadcast(const string& message) {
    cout << message << endl;
    Broad_Cast_Message(message);
    cout << "Current online clients: " << clientCount << endl;
}

//用于获取当前的时间
string Get_Formatted_Time() {
    time_t t;
    char str_time[26];
    time(&t);
    ctime_s(str_time, sizeof str_time, &t);
    str_time[strlen(str_time) - 1] = '\0';
    return string(str_time);

}


//传输消息的线程函数
DWORD WINAPI Send_Message_Thread(LPVOID lpParameter) {
    SOCKET send_temp = (SOCKET)lpParameter;

    int log = 0; // 初始化 log 变量
    bool Bool_Condition = true;

    while (Bool_Condition = true || (log != SOCKET_ERROR && log != 0)) {

        Bool_Condition = false;
        memset(sendbuf, 0, sizeof(sendbuf));
        cin.getline(sendbuf, bufsize - 1);

        if (Is_Quit(sendbuf)) {
            closesocket(Server_Socket);
            WSACleanup();
            exit(0);
        }
        string str_time = Get_Formatted_Time();
        str = "<Luhaozhhhe's Chatting Room::Server @ " + str_time + " # Message>: " + string(sendbuf);
        cout << str << endl;
        Broad_Cast_Message(str);

        Send_Prompt();

    }

    return 0;
}


//接收消息的线程函数
DWORD WINAPI Recv_Message_Thread(LPVOID lpParameter) {
    SOCKET recv_temp = (SOCKET)lpParameter;

    memset(recvbuf, 0, sizeof(recvbuf));
    int log = recv(recv_temp, recvbuf, bufsize - 1, 0);
    string username;
    Client c;
    if (recvbuf[0] == '\0') {
        c.setClientSocket(recv_temp);
        c.setClientName("\0");
    }
    else {
        c.setClientSocket(recv_temp);
        c.setClientName(string(recvbuf));
    }

    {
        lock_guard<mutex> lock(Clients_Mutex);
        Clients.push_back(c);
        clientCount++; // 增加客户端计数
    }
    username = c.getClientName();

    string str_time = Get_Formatted_Time();
    str = "<Luhaozhhhe's Chatting Room::Server @ " + str_time + " # Notice>: Welcome <" + username + "> join the ChatGroup!";
    Log_And_Broadcast(str);

    bool recv_flag = true;

    while (recv_flag = true || (log != SOCKET_ERROR && log != 0)) {
        recv_flag = false;
        memset(recvbuf, 0, sizeof(recvbuf));
        Send_Prompt();
        log = recv(recv_temp, recvbuf, bufsize - 1, 0);

        if (log == 0) {
            {
                lock_guard<mutex> lock(Clients_Mutex);
                clientCount--; // 减少客户端计数
                Clients.erase(remove_if(Clients.begin(), Clients.end(), [recv_temp](const Client& client) { return client.getClientSocket() == recv_temp; }), Clients.end());
            }
            string str_time = Get_Formatted_Time();
            str = "<Luhaozhhhe's Chatting Room::Server @ " + str_time + " # Notice>: <" + username + "> has left the ChatGroup!";
            Log_And_Broadcast(str);
            break;
        }

        string str_time = Get_Formatted_Time();
        string msg = string(recvbuf);

        str = "<Luhaozhhhe's Chatting Room::" + username + " @ " + str_time + " # Message>: " + msg;
        Log_And_Broadcast(str);
    }

    closesocket(recv_temp); // 关闭客户端套接字
    return 0;
}

int main() {
    cout << "Welcome To Luhaozhhhe's Chatting Room(Server)!" << endl;

    cout << "-----------Initializing the Environment...-----------" << endl;
    if (WSAStartup(MAKEWORD(2, 0), &Wsa_Data) != 0) {
        cerr << "Something Wrong! Failed to Initialize the Environment!" << endl;
        cerr << Get_Last_Error_Details() << endl;
        cerr << "You Should Try Again!" << endl;
        return 0;

    }

    cout << "Congratulations! Successfully Initialized the Environment!" << endl;

    cout << "-----------Creating Socket...-----------" << endl;
    Server_Socket = socket(AF_INET, SOCK_STREAM, 0);

    if (Server_Socket == INVALID_SOCKET) {
        cerr << "Something Wrong! Failed to Create the Socket!" << endl;
        cerr << Get_Last_Error_Details() << endl;
        cerr << "You Should Try Again!" << endl;
        WSACleanup();
        return 0;

    }
    cout << "Congratulations! Successfully Created the Socket!" << endl;

    cout << "-----------Setting Client Address...-----------" << endl;
    Server_Addr.sin_family = AF_INET;
    Server_Addr.sin_port = htons(8000);
    inet_pton(AF_INET, "127.0.0.1", &(Server_Addr.sin_addr));

    cout << "Congratulations! Successfully Set Client Address!" << endl;
    cout << "-----------Connecting Server-----------" << endl;

    if (bind(Server_Socket, (SOCKADDR*)&Server_Addr, sizeof(Server_Addr)) == SOCKET_ERROR) {
        cerr << "Oops! Failed to Bind the Socket!" << endl;
        cerr << Get_Last_Error_Details() << endl;
        cerr << "You Should Try Again!" << endl;
        WSACleanup();
        return 0;
    }
    cout << "Congratulations! Successfully Bind the Socket!" << endl;

    cout << "-----------Start Listening...-----------" << endl;
    if (listen(Server_Socket,bufsize - 1) != 0) {
        cerr << "Fail to Listen For the Connections!" << endl;
        cerr << Get_Last_Error_Details() << endl;
        cerr << "You Should Try Again!" << endl;
        WSACleanup();
        return 0;
    }
    cout << "Congratulations! Successfully Start Listening!" << endl;

    cout << "-----------Listening...-----------" << endl;

    CloseHandle(CreateThread(NULL, 0, Send_Message_Thread, (LPVOID)Server_Socket, 0, NULL));

    while (1) {
        sockaddr_in addrClient;
        int addr_client_len = sizeof(addrClient);
        SOCKET Socket_Information = accept(Server_Socket, (sockaddr*)&addrClient, &addr_client_len);

        if (Socket_Information == INVALID_SOCKET) {
            cerr << "Oops! Failed to connect a client" << endl;
            cerr << Get_Last_Error_Details() << endl;
        }

        CloseHandle(CreateThread(NULL, 0, Recv_Message_Thread, (LPVOID)Socket_Information, 0, NULL));
    }
    closesocket(Server_Socket);
    WSACleanup();
    return 0;

}


