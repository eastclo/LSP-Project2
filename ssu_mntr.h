#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif

#ifndef BUFLEN
	#define BUFLEN 1024
#endif
#ifndef FILELEN
	#define FILELEN 256
#endif
#ifndef PATHLEN
	#define PATHLEN 512 
#endif

#define TREECMD 1
#define SIZECMD 2

//구조체 선언

//함수 선언
void ssu_mntr();
int execute_command(int argc, char *argv[]); //명령어 구분하여 각각의 명령어 실행함수 호출
void cmd_help(); //명령어 사용법 출력

void cmd_delete(int argc, char *argv[]); //삭제 명령어 실행
void sig_delete(int signo); //alarm에 의한 삭제 시그널 핸들러, delete_file()호출
void delete_file(void); //전역변수에 저장한 파일을 삭제한다
int ask_delete(void); //delete r옵션시 삭제 질문, y면 true, n은 false리턴
int get_file_count(char *fname); //trash에 fname포함하여 같은 파일 개수 리턴
int is_info_full(void); //info 폴더가 2kb이상이면 true리턴
void erase_old_trash(void); //가장 오래된 trash 1개 삭제
void rmdirs(const char *path); //디렉토리 삭제 함수
unsigned int get_timer(char *date, char *clock); //현재시간과 입력시간의 차이 리턴

void cmd_size(int argc, char *argv[]); //크기 명령어 실행
void print_size(char *path, int depth, int limit); //디렉토리를 순회하며 size 출력 
int get_directory_size(char *path); //디렉토리 하위 파일의 합 리턴

void cmd_recover(int argc, char *argv[]); //복구 명령어 실행

void cmd_tree(int argc, char *argv[]); //트리 명령어 실행하여 디렉토리 구조 출력 
void print_tree(char *path, int depth); //디렉토리를 순회하며 트리 구조 출력 

void rtrim(char *_str); //문자열 끝에 개행문자 제거
#endif
