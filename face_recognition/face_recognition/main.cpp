#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/face.hpp>

#include <vector>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctime>

#define MAXLINE 1024
#define CV_HAAR_SCALE_IMAGE 2
#define CV_INTER_LINEAR 1

using namespace std;
using namespace cv;

auto model = face::LBPHFaceRecognizer::create();

VideoCapture cap(0);
Mat inp;
string path = "/Users/yiseo/my_code/face_recognition/";
ofstream dbout(path + "db.txt", ios::app);
struct tm *curr_tm;

static void dbread(const string& filename, vector<Mat>& images, vector<int>& labels, char separator = ';'){
    ifstream file(filename);
    
    if (!file){
        string error = "no valid input file";
        cout << error << endl;
        exit(0);
    }
    
    string line, path, label;
    while (!file.eof())
    {
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
    vector <Rect> faces, crop;
    vector <Mat> ret;
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
     string path = "/Users/yiseo/my_code/face_recognition/db.txt";
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
    model->save("/Users/yiseo/my_code/face_recognition/LBPHface.yml");
    model->read("/Users/yiseo/my_code/face_recognition/LBPHface.yml");
    cout << "training finished..." << endl;
}

// 012 function
void getUserImage(){
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
    
            dbout << rpath << ";" << "50" << endl;
            imshow("face", res);
            imwrite(rpath, res);
        }
        if(waitKey(25) >= 0) break;
    }
}
void getCurrentTime(){
    time_t curr_time;
    curr_time = time(NULL);
    curr_tm = localtime(&curr_time);
    
    /*
     curr_time -> tm_year + 1900
     curr_time -> tm_mon + 1;
     curr_time -> tm_mday;
     
     curr_time -> tm_hour;
     curr_time -> tm_min;
     curr_time -> tm_sec;
     */
}


void welcome_menu(){
    cout << "[FACE RECOGNITION ATTANDANCE CHECK]" << endl;
    cout << "1. REGISTRATION MODE" << endl;
    cout << "2. ATTANDANCE CHECK" << endl;
    cout << "3. CONFIGURATION MODE" << endl;
    cout << "4. EXIT" << endl;
}

int main(int argc, char **argv){
    /*
     Initialize
     */
    struct sockaddr_in serveraddr;
    int server_sockfd, client_len, menu_selector;
    char buf[MAXLINE];
    
    // 올렸던 변수들 자리
    
    if(!cap.isOpened()) cerr << "cant open camera device" << endl;

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
    // ---------------------------------------------------
    
    do{
        welcome_menu();
        cin >> menu_selector;
        switch(menu_selector){
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
        }
    }while(menu_selector != 3);
    
    getUserImage();
    train();
    while(1){
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
            // label을 청구했을 때 이름 string 가져올 것
            if(label == 40){
                putText(inp, "Dohyeon", Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
                imshow("facedetection", inp);
            }
            else if (label == 50){
                putText(inp, "012", Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
                imshow("facedetection", inp);
            }
            else{
                putText(inp, "unknown", Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
                imshow("facedetection", inp);
            }
        }
        else{
            // cout << "face not detected" << endl;
            putText(inp, "face not found", Point(250,450), FONT_HERSHEY_COMPLEX, 1.2, Scalar(0,255,0));
            imshow("facedetection", inp);
        }
        
        if(waitKey(25) >= 0) break;
    }
    
    memset(buf, 0x00, MAXLINE);
    read(0, buf, MAXLINE);
    if(write(server_sockfd, buf, MAXLINE) <= 0) {
        perror("write error : ");
        return 1;
    }
    memset(buf, 0x00, MAXLINE);
    if(read(server_sockfd, buf, MAXLINE) <= 0) {
        perror("read error: ");
        return 1;
    }
    close(server_sockfd);
    printf("server:%s\n", buf);

    return 0;
}
