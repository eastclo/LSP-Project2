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
char delFile[PATHLEN]; //삭제 명령어에 사용
char delTime[20]; //삭제 명령어에 사용
char trashfilesDir[PATHLEN]; //trash에 files디렉토리
char trashinfoDir[PATHLEN]; //trash에 info디렉토리
char trashDir[PATHLEN];

int ioption;
int roption;

void ssu_mntr()
{
	char command[BUFLEN];
	char *prompt = "20162444>";
	char **argv;
	int argc, cmd;
	pid_t pid;

	//전역변수 초기화
	getcwd(startDir, PATHLEN);
	getcwd(mntrDir, PATHLEN);
	getcwd(trashfilesDir, PATHLEN);
	getcwd(trashinfoDir, PATHLEN);
	getcwd(trashDir, PATHLEN);
	strcat(mntrDir, "/"); 
	strcat(mntrDir, mntrName); 
	strcat(trashDir, "/trash");
	strcat(trashfilesDir, "/trash/files");
	strcat(trashinfoDir, "/trash/info");

	if (access(mntrDir, F_OK) < 0) {
		mkdir(mntrDir, 0755);	
		//		if ((pid = fork()) == 0)  //최초 실행시 모니터링을 위한 디몬 프로세스 실행
		//		execl("./daemon", "./daemon", (char*)0); //디몬 실행
	}
	if (access(trashDir, F_OK) < 0) 
		mkdir(trashDir, 0755);
	if (access(trashfilesDir, F_OK) < 0) 
		mkdir(trashfilesDir, 0755);
	if (access(trashinfoDir, F_OK) < 0) 
		mkdir(trashinfoDir, 0755);

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

//TODO: 명령어 인자 에러 처리
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

void cmd_help(void) //명령어 사용법 출력
{
}

void cmd_delete(int argc, char *argv[]) //삭제 명령어 실행
{
	unsigned int timer;

	memset(delFile, 0, sizeof(delFile)); //전역변수 초기화
	memset(delTime, 0, sizeof(delTime)); //전역변수 초기화

	if (argc < 2 || argc >= 6) { //삭제 명령어 인자 부족하거나 많을 시 help 출력
		cmd_help();
		return;
	}

	sprintf(delFile, "%s/%s", mntrDir, argv[2]); //삭제할 파일의 절대경로 저장

	if (access(delFile, F_OK) < 0) { //2번 째 인자 파일명 잘못된 경우 에러메시지
		fprintf(stderr, "%s isn't exit", delFile);
		return;
	}

	signal(SIGALRM, delete_file);

	if (argc >= 4) {
		//TODO:반복문으로 인자 날짜형식인지 검사, argc == 3일 때 옵션부터해야하는데...
		timer = get_timer(argv[3], argv[4]);
		sprintf(delTime, "%s %s", argv[3], argv[4]); 
	}
	else
		timer = 0;

	if (argc == 5) {
		if(strcmp(argv[5], "-i") == 0) //-i옵션일 경우 전역변수에 옵션 저장
			ioption = true;
		if(strcmp(argv[5], "-r") == 0) //-r옵션일 경우 전역변수에 옵션 저장
			roption = true;
		else { //옵션인자에 그 외 명령어가 들어가면 에러
			cmd_help();
			return;
		}
	}

	alarm(timer); //삭제
}

unsigned int get_timer(char *date, char *clock)
{
	char ytmp[10], montmp[10], dtmp[10], htmp[10], mintmp[10], stmp[10];
	struct tm set = {0};
	time_t nowTime;

	nowTime = time(NULL);
	sscanf(date, "%[^-]%*c%[^-]%*c%s", ytmp, montmp, dtmp);
	sscanf(clock, "%[^:]%*c%[^:]%*c%s", htmp, mintmp, stmp);

	set.tm_year = atoi(ytmp) - 1900;
	set.tm_mon = atoi(montmp) - 1;
	set.tm_mday = atoi(dtmp);
	set.tm_hour = atoi(htmp);
	set.tm_min = atoi(mintmp);
	set.tm_sec = atoi(stmp);

	return mktime(&set) - nowTime; //현재시간부터 입력시간까지의 초 리턴
}

void delete_file(int signo)
{
	struct tm *now;
	struct stat statbuf;
	FILE *fp;
	int num;
	char delFileName[FILELEN];
	char infoFile[PATHLEN];
	char trashFile[PATHLEN];
	char *nameP = delFile + strlen(delFile) - 1;

	while (*(nameP-1) != '/')
		--nameP; //파일 이름만 추출

	num = get_file_count(nameP);//trash에 nameP와 같은 파일 개수 리턴 
	sprintf(delFileName, "%d_%s", num, nameP); 
	lstat(delFile, &statbuf);

	if (roption) { //r옵션 있을 경우 삭제 질문
		if (ask_delete() == false)		
			return;
	}		
	if (ioption) {
		if(S_ISDIR(statbuf.st_mode)) { //i옵션에서 디렉토리면 에러메시지
			printf("delete: failed to delete : '%s' is Directory", delFileName);
			return;
		}
		else 
			unlink(delFile); //삭제		
	}
	else { //옵션 없을 때
		sprintf(trashFile, "%s/%s", trashfilesDir, delFileName);
		sprintf(infoFile, "%s/%s", trashinfoDir, delFileName);

		fp = fopen(infoFile, "w+"); //info파일 작성
		now = localtime(&statbuf.st_mtime);
		fprintf(fp, "[Trash info]\n");
		fprintf(fp, "%s\n", delFile);
		fprintf(fp, "D : %s\n", delTime);
		fprintf(fp, "M : %d-%02d-%02d %02d:%02d:%02d\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday+1, now->tm_hour, now->tm_min, now->tm_sec); 
		fclose(fp);

		if (rename(delFile, trashFile) < 0) { //trash로 이동
			fprintf(stderr, "rename error\n");
			return;
		}

		//		while (is_info_full() > 0) //info가 2kb이상이면
		//			erase_old_trash(); //files와 info에 오래된 파일 1개 삭제
	}
}

int ask_delete(void)
{
	char ans; 
	printf("Delete [y/n]?");
	scanf("%c", &ans);
	getchar();
	switch (ans) {
		case 'y':
			return true;
		case 'n':
			return false;
		default:
			return ask_delete();
	}
}

int get_file_count(char *fname) //trash에 fname포함하여 같은 파일 개수 리턴
{
	char trash[PATHLEN];
	struct dirent **items;
	int nitems, i, count;
	
	nitems = scandir(trashfilesDir, &items, NULL, alphasort);

	count = 1;
	for (i = 1; i <= nitems; i++) {
		memset(trash, 0, PATHLEN);
		sprintf(trash, "%s/%d_%s", trashfilesDir, i, fname);
		if (access(trash, F_OK) == 0)
			count++;
	}
	
	return count;
}

int is_info_full(void)
{
	if(get_directory_size(delFile) < 2048)
		return false;
	return true;
}

void erase_old_trash(void)
{

}

void cmd_size(int argc, char *argv[]) //크기 명령어 실행
{
	char path[PATHLEN] = "./";
	struct stat statbuf;
	int limit = 0;

	strcat(path, argv[2]);
	lstat(path, &statbuf);
	printf("%ld\t%s\n", statbuf.st_size, path); //최초 파일 사이즈 출력

	if (argv[3] != NULL && strcmp(argv[3], "-d") == 0)
		limit = atoi(argv[4]);

	print_size(path, 0, limit);
}

int get_directory_size(char *path) //path 디렉토리 하위 파일의 합을 리턴
{
	struct stat statbuf;
	struct dirent **items;
	int nitems, i;
	size_t ret = 0;
	
	nitems = scandir(path, &items, NULL, alphasort); //내부 파일 목록 가져오기

	for (i = 0; i < nitems; i++) {
		char childPath[PATHLEN];

		if ((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, "..")))
			continue;

		sprintf(childPath, "%s/%s", path, items[i]->d_name);
		lstat(childPath, &statbuf);
		
		if (S_ISDIR(statbuf.st_mode))
			ret += get_directory_size(childPath); //디렉토리면 재귀의 리턴 값
		else
			ret += statbuf.st_size; //디렉토리가 아니면 파일 자체의 사이즈
	}

	return ret;
}

void cmd_recover(int argc, char *argv[]) //복구 명령어 실행
{
}

void cmd_tree(int argc, char *argv[]) //트리 명령어 실행하여 디렉토리 구조 출력
{
	if (argc != 1) { //인자가 맞지 않으면 help출력
		cmd_help();
		return;
	}

	printf("|%s\n", mntrName); //최초 모니터링 디렉토리 출력
	print_tree(mntrDir, 0); //구조출력 재귀함수
}

void print_tree(char *path, int depth) //디렉토리 순회
{
	struct stat statbuf;
	struct dirent **items;
	int nitems, i, j;

	nitems = scandir(path, &items, NULL, alphasort); //알파벳 순서로 items에 저장 

	for (i = 0; i < nitems; i++) { //하위 파일 출력
		char childPath[PATHLEN];

		if ((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, ".."))) //현재, 부모디렉토리 제외
			continue;

		sprintf(childPath, "%s/%s", path, items[i]->d_name);
		lstat(childPath, &statbuf);

		for(j = 0; j < depth; j++) //구조 출력
			printf("|         ");
		printf("|-----------%s\n",items[i]->d_name); 

		if (S_ISDIR(statbuf.st_mode))  //디렉토리면 재귀
			print_tree(childPath, depth + 1);
	}

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);
}

void print_size(char *path, int depth, int limit) //디렉토리 순회
{

	struct stat statbuf;
	struct dirent **items;
	int nitems, i, j;

	if(limit != 0 && depth + 1 == limit) //-d옵션일 경우 depth만큼 출력
		return;

	nitems = scandir(path, &items, NULL, alphasort); //알파벳 순서로 items에 저장

	for (i = 0; i < nitems; i++) { //하위 파일 출력
		char childPath[PATHLEN];

		if ((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, ".."))) //현재, 부모디렉토리 제외
			continue;

		strcpy(childPath, path); strcat(childPath, "/"); strcat(childPath, items[i]->d_name);
		lstat(childPath, &statbuf);

		if (S_ISDIR(statbuf.st_mode)) {  //디렉토리면 재귀
			printf("%ld\t%s\n", get_directory_size(childPath), childPath); //디렉토리는 하위 파일의 합 출력
			print_size(childPath, depth + 1, limit);
		}
		else //파일일 경우 그냥 출력 
			printf("%ld\t%s\n", statbuf.st_size, childPath); //size 출력
	}

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);
}
