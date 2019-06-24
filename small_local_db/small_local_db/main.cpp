//
//  main.cpp
//  small_local_db
//
//  Created by Young-il Seo on 21/06/2019.
//  Copyright © 2019 Young-il Seo. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>

using namespace std;

class Student{
protected:
    int id;
    string name;
    
public:
    Student(){ id = 0; name = ""; }
    Student(int _id, string _name){ id = _id; name = _name; }
    int get_student_id(){ return id; }
    string get_student_name(){ return name; }
    void set_student(int _id, string _name){ id = _id; name = _name; }
};

class Att_record : public Student{
private:
    tm att_time;
public:
    Att_record(int, string, tm);
};
Att_record::Att_record(int stu_id, string stu_name, tm _att_time){
    set_student(stu_id, stu_name);
    att_time = _att_time;
}

vector<Student> student_list;
vector<Att_record> attandance_clist;
struct tm *curr_tm;

void get_current_time(){
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
    cout << "3. EXIT" << endl;
}
int check_in_list(int _id){
    int i;
    for(i = 0; i < student_list.size(); i++){
        if(student_list[i].get_student_id() == _id) return i;
    }
    return -1;
}
void load_data(){
    ifstream inFile("user_list.dat");
    int temp_id;
    string temp_name;
    
    while(!inFile.eof()){
        inFile >> temp_id >> temp_name;
        student_list.push_back(Student(temp_id, temp_name));
    }
    inFile.close();
    
    // load att_list.dat
}
void reg_student(){
    int temp_id;
    string temp_name;
    
    while(1){
        cout << "[ID == 0 >> EXIT]" << endl;;
        cout << "ID : ";
        cin >> temp_id;
        if(temp_id == 0) break;
        if(check_in_list(temp_id) != -1){
            cout << "[ERROR] DUPLICATED" << endl;
            break;
        }
        cout << "NAME : ";
        cin >> temp_name;
        student_list.push_back(Student(temp_id, temp_name));
    }
}
void check_attandance(int _id){
    // 이미 지정된 시간대에 출석을 완료했다면 중복 출석체크를 방지하도록 해야함
    int position = check_in_list(_id);
    if(position != -1){
        get_current_time();
        Att_record temp(student_list[position].get_student_id(),
                        student_list[position].get_student_name(),
                        *curr_tm);
        attandance_clist.push_back(temp);
    }
    else cout << "[ERROR] NOT REGISTRATED" << endl;
    return;
}
void write_data(){
    // save att_list.dat;
}
int main(int argc, const char * argv[]) {
    int menu_selector = 0;
    int student_id = 0; // get id from face detection program
    load_data();
    do{
        welcome_menu();
        cin >> menu_selector;
        switch(menu_selector){
            case 1: // REGISTRATION MODE
                reg_student();
                break;
            case 2: // ATTANDANCE CHECK
                check_attandance(student_id);
                break;
            case 3:
                write_data();
                break;
        }
    }while(menu_selector != 3);
    
    
    return 0;
}
