//
//  main.cpp
//  smalldb_server
//
//  Created by Young-il Seo on 21/06/2019.
//  Copyright © 2019 Young-il Seo. All rights reserved.
//  Only receive data

#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <ctime>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFF_SIZE 1024

typedef struct{
    int hour;
    int min;
}CustomTime;

typedef struct{
    char s_code[7];         // subject code
    CustomTime s_time;      // 수업 시작 시간
    CustomTime f_time;      // 수업 종료 시간
    int s_num;              // number of students
}ConfData;

typedef struct{
    int label;
    char name[BUFF_SIZE];
    bool checked;
}UserData;

typedef struct{
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
    int label;
}AttData;

std::vector<UserData> user_data;
std::vector<AttData> att_data;
ConfData sys_ConfData;

tm* get_current_time(){
    struct tm *curr_tm;
    time_t curr_time;
    curr_time = time(NULL);
    curr_tm = localtime(&curr_time);
    return curr_tm;
}

int req_mod_switch(char* msg){
    if(0 == strncmp(msg, "REQ REG", 7)) return 0;
    else if(0 == strncmp(msg, "REQ ATT", 7)) return 1;
    else if(0 == strncmp(msg, "REQ CONFIG", 9)) return 2;
    else if(0 == strncmp(msg, "EXIT", 4)) return 3;
    return -1;
}
int input_file(){
    UserData temp;
    std::ifstream inFile("user_db.ppap");
    if(!inFile.is_open()) return -1;
    inFile.seekg(0, inFile.end);
    long long length = inFile.tellg();
    if(length == 0) return 1;
    while(!inFile.eof()){
        inFile >> temp.label >> temp.name;
        temp.checked = false;
        user_data.push_back(temp);
    }
    inFile.close();
    return 1;
}
int input_att_file(){
    AttData temp;

    std::ifstream attInFile("att_data.ppap");
    if(!attInFile.is_open()) return -1;
    // if(is_empty(inFile)) return 1;
    
    attInFile >> sys_ConfData.s_code >>
    sys_ConfData.s_time.hour >> sys_ConfData.s_time.min >>
    sys_ConfData.f_time.hour >> sys_ConfData.f_time.min >>
    sys_ConfData.s_num;
    
    while(!attInFile.eof()){
        //year:mon:mday:hour:min:sec:label
        attInFile >> temp.year >> temp.month >> temp.day
        >> temp.hour >> temp.min >> temp.sec >> temp.label;
        att_data.push_back(temp);
    }
    attInFile.close();
    return 1;
}
void output_file(){
    std::ofstream outFile("user_db.ppap");
    for(int i = 0; i < user_data.size(); i++)
        outFile << user_data[i].label << " " << user_data[i].name << std::endl;
    outFile.close();
}
void output_att_file(){
    std::ofstream attOutFile("att_data.ppap");
    attOutFile << sys_ConfData.s_code << " "
    << sys_ConfData.s_time.hour << " " << sys_ConfData.s_time.min << " "
    << sys_ConfData.f_time.hour << " " << sys_ConfData.f_time.min << " "
    << sys_ConfData.s_num << std::endl;
    
    for(int i = 0; i < att_data.size(); i++){
        attOutFile << att_data[i].year << " " << att_data[i].month << " "
        << att_data[i].day << " " << att_data[i].hour << " " <<
        att_data[i].min << " " << att_data[i].sec << " "
        << att_data[i].label << std::endl;
    }
    attOutFile.close();
}
int find_duplicated(int t){
    int i = 0;
    for(i = 0; i < user_data.size(); i++){
        if(user_data[i].label == t) return -1;
    }
    return i;
}
int main(void){
    int mod_sw = 0;
    int server_socket, client_socket, client_addr_size;
    struct sockaddr_in server_addr, client_addr;
    
    char buff_rcv[BUFF_SIZE+5];
    char buff_snd[BUFF_SIZE+5];
    
    input_file();
    input_att_file();
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    
    if(-1 == server_socket){
        std::cout << "server socket error" << std::endl;
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons(4000);
    server_addr.sin_addr.s_addr= htonl(INADDR_ANY);
    
    // Don't use 'using namespace std'
    if(-1 == bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        std::cout << "bind() error" << std::endl;
        exit(1);
    }
    if(-1 == listen(server_socket, 5)){
        std::cout << "wait mode error" << std::endl;
        exit(1);
    }
    client_addr_size  = sizeof( client_addr);
    client_socket     = accept( server_socket,
                               (struct sockaddr*)&client_addr,
                               (socklen_t*)&client_addr_size);
    
    if (-1 == client_socket){
        std::cout << "client error" << std::endl;
        exit(1);
    }
    
    while(1){
        /* mode select
         1. REGISTRATION MODE
         2. ATTENDANCE CHECK
         3. CONFIGURATION MODE
         4. EXIT
         */
        read(client_socket, buff_rcv, BUFF_SIZE);
        printf("receive: %s\n", buff_rcv);
        mod_sw = req_mod_switch(buff_rcv);

        switch(mod_sw){
            // case 0:{ -> ???? fookin compile error :(
            case 0:{
                strcpy(buff_snd, "REG MODE OK");
                if((-1 == write(client_socket, buff_snd, strlen(buff_snd) + 1))
                   || (-1 == input_file())){
                    std::cout << "[ERROR] REPLY REG MODE MSG FAILED" << std::endl;
                    return -1;
                }
                // read student_num & usr_name
                read(client_socket, buff_rcv, BUFF_SIZE);
                std::cout << "RE : " << buff_rcv << std::endl;
                
                int usr_len = 0;
                UserData temp_user;
                temp_user.label = atoi(strtok(buff_rcv, " "));
                usr_len = atoi(strtok(NULL, " "));
                strncpy(temp_user.name, strtok(NULL, " "), usr_len + 1);
                
                if(-1 != find_duplicated(temp_user.label)){
                    user_data.push_back(temp_user);
                    strcpy(buff_snd, "REG OK");
                    write(client_socket, buff_snd, strlen(buff_snd)+1);
                }
                else{
                    strcpy(buff_snd, "REG DP");
                    write(client_socket, buff_snd, strlen(buff_snd)+1);
                }
                std::cout << "REG COMPLETE" << std::endl;
                break;
            }
            case 1:{
                strcpy(buff_snd, "REQ ATT OK");
                if((-1 == write(client_socket, buff_snd, strlen(buff_snd) + 1)) || (-1 == input_att_file())){
                    std::cout << "[ERROR] REPLY ATT MODE MSG FAILED" << std::endl;
                    return -1;
                };
                // read attendance, about 1 minute
                // read를 계속 반복, 특정 요청이 들어오면 레이블을 반환하도록 함
                do{
                    int index = 0;
                    read(client_socket, buff_rcv, BUFF_SIZE);
                    printf("receive: %s\n", buff_rcv);
                    
                    index = find_duplicated(atoi(buff_rcv));
                    strcpy(buff_snd, user_data[index].name);
                    write(client_socket, buff_snd, strlen(buff_snd)+1);
                    if(user_data[index].checked == false){
                        tm* cur_time = get_current_time();
                        int conv_stime = sys_ConfData.s_time.hour * 60 + sys_ConfData.s_time.min;
                        int conv_ftime = sys_ConfData.f_time.hour * 60 + sys_ConfData.f_time.min;
                        int conv_ctime = cur_time->tm_hour * 60 + cur_time -> tm_min;
                        
                        if((conv_ftime > conv_ctime) && (conv_ctime > conv_stime)){
                            user_data[index].checked = true;
                            AttData temp;
                            temp.year = cur_time->tm_year;
                            temp.month = cur_time->tm_mon;
                            temp.day = cur_time->tm_mday;
                            temp.hour = cur_time->tm_hour;
                            temp.min = cur_time->tm_min;
                            temp.sec = cur_time->tm_sec;
                            temp.label = user_data[index].label;
                            att_data.push_back(temp);
                        }
                    }
                }while(0 != strncmp(buff_rcv, "KILL", 4));
                break;
            }
            case 2:{
                sprintf(buff_snd, "%s %d %d %d %d %d", sys_ConfData.s_code,
                        sys_ConfData.s_time.hour, sys_ConfData.s_time.min,
                        sys_ConfData.f_time.hour, sys_ConfData.f_time.min,
                        sys_ConfData.s_num);
                if(-1 == write(client_socket, buff_snd, strlen(buff_snd)+1)){
                    std::cout << "[ERROR] SEND CONFIG ERROR" << std::endl;
                    return -1;
                }
                break;
            }
            case 3:{
                output_file();
                output_att_file();
                close(client_socket);
                return -1;
                break;
            }
        }
    }
}
    
