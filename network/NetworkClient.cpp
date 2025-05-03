#include "NetworkClient.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#endif

// 套接字相关的平台差异处理
#ifdef _WIN32
typedef int socklen_t;
#define CLOSE_SOCKET(s) closesocket(s)
#define SOCKET_ERROR_CODE WSAGetLastError()
#else
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define CLOSE_SOCKET(s) close(s)
#define SOCKET_ERROR_CODE errno
#endif

NetworkClient::NetworkClient(const std::string& ip, int port)
    : serverIP(ip), serverPort(port), clientSocket(INVALID_SOCKET),
      running(false), connected(false) {
}

NetworkClient::~NetworkClient() {
    shutdown();
}

bool NetworkClient::initialize() {
    #ifdef _WIN32
    // 在Windows上初始化Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
    #endif
    
    return initializeSocket();
}

bool NetworkClient::initializeSocket() {
    // 创建客户端套接字
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << SOCKET_ERROR_CODE << std::endl;
        return false;
    }
    
    running = true;
    return true;
}

bool NetworkClient::connectToServer() {
    if (connected) return true;
    
    // 设置服务器地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    
    // 将IP地址转换为网络地址
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << serverIP << std::endl;
        return false;
    }
    
    // 连接到服务器
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << SOCKET_ERROR_CODE << std::endl;
        return false;
    }
    
    // 设置套接字为非阻塞模式
    #ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);
    #else
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
    #endif
    
    connected = true;
    
    // 启动接收线程
    receiveThread = std::thread(&NetworkClient::receiveThreadFunc, this);
    
    std::cout << "已连接到服务器: " << serverIP << ":" << serverPort << std::endl;
    return true;
}

void NetworkClient::disconnect() {
    connected = false;
    
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
    
    if (clientSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
}

void NetworkClient::receiveThreadFunc() {
    const int bufSize = 1024;
    char buffer[bufSize];
    
    while (running && connected) {
        // 清空缓冲区
        memset(buffer, 0, bufSize);
        
        // 接收数据
        int bytesReceived = recv(clientSocket, buffer, bufSize - 1, 0);
        
        if (bytesReceived > 0) {
            // 处理接收到的数据
            try {
                // 简单处理：假设每条消息的第一个字节是消息类型
                MessageType type = static_cast<MessageType>(buffer[0]);
                
                // 剩余字节是数据
                std::vector<uint8_t> data(buffer + 1, buffer + bytesReceived);
                
                NetworkMessage msg(type, data);
                
                // 处理消息
                handleMessage(msg);
            }
            catch (const std::exception& e) {
                std::cerr << "接收消息处理错误: " << e.what() << std::endl;
            }
        }
        else if (bytesReceived == 0) {
            // 服务器关闭连接
            std::cout << "服务器断开连接" << std::endl;
            connected = false;
            break;
        }
        else {
            // 检查是否是合法的错误（非阻塞模式下没有数据可读）
            #ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "接收数据错误: " << error << std::endl;
                connected = false;
                break;
            }
            #else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "接收数据错误: " << errno << std::endl;
                connected = false;
                break;
            }
            #endif
        }
        
        // 暂停一下，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool NetworkClient::sendMessage(const NetworkMessage& msg) {
    if (!connected) return false;
    
    // 锁定，防止多线程同时发送数据
    std::lock_guard<std::mutex> lock(sendMutex);
    
    // 构建要发送的数据，第一个字节是消息类型
    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(msg.type));
    buffer.insert(buffer.end(), msg.data.begin(), msg.data.end());
    
    // 发送数据
    int bytesSent = send(clientSocket, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0);
    
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "发送数据错误: " << SOCKET_ERROR_CODE << std::endl;
        return false;
    }
    
    return true;
}

bool NetworkClient::receiveMessage(NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(receiveMutex);
    
    if (receiveQueue.empty()) {
        return false;
    }
    
    msg = receiveQueue.front();
    receiveQueue.pop();
    return true;
}

void NetworkClient::handleMessage(const NetworkMessage& msg) {
    // 将消息添加到接收队列
    {
        std::lock_guard<std::mutex> lock(receiveMutex);
        receiveQueue.push(msg);
    }
    
    // 如果设置了回调函数，则调用
    if (messageCallback) {
        messageCallback(msg);
    }
}

void NetworkClient::shutdown() {
    running = false;
    
    disconnect();
    
    #ifdef _WIN32
    // 清理Winsock
    WSACleanup();
    #endif
}

bool NetworkClient::isConnected() const {
    return connected;
}

void NetworkClient::setMessageCallback(MessageCallback callback) {
    messageCallback = callback;
}

// 实现获取客户端ID的方法
int NetworkClient::getClientId() const {
    // 假设NetworkClient类中有一个clientId成员变量
    // 如果没有，需要添加该成员变量
    return clientId;
}

// 实现获取最后接收的数据包大小的方法
int NetworkClient::getLastPacketSize() const {
    // 假设NetworkClient类中有一个lastPacketSize成员变量
    // 如果没有，需要添加该成员变量
    return lastPacketSize;
}

// 实现限制连接速度的方法
void NetworkClient::throttleConnection() {
    // 实现限制连接的逻辑
    // 例如，减少发送频率，增加等待时间等
    isThrottled = true;
    throttleDelayMs = 50; // 增加50毫秒延迟
    
    std::cout << "客户端 " << clientId << " 连接已被限制" << std::endl;
}