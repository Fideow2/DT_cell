#include <opencv2/opencv.hpp>
#include <iostream>
#include "GameEngine.h"
#include "NetGameEngine.h"
#include <fstream>
#include <string>

void printHelp() {
    std::cout << "使用方法: cell [参数]\n"
              << "参数:\n"
              << "  --standalone    单机模式 (默认)\n"
              << "  --server [端口] 服务器模式，可选指定端口号，默认8888\n"
              << "  --client [IP] [端口] 客户端模式，可选指定服务器IP和端口，默认127.0.0.1:8888\n"
              << "  --help         显示此帮助\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        // 默认是单机模式
        NetGameMode gameMode = NetGameMode::STANDALONE;
        std::string serverIP = "127.0.0.1";
        int port = 8888;
        
        // 解析命令行参数
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                printHelp();
                return 0;
            }
            else if (arg == "--standalone") {
                gameMode = NetGameMode::STANDALONE;
            }
            else if (arg == "--server") {
                gameMode = NetGameMode::SERVER;
                // 检查是否有端口参数
                if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                    port = std::stoi(argv[i + 1]);
                    ++i;
                }
            }
            else if (arg == "--client") {
                gameMode = NetGameMode::CLIENT;
                // 检查是否有IP参数
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    serverIP = argv[i + 1];
                    ++i;
                    // 检查是否有端口参数
                    if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                        port = std::stoi(argv[i + 1]);
                        ++i;
                    }
                }
            }
        }
        
        // 根据游戏模式启动游戏
        if (gameMode == NetGameMode::STANDALONE) {
            std::cout << "启动单机模式游戏..." << std::endl;
            GameEngine engine;
            engine.run();
        }
        else {
            std::cout << "启动网络模式游戏..." << std::endl;
            NetGameEngine engine;
            
            if (gameMode == NetGameMode::SERVER) {
                std::cout << "服务器模式，监听端口: " << port << std::endl;
                if (!engine.initialize(gameMode, "", port)) {
                    std::cerr << "初始化服务器失败！" << std::endl;
                    return 1;
                }
            }
            else if (gameMode == NetGameMode::CLIENT) {
                std::cout << "客户端模式，连接到: " << serverIP << ":" << port << std::endl;
                if (!engine.initialize(gameMode, serverIP, port)) {
                    std::cerr << "连接到服务器失败！" << std::endl;
                    return 1;
                }
            }
            
            engine.run();
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
