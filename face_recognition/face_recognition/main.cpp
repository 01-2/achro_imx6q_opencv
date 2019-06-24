#include <iostream>
#include <utility>
#include <vector>
#include <ctime>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/face.hpp>

#define _CRT_SECURE_NO_WARNINGS
#define CV_HAAR_SCALE_IMAGE 2
#define CV_INTER_LINEAR 1
#define MAXLINE 1024

using namespace std;
using namespace cv;

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
    char name[MAXLINE];
}UserData;

auto model = face::LBPHFaceRecognizer::create();
VideoCapture cap(0);
Mat inp;

string path = "/Users/yiseo/my_code/face_recognition/";
ofstream dbout(path + "db.txt", ios::app);
ConfData sys_ConfData;

static void dbread(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';'){
    ifstream file(filename);
    if (!file){
        string error = "no valid input file";
        cout << error << endl;
        exit(0);
    }
    
    string line, path, label;
    while (!file.eof()){
        getline(file, line);
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, label);
        if (!path.empty() && !label.empty()){
            images.push_back(imread(path, 0));
            labels.push_back(atoi(label.c_str()));
        }
    }
}
vector<Mat> faceDetect(Mat inp){
    Mat gray;
    vector <Mat> ret;
    vector <Rect> faces, crop;
    cvtColor(inp, gray, COLOR_BGR2GRAY);
    
    CascadeClassifier cas;
    string path = "/usr/local/Cellar/opencv/4.1.0_2/share/opencv4/haarcascades/haarcascade_frontalface_default.xml";
    
    if(! cas.load(path) ){
        cerr << "haarcascade load failed" << endl;
        exit(0);
    }
    
    cas.detectMultiScale(gray, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30));
    
    for(int i = 0; i < faces.size(); i++){
        Point lb(faces[i].x + faces[i].width, faces[i].y + faces[i].height);
        Point tr(faces[i].x, faces[i].y);
        
        Mat roi(inp, Rect(faces[i].x, faces[i].y, faces[i].width, faces[i].height));
        Mat result;
        roi.copyTo(result);
        
        ret.push_back(result);
    }
    
    return ret;
}
void train(){
    string train_path = path + "db.txt";
    string model_path = path + "LBPHface.yml";
    vector <Mat> images;
    vector <int> labels;
    
    try{
        dbread(path, images, labels);
        cout << "size of the image is" << images.size() << endl;
        cout << "size of labels is " << labels.size() << endl;
        cout << "Training begins..." << endl;
    }catch(cv::Exception& e){
        cerr << e.msg << endl;
        exit(1);
    }
    
    model->train(images, labels);
    model->save(model_path);
    model->read(model_path);
    cout << "training finished..." << endl;
}
void getUserImage(int label){
    int cnt = 100;
    while(1){
        cap.read(inp);
        if(inp.empty()) {
            cerr << "can't read from camera" << endl;
            break;
        }
        if(cnt >= 200) break;
        vector <Mat> out = faceDetect(inp);
        if(!out.empty() && cnt < 200){
            cnt++;
            Mat res = out[0];
            resize(res, res, Size(200, 200), 0, 0, CV_INTER_LINEAR);
            cvtColor(res, res, COLOR_BGR2GRAY);
            string rpath = path + "pic/" + to_string(cnt) + ".jpg";
    
            dbout << rpath << ";" << label << endl;
            imshow("face", res);
            imwrite(rpath, res);
        }
        if(waitKey(25) >= 0) break;
    }
}
void welcome_menu(){
    cout << "[FACE RECOGNITION ATTENDANCE CHECK]" << endl;
    cout << "1. REGISTRATION MODE" << endl;
    cout << "2. ATTENDANCE CHECK" << endl;
    cout << "3. CONFIGURATION INFO" << endl;
    cout << "4. EXIT" << endl;
}
pair<int, int> time_split(char* time_c){
    int temp = 0;
    pair <int, int> result;
    
    temp = atoi(time_c);
    result.first = temp / 100;
    temp = temp - result.first * 100;
    result.second = temp;
    
    return result;
}
int reg_mode(int s_sockfd){
    char req_msg[] = "REQ REG";
    char recv_msg[MAXLINE];
    
    int student_num = 0;
    char usr_name[MAXLINE];
    
    // send request message
    if(-1 == write(s_sockfd, req_msg, strlen(req_msg)+1)){
        cout << "[ERROR] REQUEST REGISTRATION MODE FAILED" << endl;
        return -1;
    }
    
    // receive reg ok message
    if((-1 == read(s_sockfd, recv_msg, 11)) && (0 != strncmp(recv_msg, "REG MODE OK", 11))){
        cout << "[ERROR] RECEIVE REG OK MSG FAILED" << endl;
        return -1;
    }
    
    cout << "USER STUDENT NUM >> "; cin >> student_num;
    cout << "USER NAME        >> "; cin >> usr_name;
    
    sprintf(req_msg, "%d %zu %s", student_num, strlen(usr_name), usr_name);
    
    if(-1 == write(s_sockfd, req_msg, strlen(req_msg)+1)){
        cout << "[ERROR] SEND USER INFO ERROR" << endl;
        return -1;
    }
    
    // RESULT MESSAGE
    if(-1 == read(s_sockfd, recv_msg, 6)){
        cout << "[ERROR] READ MSG FAILED" << endl;
        return -1;
    }
    else{
        if(0 == strncmp(recv_msg, "REG OK", 6)){
            cout << "[OK] REGISTERED " << endl;
            getUserImage(student_num);
            train();
            return 1;
        }
        else if(0 == strncmp(recv_msg, "REG DP", 6)){
            cout << "[ERROR] DUPLICATED" << endl;
            return -1;
        }
        else{
            cout << "[ERROR] REG -> EXCEPTION" << endl;
            return -1;
        }
    }
}
int att_mode(int s_sockfd){
    // checking attendance about 1 minute
    clock_t delay = 60 * CLOCKS_PER_SEC;
    clock_t start = clock();
    
    char req_msg[] = "REQ ATT";
    char recv_msg[MAXLINE];
    
    if(-1 == write(s_sockfd, req_msg, strlen(req_msg)+1)){
        cout << "[ERROR] REQUEST ATTENDANCE CHECK MODE FAILED" << endl;
        return -1;
    }
    if((-1 == read(s_sockfd, recv_msg, 10)) && (0 != strncmp(recv_msg, "REQ ATT OK", 10))){
        cout << "[ERROR] RECEIVE CHECK SIGN FAILED" << endl;
        return -1;
    }
    while((clock() - start) < delay){
        cap >> inp;
        imshow("window", inp);
        vector <Mat> out = faceDetect(inp);
        
        if(!out.empty()){
            Mat res;
            cvtColor(out[0], res, COLOR_BGR2GRAY);
            
            int label = -1;
            double confidence;
            model->predict(res, label, confidence);
            
            string display = to_string(confidence) + "% Confience it is user";
            putText(inp, display, Point(100, 120), FONT_HERSHEY_COMPLEX, 1.2, Scalar::all(255));
            
            if(-1 == label){
                // label을 청구했을 때 이름 string 가져올 것
                sprintf(req_msg, "%d", label);
                if(-1 == write(s_sockfd, req_msg, strlen(req_msg)+1)){
                    cout << "[ERROR] SEND LABEL FAILED" << endl;
                    return -1;
                }
                if(-1 == read(s_sockfd, recv_msg, MAXLINE)){
                    cout << "[ERROR] RECEIVE LABEL FAILED" << endl;
                    return -1;
                }
                else{
                    putText(inp, recv_msg, Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
                    imshow("facedetection", inp);
                }
            }
            else{
                putText(inp, "unknown", Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
                imshow("facedetection", inp);
            }
        }
        else{
            putText(inp, "face not found", Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
            imshow("facedetection", inp);
        }
        
        if(waitKey(25) >= 0) break;
    }
    strncpy(req_msg, "KILL", 5);
    if(-1 == write(s_sockfd, req_msg, strlen(req_msg)+1)){
        cout << "[ERROR] REQUEST ATTENDANCE CHECK MODE FAILED" << endl;
        return -1;
    }
    
    return 1;
}
int config_mode(int s_sockfd){
    char req_msg[] = "REQ CONFIG";
    char recv_msg[MAXLINE];
    pair<int, int> temp_time;
    
    if(-1 == write(s_sockfd, req_msg, strlen(req_msg)+1)){
        cout << "[ERROR] REQUEST CONFIG INFO FAILED" << endl;
        return -1;
    }
    // CONFIG DATA EXAMPLE(20byte) : CP33004 0900 1300 40
    if(-1 == read(s_sockfd, recv_msg, 20)){
        cout << "[ERROR] RECEIVE CONFIG INFO FAILED" << endl;
        return -1;
    }
    char *token = strtok(recv_msg, " ");    // subject code
    strncpy(sys_ConfData.s_code, token, 7);
    
    token = strtok(NULL, " ");          // start time
    temp_time = time_split(token);
    sys_ConfData.s_time.hour = temp_time.first;
    sys_ConfData.s_time.min = temp_time.second;
    
    token = strtok(NULL, " ");          // finish time
    temp_time = time_split(token);
    sys_ConfData.f_time.hour = temp_time.first;
    sys_ConfData.f_time.min = temp_time.second;
    
    token = strtok(NULL, " ");          // number of students
    sys_ConfData.s_num = atoi(token);
    
    return 1;
}
int main(int argc, char **argv){
    struct sockaddr_in serveraddr;
    int server_sockfd, client_len, menu_selector;

    if(!cap.isOpened()) cerr << "can't open camera device" << endl;

    if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("error : ");
        return 1;
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serveraddr.sin_port = htons(4000);
    
    client_len = sizeof(serveraddr);
    if(connect(server_sockfd, (struct sockaddr*)&serveraddr, client_len) == -1) {
        perror("connect error : ");
        return 1;
    }
    
    do{
        welcome_menu();
        cout << ">> ";
        cin >> menu_selector;
        switch(menu_selector){
            case 1:{ // REGISTRATION MODE
                reg_mode(server_sockfd);
                break;
            }
            case 2:{ // ATTENDANCE CHECK
                att_mode(server_sockfd);
                break;
            }
            case 3:{ // CONFIGRATION MODE
                if(-1 != config_mode(server_sockfd)){
                    cout << "[CONFIGURATION INFO]" << endl;
                    cout << "CODE : " << sys_ConfData.s_code << endl;
                    cout << "START : "
                    << sys_ConfData.s_time.hour << " - " << sys_ConfData.s_time.min << endl;
                    cout << "FINISH : "
                    << sys_ConfData.f_time.hour << " - " << sys_ConfData.f_time.min << endl;
                    cout << "NUM OF STUDENTS : " << sys_ConfData.s_num << endl;
                }
                break;
            }
            case 4:{
                char op_msg[] = "EXIT";
                if(-1 == write(server_sockfd, op_msg, strlen(op_msg)+1)){
                    cout << "[ERROR] DISCONNECT REQUEST FAILED" << endl;
                    return -1;
                }
                else return 0;
            }
        }
    }while(menu_selector != 4);
    
    close(server_sockfd);
    return 0;
}
