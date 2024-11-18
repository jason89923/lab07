#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <wiringPi.h>
#include <fcntl.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

const int port = 8080;
const int led = 5;     // GPIO5

void clear(char moss[], int arr_size) {
    for (int i = 0; i < arr_size; i++) {
        moss[i] = '\0';
    }
}

char checkMorse(char moss[]) {
    // 修改此function，轉換摩斯密碼為英文或數字
    
    if (strcmp(moss, "-.-.") == 0) return 'C';
    else if (strcmp(moss, ".") == 0) return 'E';
    else if (strcmp(moss, "..") == 0) return 'I';
    else if (strcmp(moss, "-.--") == 0) return 'Y';
    else return '\0';
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[2] = {0};
    
    if (wiringPiSetup() == -1) {
        printf("\033[1;31mFailed to setup WiringPi.\033[0m\n");
        fflush(stdout);
        return 0;
    }

    pinMode(led, OUTPUT);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        printf("\033[1;31mSocket failed.\033[0m\n");
        fflush(stdout);
        exit(1);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        printf("\033[1;31msetsockopt failed.\033[0m\n");
        fflush(stdout);
        exit(1);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        printf("\033[1;31mBind failed.\033[0m\n");
        fflush(stdout);
        exit(1);
    }

    if (listen(server_fd, 3) < 0) {
        printf("\033[1;31mListen failed.\033[0m\n");
        fflush(stdout);
        exit(1);
    }

    cout << "Waiting for connection...\n";
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        printf("\033[1;31mAccept failed.\033[0m\n");
        fflush(stdout);
        exit(1);
    }

    fcntl(new_socket, F_SETFL, O_NONBLOCK);

    printf("\033[1;32mConnection established.\033[0m\n");

    bool buttonPressed = false;
    time_point<system_clock> pressTime, releaseTime;
    int arr_size = 6, count = 0;
    char moss[arr_size] = {'\0'};
    char ans = '\0';

    while (1) {
        int valread = read(new_socket, buffer, sizeof(buffer));
        if (valread > 0) {
            if (buffer[0] == '1' && !buttonPressed) {  // 按鈕被按下
                buttonPressed = true;
                pressTime = system_clock::now();
                digitalWrite(led, 1);   // LED 亮
            } else if (buffer[0] == '0' && buttonPressed) {  // 按鈕被釋放
                buttonPressed = false;
                releaseTime = system_clock::now();
                digitalWrite(led, 0);   // LED 滅

                auto duration = duration_cast<milliseconds>(releaseTime - pressTime).count();

                if (duration < 200) {    // 小於 200ms 視為短按（dot）
                    moss[count] = '.';   // dot(.)
                    printf(".");
                    count++;
                } else {                 // 大於等於 200ms 視為長按（dash）
                    moss[count] = '-';   // dash(-)
                    printf("-");
                    count++;
                }
                fflush(stdout);
            }
        }

        // 檢查是否超過1秒無新輸入來確定結束
        auto currentTime = system_clock::now();
        auto idleDuration = duration_cast<milliseconds>(currentTime - releaseTime).count();
        
        if (!buttonPressed && idleDuration > 500 && count > 0) {
            ans = checkMorse(moss);
            if (ans != '\0') {
                printf(" -> %c\n", ans);
            } else {
                printf("\nInvalid Morse code sequence.\n");
            }
            fflush(stdout);
            clear(moss, arr_size);
            count = 0;
        }

        if (!buttonPressed && idleDuration > 800 && count > 0) {
            printf(" ", ans);
            fflush(stdout);

        }
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
