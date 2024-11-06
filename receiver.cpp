#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <wiringPi.h>

using namespace std;

const int port = 8080;

const int led = 5;     // GPIO5

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[2] = {0};

    if (wiringPiSetup() == -1) {
        cerr << "Failed to setup WiringPi.\n";
        return 0;
    }

    pinMode(led, OUTPUT);

    // 創建 Socket 檔案描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Socket failed\n";
        exit(1);
    }

    // 設置 Socket 選項
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        cerr << "setsockopt failed\n";
        exit(1);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 綁定 Socket 到指定的 Port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        cerr << "Bind failed\n";
        exit(1);
    }

    // 開始監聽
    if (listen(server_fd, 3) < 0) {
        cerr << "Listen failed\n";
        exit(1);
    }

    cout << "Waiting for connection...\n";
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        cerr << "Accept failed\n";
        exit(1);
    }

    cout << "Connection established.\n";

    // 持續接收資料
    while (1) {
        int valread = read(new_socket, buffer, sizeof(buffer));
        if (valread > 0) {
            if (buffer[0] == '1') {
                cout << "Button is pressed.\n";
                digitalWrite(led, 1);   // LED 亮
            } else if (buffer[0] == '0') {
                digitalWrite(led, 0);   // LED 滅
                cout << "Button is released.\n";
            }
        }
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
