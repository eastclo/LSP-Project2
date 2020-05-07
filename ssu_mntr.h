#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif
#ifndef false
	#define false 0
#endif

//기타 상수 선언
#define BUFLEN 1024
#define FILELEN 256
#define PATHLEN 4096

//구조체 선언

//함수 선언
void ssu_mntr();
int execute_command(int argc, char *argv[]); //명령어 구분하여 각각의 명령어 실행함수 호출
void cmd_help(); //명령어 사용법 출력
void cmd_delete(int argc, char *argv[]); //삭제 명령어 실행
void cmd_size(int argc, char *argv[]); //크기 명령어 실행
void cmd_recover(int argc, char *argv[]); //복구 명령어 실행

void cmd_tree(int argc, char *argv[]); //트리 명령어 실행하여 디렉토리 구조 출력 
void print_tree(char *path, int depth); //디렉토리 트리 구조 출력

void rtrim(char *_str); //문자열 끝에 개행문자 제거
#endif
