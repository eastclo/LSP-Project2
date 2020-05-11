#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif

#ifndef BUFLEN
	#define BUFLEN 100 
#endif
#ifndef FILELEN
	#define FILELEN 256
#endif
#ifndef PATHLEN
	#define PATHLEN 512 
#endif

#define TREECMD 1
#define SIZECMD 2

//함수 선언
void ssu_mntr();
int execute_command(int argc, char *argv[]); //명령어 구분하여 각각의 명령어 실행함수 호출

void cmd_help(); //명령어 사용법 출력

void cmd_delete(int argc, char *argv[]); //삭제 명령어 실행
int check_time(char *date, char *clock); //yyyy-mm-dd hh-mm 형식이 맞는지 확인
int get_timer(char *date, char *clock); //현재시간과 입력시간의 차이 리턴
void sig_delete(int signo); //alarm에 의한 삭제 시그널 핸들러, delete_file()호출
void delete_file(void); //삭제 명령어, 전역변수에 저장한 파일을 삭제한다
int ask_delete(void); //delete r옵션시 삭제 질문, y면 true, n은 false리턴
void check_same_delete(char *path); //path 파일명이 중복이면 앞에 "숫자_"를 붙임
int is_info_full(void); //info 폴더가 2kb이상이면 true리턴
void erase_old_trash(void); //가장 오래된 trash 1개 삭제
void rmdirs(const char *path); //디렉토리 삭제 함수

void cmd_size(int argc, char *argv[]); //크기 명령어 실행
void print_size(char *path, int depth, int limit); //디렉토리를 순회하며 size 출력 
size_t get_directory_size(char *path); //디렉토리 하위 파일의 합 리턴

void cmd_recover(int argc, char *argv[]); //복구 명령어 실행
char *print_recover_question(char *fname, int count); //fname과 같은 이름의 파일에서 선택한 파일명 리턴, count가 1이면 전체파일 중 선택
int get_file_count(char *path, char *fname); //path에 fname과 같은 파일 개수 리턴
void get_file_name(char *path, char *fname); //path경로에서 "숫자_"가 붙은 fname파일 이름 리턴
void sort_by_dates_modified(char **files, int size); //files를 오래된 순으로 정렬
int get_timer_in_info(char *str); //str에서 삭제시간을 추출 후 deltime - currtime을 리턴
int select_recover_file(char **files, int size); //복구 파일 선택 질문 후 결과 리턴
void check_same_recover(char *path); //path 파일명이 중복이면 앞에 "숫자_"를 붙임
int is_parent_dir(char *path); //path경로의 부모 디렉토리가 존재하지 않으면 false 리턴

void cmd_tree(int argc, char *argv[]); //트리 명령어 실행하여 디렉토리 구조 출력 
void print_tree(char *path, int depth); //디렉토리를 순회하며 트리 구조 출력 

void rtrim(char *_str); //문자열 끝에 개행문자 제거
#endif
