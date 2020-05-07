#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ssu_mntr.h"

char mntrDir[PATHLEN]; //모니터링 디렉토리 절대경로
char mntrName[FILELEN] = "check"; //모니터링 디렉토리 이름
char startDir[PATHLEN]; //시작 디렉토리 절대경로

void ssu_mntr()
{
	char command[BUFLEN];
	char *prompt = "20162444>";
	char **argv;
	int argc, cmd;
	pid_t pid;
	FILE *fp;

	//전역변수 초기화
	getcwd(startDir, PATHLEN);
	getcwd(mntrDir, PATHLEN);
	strcat(mntrDir, "/"); 
	strcat(mntrDir, mntrName); 

/*	if ((fp = fopen("check", "r+")) == NULL) {  //TODO: 디렉토리 있는지 확인
		if ((fp = fopen("check", "w+")) == NULL) {
			fprintf(stderr, "fopen error for check\n");
			exit(1);	
		}

//		if ((pid = fork()) == 0) { //최초 실행시 모니터링을 위한 디몬 프로세스 실행
//TODO			execl("./daemon", "./daemon", (char*)0); //디몬 실행
//		}
	}
	fclose(fp);*/

	while (1) {
		argc = 0;
		fputs(prompt, stdout);
		fgets(command, sizeof(command), stdin);

		argv[++argc] = strtok(command, " ");
		while ((argv[++argc] = strtok(NULL, " ")) != NULL);  //입력받은 문자를 공백기준으로 나누기
		rtrim(argv[--argc]); //마지막에 개행문자 제거

		if((cmd = execute_command(argc, argv)) < 0)
			break;
		else if(cmd == 0) //이외의 명령어는 help
			cmd_help();
	}

	fprintf(stdout, "모니터링 종료.\n");
	fflush(stdout);
	return;
}

void rtrim(char *_str)
{
	char *start;

	start = _str;
	while (!isspace(*start)) //문자열 개행문자, 공백, 탭 등의 문자가 아닐 때까지 이동
		++start;
	
	*start = 0;  //공백문자 제거
}

int execute_command(int argc, char *argv[]) //명령어 구분하여 각각의 명령어 실행함수 호출
{
	int i;
	if (strcmp(argv[1], "exit") == 0) //프로그램 종료
		return -1;
	else if (strcmp(argv[1], "delete") == 0) //삭제 명령어: DELETE [FILENAME] [END_TIME] [OPTION] 
		cmd_delete(argc, argv);
	else if (strcmp(argv[1], "size") == 0) //크기 명령어: SIZE [FILENAME] [OPTION]
		cmd_size(argc, argv);
	else if (strcmp(argv[1], "recover") == 0) //복구 명령어: RECOVER [FILENAME] [OPTION]
		cmd_recover(argc, argv);	
	else if (strcmp(argv[1], "tree") == 0) //트리 명령어: TREE
		cmd_tree(argc, argv);
	else 
		return 0;

	return 1;
}
//TODO: 명령어 인자 에러 처리
void cmd_help(void) //명령어 사용법 출력
{
}

void cmd_delete(int argc, char *argv[]) //삭제 명령어 실행
{
	if (argc < 2) { //삭제 명령어 인자 부족시 help 출력
		cmd_help();
		return;
	}
	
}

void cmd_size(int argc, char *argv[]) //크기 명령어 실행
{
	
}

void cmd_recover(int argc, char *argv[]) //복구 명령어 실행
{
}

void cmd_tree(int argc, char *argv[]) //트리 명령어 실행하여 디렉토리 구조 출력
{
	int depth = 0;

	if (argc != 1) { //인자가 맞지 않으면 help출력
		cmd_help();
		return;
	}

	chdir(startDir);  //디렉토리 이동
	print_tree(mntrName, depth); //구조출력 재귀함수
}

void print_tree(char *path, int depth) //트리 구조 출력
{
	struct dirent **items;
	int nitems, i, j;

	if (chdir(path) < 0) { //디렉토리 이동 안 될 때 
		fprintf(stderr, "chdir error %s\n", path);
		return;
	}

	nitems = scandir(".", &items, NULL, alphasort); //알파벳 순서로 items에 저장 

	if(depth == 0) //최초 모니터링 디렉토리 경로 출력
		printf("|%s\n", path);

	else if(nitems == 0) { //최초 디렉토리가 아닌데 하위 파일이 없을 경우
		for(j = 0; j < depth; j++) //구조 출력
			printf("|         ");
		printf("|-----------%s\n", path); 
		return;	
	}

	for (i = 0; i < nitems; i++) { //하위 파일 출력
		struct stat statbuf;

		if ((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, ".."))) //현재, 부모디렉토리 제외
			continue;

		for(j = 0; j < depth; j++) //구조 출력
			printf("|         ");
		printf("|-----------%s\n",items[i]->d_name); 

		lstat(items[i]->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode)) //디렉토리면 재귀
			print_tree(items[i]->d_name, depth + 1);
	}

	chdir("..");
}
