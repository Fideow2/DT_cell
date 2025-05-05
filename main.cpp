#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include "games/SinglePlayerGame.h"
#include "games/MultiPlayerGame.h"

// 游戏类型
enum class GameType {
    SINGLE_PLAYER,
    MULTI_PLAYER
};

// 选择器状态
struct SelectorState {
    GameType selectedGame;
    NetGameMode networkMode;
    bool modeSelected;
    std::string serverIP;
    int port;
};

// 全局状态
SelectorState g_state = {
    GameType::SINGLE_PLAYER,
    NetGameMode::STANDALONE,
    false,
    "127.0.0.1",
    8888
};

// 函数声明
void showHelp();
void runSinglePlayerGame();
void runMultiPlayerGame(const SelectorState& state);
GameType showGameSelector();
void mouseCallback(int event, int x, int y, int flags, void* userdata);
void networkModeMouseCallback(int event, int x, int y, int flags, void* userdata);

int main(int argc, char* argv[]) {
    try {
        // 如果有命令行参数，按照原来的方式处理
        if (argc > 1) {
            std::string arg = argv[1];
            if (arg == "--help" || arg == "-h") {
                showHelp();
                return 0;
            }
            else if (arg == "--standalone") {
                runSinglePlayerGame();
                return 0;
            }
            else if (arg == "--server") {
                SelectorState state;
                state.networkMode = NetGameMode::SERVER;
                state.port = 8888;
                
                // 检查是否有端口参数
                if (argc > 2 && isdigit(argv[2][0])) {
                    state.port = std::stoi(argv[2]);
                }
                
                runMultiPlayerGame(state);
                return 0;
            }
            else if (arg == "--client") {
                SelectorState state;
                state.networkMode = NetGameMode::CLIENT;
                state.serverIP = "127.0.0.1";
                state.port = 8888;
                
                // 检查是否有IP参数
                if (argc > 2 && argv[2][0] != '-') {
                    state.serverIP = argv[2];
                    // 检查是否有端口参数
                    if (argc > 3 && isdigit(argv[3][0])) {
                        state.port = std::stoi(argv[3]);
                    }
                }
                
                runMultiPlayerGame(state);
                return 0;
            }
        }
        
        // 没有参数时，显示游戏选择器
        GameType selectedGame = showGameSelector();
        
        if (selectedGame == GameType::SINGLE_PLAYER) {
            runSinglePlayerGame();
        } else {
            // 显示网络模式选择界面
            cv::Mat selector(300, 500, CV_8UC3, cv::Scalar(240, 240, 240));
            cv::putText(selector, "Select Network Game Mode", cv::Point(120, 40), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            
            // 绘制按钮
            cv::rectangle(selector, cv::Rect(100, 80, 300, 50), cv::Scalar(200, 200, 200), -1);
            cv::rectangle(selector, cv::Rect(100, 150, 300, 50), cv::Scalar(200, 200, 200), -1);
            
            cv::putText(selector, "Create Server", cv::Point(180, 115), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            cv::putText(selector, "Connect to Server", cv::Point(170, 185), 
                        cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            
            // 创建窗口并设置鼠标回调
            cv::namedWindow("Network Mode Selection");
            g_state.modeSelected = false;
            cv::setMouseCallback("Network Mode Selection", networkModeMouseCallback, &g_state);
            
            cv::imshow("Network Mode Selection", selector);
            
            // 等待选择
            while (!g_state.modeSelected) {
                int key = cv::waitKey(20);
                if (key == 27) { // ESC键退出
                    cv::destroyWindow("Network Mode Selection");
                    return 0;
                }
            }
            
            cv::destroyWindow("Network Mode Selection");
            
            // 根据选择运行游戏
            runMultiPlayerGame(g_state);
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}

// 鼠标回调函数 - 网络模式选择
void networkModeMouseCallback(int event, int x, int y, int flags, void* userdata) {
    SelectorState* state = static_cast<SelectorState*>(userdata);
    
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (x >= 100 && x <= 400) {
            if (y >= 80 && y <= 130) {
                // 服务器模式按钮
                state->networkMode = NetGameMode::SERVER;
                state->modeSelected = true;
            }
            else if (y >= 150 && y <= 200) {
                // 客户端模式按钮
                state->networkMode = NetGameMode::CLIENT;
                
                // 提示用户输入IP (在实际应用中可以使用更复杂的UI)
                std::cout << "请输入服务器IP地址 (默认127.0.0.1): ";
                std::string ip;
                std::getline(std::cin, ip);
                if (!ip.empty()) {
                    state->serverIP = ip;
                }
                
                // 提示用户输入端口
                std::cout << "请输入服务器端口 (默认8888): ";
                std::string portStr;
                std::getline(std::cin, portStr);
                if (!portStr.empty()) {
                    try {
                        state->port = std::stoi(portStr);
                    } catch (...) {
                        std::cout << "端口无效，使用默认值8888" << std::endl;
                    }
                }
                
                state->modeSelected = true;
            }
        }
    }
}

// 显示帮助信息
void showHelp() {
    std::cout << "使用方法: cell [参数]\n"
              << "参数:\n"
              << "  --standalone    单机模式\n"
              << "  --server [端口] 服务器模式，可选指定端口号，默认8888\n"
              << "  --client [IP] [端口] 客户端模式，可选指定服务器IP和端口，默认127.0.0.1:8888\n"
              << "  --help         显示此帮助\n"
              << std::endl;
}

// 运行单人游戏
void runSinglePlayerGame() {
    std::cout << "启动单人游戏模式..." << std::endl;
    SinglePlayerGame game;
    game.run();
}

// 运行多人游戏
void runMultiPlayerGame(const SelectorState& state) {
    std::cout << "启动多人游戏模式..." << std::endl;
    
    // 初始化并运行多人游戏
    MultiPlayerGame engine;
    
    // 根据选择的模式进行初始化
    if (state.networkMode == NetGameMode::SERVER) {
        std::cout << "服务器模式，监听端口: " << state.port << std::endl;
        if (!engine.initialize(state.networkMode, "", state.port)) {
            std::cerr << "初始化服务器失败！" << std::endl;
            return;
        }
    }
    else if (state.networkMode == NetGameMode::CLIENT) {
        std::cout << "客户端模式，连接到: " << state.serverIP << ":" << state.port << std::endl;
        if (!engine.initialize(state.networkMode, state.serverIP, state.port)) {
            std::cerr << "连接到服务器失败！" << std::endl;
            return;
        }
    }
    
    engine.run();
}

// 鼠标回调函数 - 主游戏选择
void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    SelectorState* state = static_cast<SelectorState*>(userdata);
    
    if (event == cv::EVENT_LBUTTONDOWN) {
        if (x >= 100 && x <= 400) {
            if (y >= 80 && y <= 130) {
                // 单人游戏按钮
                state->selectedGame = GameType::SINGLE_PLAYER;
                state->modeSelected = true;
            }
            else if (y >= 150 && y <= 200) {
                // 多人游戏按钮
                state->selectedGame = GameType::MULTI_PLAYER;
                state->modeSelected = true;
            }
        }
    }
}

// 显示游戏选择界面
GameType showGameSelector() {
    cv::Mat selector(300, 500, CV_8UC3, cv::Scalar(240, 240, 240));
    cv::putText(selector, "Multi-Cell Game Launcher", cv::Point(140, 40), 
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    
    // 绘制按钮
    cv::rectangle(selector, cv::Rect(100, 80, 300, 50), cv::Scalar(200, 200, 200), -1);
    cv::rectangle(selector, cv::Rect(100, 150, 300, 50), cv::Scalar(200, 200, 200), -1);
    
    cv::putText(selector, "Single Player", cv::Point(200, 115), 
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    cv::putText(selector, "Multiplayer", cv::Point(200, 185), 
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
    
    // 创建窗口并设置鼠标回调
    cv::namedWindow("Game Selection");
    g_state.modeSelected = false;
    cv::setMouseCallback("Game Selection", mouseCallback, &g_state);
    
    cv::imshow("Game Selection", selector);
    
    // 等待选择
    while (!g_state.modeSelected) {
        int key = cv::waitKey(20);
        if (key == 27) { // ESC键
            cv::destroyWindow("Game Selection");
            exit(0);
        }
        
        // 也支持键盘选择
        if (key == '1' || key == 49) {
            g_state.selectedGame = GameType::SINGLE_PLAYER;
            g_state.modeSelected = true;
        }
        else if (key == '2' || key == 50) {
            g_state.selectedGame = GameType::MULTI_PLAYER;
            g_state.modeSelected = true;
        }
    }
    
    cv::destroyWindow("Game Selection");
    return g_state.selectedGame;
}
