#include "NetworkServer.h"
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

NetworkServer::NetworkServer(int port)
    : serverPort(port), serverSocket(INVALID_SOCKET), clientSocket(INVALID_SOCKET),
      running(false), connected(false) {
}

NetworkServer::~NetworkServer() {
    shutdown();
}

bool NetworkServer::initialize() {
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

bool NetworkServer::initializeSocket() {
    // 创建服务器套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << SOCKET_ERROR_CODE << std::endl;
        return false;
    }
    
    // 设置服务器地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);
    
    // 绑定套接字
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << SOCKET_ERROR_CODE << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    // 监听连接请求
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << SOCKET_ERROR_CODE << std::endl;
        CLOSE_SOCKET(serverSocket);
        return false;
    }
    
    // 设置套接字为非阻塞模式
    #ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(serverSocket, FIONBIO, &mode);
    #else
    int flags = fcntl(serverSocket, F_GETFL, 0);
    fcntl(serverSocket, F_SETFL, flags | O_NONBLOCK);
    #endif
    
    running = true;
    return true;
}

void NetworkServer::startListening() {
    if (!running) return;
    
    // 启动监听线程
    listenThread = std::thread(&NetworkServer::listenThreadFunc, this);
}

void NetworkServer::stopListening() {
    running = false;
    
    if (listenThread.joinable()) {
        listenThread.join();
    }
    
    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

void NetworkServer::listenThreadFunc() {
    std::cout << "服务器开始监听连接..." << std::endl;
    
    while (running) {
        if (!connected) {
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            // 非阻塞接受连接
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            
            if (clientSocket != INVALID_SOCKET) {
                connected = true;
                std::cout << "客户端已连接: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
                
                // 设置客户端套接字为非阻塞模式
                #ifdef _WIN32
                u_long mode = 1;
                ioctlsocket(clientSocket, FIONBIO, &mode);
                #else
                int flags = fcntl(clientSocket, F_GETFL, 0);
                fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
                #endif
                
                // 启动接收线程
                receiveThread = std::thread(&NetworkServer::receiveThreadFunc, this);
            }
        }
        
        // 暂停一下，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void NetworkServer::receiveThreadFunc() {
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
            // 客户端关闭连接
            std::cout << "客户端断开连接" << std::endl;
            connected = false;
            CLOSE_SOCKET(clientSocket);
            break;
        }
        else {
            // 检查是否是合法的错误（非阻塞模式下没有数据可读）
            #ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "接收数据错误: " << error << std::endl;
                connected = false;
                CLOSE_SOCKET(clientSocket);
                break;
            }
            #else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "接收数据错误: " << errno << std::endl;
                connected = false;
                CLOSE_SOCKET(clientSocket);
                break;
            }
            #endif
        }
        
        // 暂停一下，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool NetworkServer::sendMessage(const NetworkMessage& msg) {
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

bool NetworkServer::receiveMessage(NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(receiveMutex);
    
    if (receiveQueue.empty()) {
        return false;
    }
    
    msg = receiveQueue.front();
    receiveQueue.pop();
    return true;
}

void NetworkServer::handleMessage(const NetworkMessage& msg) {
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

void NetworkServer::shutdown() {
    running = false;
    connected = false;
    
    // 等待线程结束
    stopListening();
    
    // 关闭套接字
    if (clientSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    
    if (serverSocket != INVALID_SOCKET) {
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    #ifdef _WIN32
    // 清理Winsock
    WSACleanup();
    #endif
}

bool NetworkServer::isConnected() const {
    return connected;
}

void NetworkServer::setMessageCallback(MessageCallback callback) {
    messageCallback = callback;
}

bool NetworkServer::hasClient() const {
    return connected;
}