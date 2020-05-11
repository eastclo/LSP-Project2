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

char mntrName[FILELEN] = "check"; //모니터링 디렉토리 이름
char mntrDir[PATHLEN]; //모니터링 디렉토리 절대경로
char startDir[PATHLEN]; //시작 디렉토리 절대경로
char trashfilesDir[PATHLEN]; //trash에 files디렉토리
char trashinfoDir[PATHLEN]; //trash에 info디렉토리
char trashDir[PATHLEN];

char delFile[FILELEN]; //삭제 명령어에 사용, 삭제 대상 파일명

int ioption; //삭제 i옵션
int roption; //삭제 r옵션

void ssu_mntr()
{
	char command[BUFLEN];
	char *prompt = "20162444>";
	char *argv[7];
	int argc, cmd, i;
	pid_t pid;

	//전역변수 초기화
	getcwd(startDir, PATHLEN);
	sprintf(mntrDir, "%s/%s", startDir, mntrName);
	sprintf(trashDir, "%s/%s", startDir, "trash");
	sprintf(trashfilesDir, "%s/%s", trashDir, "files");
	sprintf(trashinfoDir, "%s/%s", trashDir, "info");

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
		memset(command, 0, BUFLEN);
		argc = 0;
		fputs(prompt, stdout);
		fgets(command, sizeof(command), stdin);

		if(command[0] == '\n')
			continue;

		argv[++argc] = strtok(command, " ");
		while ((argv[++argc] = strtok(NULL, " ")) != NULL)  //입력받은 문자를 공백기준으로 나누기
			if (argc == 6) { //최대 인자 5개
				fprintf(stderr, "argument error\n");
				cmd_help();
				continue;
			}

		rtrim(argv[--argc]); //마지막에 개행문자 제거

		if((cmd = execute_command(argc, argv)) < 0)
			break;
		else if(cmd == 0)  //이외의 명령어는 help
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

void cmd_help(void) //명령어 사용법 출력
{
	printf("usage:\n\n");
	printf("delete [FILENAME] [END_TIME] [OPTION] : FILENAME 삭제\n");
	printf("- FILENAME : 절대경로, 상대경로 가능\n");
	printf("- END_TIME : yyyy-mm-dd hh:mm\n");
	printf("- OPTION : -i, -r //둘 중 하나만 사용 가능\n\n");
	printf("size [FILENAME] [OPTION] : FILENAME 이하 파일 경로와 크기\n");
	printf("- FILENAME : 상대경로만 가능\n");
	printf("- OPTION : -d NUMBER //NUMBER는 자연수\n\n");
	printf("recover [FILENAME] [OPTION] : FILENAME 파일 복구\n");
	printf("- FILENAME : 파일명(경로x)\n");
	printf("- OPTION : -l //모든 파일 중 선택\n\n");
	printf("tree : 파일 구조 출력\n\n");
	printf("exit : 프로그램 종료\n\n");
}

void cmd_delete(int argc, char *argv[]) //삭제 명령어 실행
{
	char delFilePath[PATHLEN];
	unsigned int timer;

	memset(delFile, 0, sizeof(delFile)); //전역변수 초기화

	if (argc < 2 || argc >= 6) { //삭제 명령어 인자 부족하거나 많을 시 help 출력
		cmd_help();
		return;
	}

	strcpy(delFile, argv[2]); //전역변수에 파일명 저장
	if (delFile[0] != '/') //절대 경로가 아니면
		sprintf(delFilePath, "%s/%s", mntrDir, delFile); //삭제할 파일의 절대경로 저장
	else
		strcpy(delFilePath, delFile);

	if (access(delFilePath, F_OK) < 0) { //2번 째 인자 파일명 잘못된 경우 에러메시지
		fprintf(stderr, "%s isn't exit\n", delFilePath);
		return;
	}

	signal(SIGALRM, sig_delete);

	if (argc == 3 && argc == 5) {
		int idx = argc;
		if(strcmp(argv[idx], "-i") == 0) //-i옵션일 경우 전역변수에 옵션 저장
			ioption = true;
		if(strcmp(argv[idx], "-r") == 0) //-r옵션일 경우 전역변수에 옵션 저장
			roption = true;
		else { //옵션인자에 그 외 명령어가 들어가면 에러
			fprintf(stderr, "option error\n");
			cmd_help();
			return;
		}
	}

	if (argc >= 4) {
		if(check_time(argv[3], argv[4]) == false) {//인자 날짜형식인지 검사
			fprintf(stderr, "날짜 형식이 맞지 않습니다.\n");
			return;
		}
		if ((timer = get_timer(argv[3], argv[4])) < 0) {
			fprintf(stderr, "현재시간보다 과거의 시간 예약은 불가능합니다.\n");
			return;
		}
	}
	else
		timer = 0;

	if (timer > 0)
		alarm(timer); //삭제예약
	else
		delete_file(); //시간설정 없을 시 바로 삭제
}

int check_time(char *date, char *clock)	//yyyy-mm-dd hh-mm 형식이 맞는지 확인
{
	char ytmp[10]={0}, montmp[10]={0}, dtmp[10]={0}, htmp[10]={0}, mintmp[10]={0};
	int i, cnt = 0;
	//날짜 형식:숫자 조합과 -가 2개 있는지 확인
	for (i = 0; i < strlen(date); i++) { 
		if(('0' <= date[i] && date[i] <= '9') || date[i] == '-') {
			if(date[i] == '-')
				++cnt;
		}
		else
			return false;
	}
	if(cnt != 2) 
		return false;

	//시간 형식:숫자 조합과 :가 1개 있는지 확인
	cnt = 0;
	for (i = 0; i < strlen(clock); i++) {
		if(('0' <= clock[i] && clock[i] <= '9') || clock[i] == ':') {
			if(clock[i] == ':')
				++cnt;
		}
		else
			return false;
	}
	if(cnt != 1)
		return false;
	
	sscanf(date, "%[^-]%*c%[^-]%*c%s", ytmp, montmp, dtmp);
	sscanf(clock, "%[^:]%*c%s%*c", htmp, mintmp);

	if (atoi(montmp) < 1 &&  12 < atoi(montmp))
		return false;
	if (atoi(dtmp) < 1 &&  31 < atoi(dtmp))
		return false;
	if (atoi(htmp) < 0 &&  23 < atoi(htmp))
		return false;
	if (atoi(mintmp) < 0 &&  59 < atoi(mintmp))
		return false;
	
	return true;
}

int get_timer(char *date, char *clock) //현재시간과 입력시간의 차이 리턴
{
	char ytmp[10]={0}, montmp[10]={0}, dtmp[10]={0}, htmp[10]={0}, mintmp[10]={0}, stmp[10]={0};
	struct tm set = {0};
	time_t nowTime;
	int i, cnt = 0;

	nowTime = time(NULL);
	sscanf(date, "%[^-]%*c%[^-]%*c%s", ytmp, montmp, dtmp);

	for(i = 0; i < strlen(clock); i++)
		if(clock[i] == ':')
			cnt++;
	if(cnt == 1)
		sscanf(clock, "%[^:]%*c%s%*c", htmp, mintmp);
	else if(cnt == 2)
		sscanf(clock, "%[^:]%*c%[^:]%*c%s", htmp, mintmp, stmp);

	set.tm_year = atoi(ytmp) - 1900;
	set.tm_mon = atoi(montmp) - 1;
	set.tm_mday = atoi(dtmp);
	set.tm_hour = atoi(htmp);
	set.tm_min = atoi(mintmp);
	set.tm_sec = atoi(stmp);

	return mktime(&set) - nowTime; //현재시간부터 입력시간까지의 초 리턴
}

void sig_delete(int signo) //alarm에 의한 삭제 시그널 핸들러, delete_file()호출
{
	delete_file();
}

void delete_file(void) //삭제명령어, 전역변수에 저장한 파일을 삭제한다
{
	struct tm modi;
	struct tm now;
	struct stat statbuf;
	time_t nowTime;
	char infoFile[PATHLEN] ={0};
	char trashFile[PATHLEN] = {0};
	char delFilePath[PATHLEN] = {0};
	char fname[FILELEN]; //삭제할 파일명
	char *end;
	FILE *fp;
	int num;

	if (roption && ask_delete() == false) //r옵션 있을 경우 삭제 질문
		return;

	nowTime = time(NULL); //현재 시간 기록
	localtime_r(&nowTime, &now);

	//삭제할 파일 경로 저장
	if(delFile[0] != '/') 
		sprintf(delFilePath, "%s/%s", mntrDir, delFile);
	else
		strcpy(delFilePath, delFile);

	end = delFile + strlen(delFile) - 1;
	while (end != delFile && *(end-1) != '/') 
		--end;
	strcpy(fname, end);	

	lstat(delFilePath, &statbuf);
	//i옵션 있으면 바로 삭제
	if (ioption) {
		if(S_ISDIR(statbuf.st_mode)) { //i옵션에서 디렉토리면 에러메시지
			printf("delete: failed to delete : '%s' is Directory", fname);
			return;
		}
		else 
			unlink(delFilePath); //삭제		
	}
	else { //옵션 없을 때
		sprintf(trashFile, "%s/%s", trashfilesDir, fname);
		sprintf(infoFile, "%s/%s", trashinfoDir, fname);
		check_same_delete(trashFile); //중복 파일명이면 숫자_를 붙임
		check_same_delete(infoFile); //중복 파일명이면 숫자_를 붙임

		fp = fopen(infoFile, "w+"); //info파일 작성
		localtime_r(&statbuf.st_mtime, &modi);
		fprintf(fp, "[Trash info]\n");
		fprintf(fp, "%s\n", delFilePath);
		fprintf(fp, "D : %04d-%02d-%02d %02d:%02d:%02d\n", now.tm_year+1900, now.tm_mon+1, now.tm_mday+1, now.tm_hour, now.tm_min, now.tm_sec); 
		fprintf(fp, "M : %04d-%02d-%02d %02d:%02d:%02d\n", modi.tm_year+1900, modi.tm_mon+1, modi.tm_mday+1, modi.tm_hour, modi.tm_min, modi.tm_sec); 
		fclose(fp);

		if (rename(delFilePath, trashFile) < 0) { //trash로 이동
			fprintf(stderr, "rename error\n");
			return;
		}

		while (is_info_full() > 0) //info가 2kb이상이면
			erase_old_trash(); //files와 info에 오래된 파일 1개 삭제
	}
}

int ask_delete(void) //r옵션, 삭제시 확인 문구출력 
{
	char ans; 
	fprintf(stderr, "Delete [y/n]? ");
	scanf("%c", &ans);
	getchar();
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

void check_same_delete(char *path) //path 경로의 중복파일을 체크하여 중복 시 앞에 "숫자_"를 붙임
{
	char fname[FILELEN] = ""; 
	char number[3];
	char *end;
	int num = 2;    

	end = path + strlen(path) - 1;
	while (*(end-1) != '/')
		--end;
	strcpy(fname, end);
	memset(end, 0, strlen(end)); //파일명 지우고    
	strcat(path, "1_"); //숫자_파일명 붙이기
	strcat(path, fname);

	while (access(path, F_OK) == 0) { //파일명이 겹칠경우
		memset(end, 0, strlen(end));      
		sprintf(number, "%d_", num++);
		strcat(path, number); //파일명 앞에 "숫자_"
		strcat(path, fname);
	}   
}

int is_info_full(void) //info폴더가 2kb이상이면 true리턴 
{
	if(get_directory_size(trashinfoDir) < 2048)
		return false;
	return true;
}

void erase_old_trash(void) //info에서 가장 오래된 파일을 찾아 files와 info 둘다 삭제
{
	char files[PATHLEN];
	char info[PATHLEN];
	char fname[FILELEN];
	struct stat statbuf;
	struct dirent **items;
	int nitems, i;
	time_t oldest;

	nitems = scandir(trashinfoDir, &items, NULL, alphasort); //내부 파일 목록 가져오기

	oldest = time(NULL); //현재시간으로 초기화
	for (i = 0; i < nitems; i++) {
		char childPath[PATHLEN];

		if (items[i]->d_name[0] == '.') //숨김파일 제외
			continue;

		sprintf(childPath, "%s/%s", trashinfoDir, items[i]->d_name); 
		lstat(childPath, &statbuf);

		if (oldest > statbuf.st_mtime) {	 //파일 수정시간이 저장된 시간보다 오래될 경우
			oldest = statbuf.st_mtime; //해당 파일 시간 저장
			strcpy(fname, items[i]->d_name); //해당 파일 이름 저장
			fname[strlen(items[i]->d_name)] = 0; //마지막에 NULL삽입
		}	
	}

	//가장 오래된 파일인 fname을 삭제
	sprintf(info, "%s/%s", trashinfoDir, fname);
	sprintf(files, "%s/%s", trashfilesDir, fname);

	unlink(info); //info파일은 텍스트 파일이므로 바로 삭제

	lstat(files, &statbuf);
	if (S_ISDIR(statbuf.st_mode)) //file 파일이 디렉토리일 경우
		rmdirs(files); //디렉토리 삭제 함수 호출
	else
		unlink(files);

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);
}

void rmdirs(const char *path) //디렉토리 삭제 함수
{ //rmdir은 빈 디렉토리를 삭제. 이 함수는 비어있지 않은 디렉토리도 삭제한다.
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[PATHLEN];

	if((dp = opendir(path)) == NULL) //디렉토리 오픈
		return;

	while((dirp = readdir(dp)) != NULL) //해당 디렉토리 내부 파일들을 가져온다.
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) //.과 ..은 제외
			continue;

		sprintf(tmp, "%s/%s", path, dirp->d_name);

		if(lstat(tmp, &statbuf) == -1) //해당 파일 자체를 삭제하기 위해 lstat사용
			continue;

		if(S_ISDIR(statbuf.st_mode)) //디렉토리면 재귀호출로 처리
			rmdirs(tmp);
		else
			unlink(tmp); //일반 파일이면 바로 삭제
	}

	closedir(dp);
	rmdir(path);
}

void cmd_size(int argc, char *argv[]) //크기 명령어 실행
{
	char path[PATHLEN] = "./";
	struct stat statbuf;
	int i = 0;
	int limit = 0;
	int error = false;

	strcat(path, argv[2]);

	//인자 에러체크
	if (argc != 2 && argc != 4)  {
		fprintf(stderr, "argument error\n");
		error = true;
	}

	if (argc == 4) {
		if(strcmp(argv[3], "-d") == 0) {
			for(i = 0; i < strlen(argv[4]); i++)  
				if(argv[4][i] <= '0' && '9' <= argv[4][i]) 
					error = true;
			if(!error)
				limit = atoi(argv[4]);
		}
		else { 
			fprintf(stderr, "option error\n");
			error = true;
		}
	}
	if(access(path, F_OK) < 0) {
		fprintf(stderr, "There isn't %s\n", argv[2]);
		error = true;
	}

	if(error) {
		cmd_help();
		return;
	}

	printf("%ld\t%s\n", get_directory_size(path), path); //최초 파일 사이즈 출력
	print_size(path, 0, limit);
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

		if (items[i]->d_name[0] == '.') //숨김파일 제외 
			continue;

		sprintf(childPath, "%s/%s", path, items[i]->d_name);
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

size_t get_directory_size(char *path) //path 디렉토리 하위 파일의 합을 리턴
{
	struct stat statbuf;
	struct dirent **items;
	int nitems, i;
	size_t ret = 0;

	nitems = scandir(path, &items, NULL, alphasort); //내부 파일 목록 가져오기

	for (i = 0; i < nitems; i++) {
		char childPath[PATHLEN];

		if (items[i]->d_name[0] == '.') //숨김파일 제외
			continue;

		sprintf(childPath, "%s/%s", path, items[i]->d_name);
		lstat(childPath, &statbuf);

		if (S_ISDIR(statbuf.st_mode))
			ret += get_directory_size(childPath); //디렉토리면 재귀의 리턴 값
		else
			ret += statbuf.st_size; //디렉토리가 아니면 파일 자체의 사이즈
	}

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);

	return ret;
}

void cmd_recover(int argc, char *argv[]) //복구 명령어 실행
{
	char info[PATHLEN];
	char files[PATHLEN];
	char recoverPath[PATHLEN];
	char fname[FILELEN];
	char tmp[FILELEN];
	FILE *fp;
	int count;

	if(argc != 2 && argc != 3) { //인자가 맞지 않으면 에러
		fprintf(stderr, "argument error\n");
		cmd_help();
		return;
	}

	strcpy(tmp, argv[2]);

	if (argc == 3) {
		if(strcmp(argv[3], "-l") == 0) //l옵션일 경우
			strcpy(fname, print_recover_question(tmp, true)); //l옵션 인자를 주어 파일 전체에서 선택
		else { //옵션 에러
			fprintf(stderr, "option error\n");	
			cmd_help();
			return;
		}
	}

	else {
		if ((count = get_file_count(trashfilesDir, tmp)) == 0) {
			fprintf(stderr, "There isn't <%s> file\n", argv[2]);	
			return;
		}
		else if (count > 1) //입력한 파일과 같은 파일이 여러개일 경우			
			strcpy(fname, print_recover_question(tmp, count)); //선택한 파일명을 가져온다
		else {
			get_file_name(trashfilesDir, tmp); //파일이 하나일 경우
			strcpy(fname, tmp);
		}
	}	

	//info와 files에 있는 fname파일 경로 저장
	sprintf(files, "%s/%s", trashfilesDir, fname);
	sprintf(info, "%s/%s", trashinfoDir, fname);

	fp = fopen(info, "r");	
	fscanf(fp, "%[^\n]%*c", recoverPath);
	fscanf(fp, "%[^\n]%*c", recoverPath); //두 번째 라인에 경로가 있으므로
	fclose(fp);

	//복구 작업
	check_same_recover(recoverPath); //복구 자리에 중복 파일이 있을 경우 파일명 수정
	if (is_parent_dir(recoverPath) == 0) {
		fprintf(stderr, "failed to recover: There isn't parent directory\n");
		return;
	}
	unlink(info);
	rename(files, recoverPath);
}

char *print_recover_question(char *fname, int count) //fname과 같은 이름의 파일에서 선택한 파일명 리턴, l옵션시 전체파일
{
	struct stat statbuf;
	struct dirent **items;
	char buf[BUFLEN];
	char **files;
	char *tmp;
	FILE *fp;
	int nitems, i, cnt, idx = 0;

	nitems = scandir(trashinfoDir, &items, NULL, alphasort); //info의 파일목록 불러오기

	//count가 1이면 전체 파일 개수만큼
	//아니면 count만큼 files 동적할당
	if (count == 1) cnt = nitems - 2; //'.', '..'제외 
	else cnt = count; 
	files = (char **)calloc(cnt, sizeof(char *)); 
	for (i = 0; i < cnt; i++)
		files[i] = (char *)calloc(PATHLEN, sizeof(char));

	//파일 탐색
	for (i = 0; i < nitems; i++) {
		char path[PATHLEN];
		if (items[i]->d_name[0] == '.') //숨김파일 제외
			continue;

		if (count > 1 && strcmp(fname, items[i]->d_name + 2) != 0)  //count가 1이 아니면 fname과 같은 파일만 체크
			continue;

		//files에 <파일명> <삭제시간> <수정시간> 저장
		sprintf(path, "%s/%s", trashinfoDir, items[i]->d_name);
		fp = fopen(path, "r"); 

		if (count == 1) { // l옵션 시
			fscanf(fp, "%[^\n]%*c", buf); //[Trash info]
			fscanf(fp, "%[^\n]%*c", buf); //파일 경로
			fscanf(fp, "%s", buf); //D
			fscanf(fp, "%s", buf); //:
			fscanf(fp, "%s", buf); //삭제 날짜
			sprintf(files[idx], "%s %s ", items[i]->d_name, buf);
			fscanf(fp, "%s", buf); //삭제 시간
			strcat(files[idx], buf);
		}
		else { //l옵션 아닐 때
			fscanf(fp, "%[^\n]%*c", buf); //[Trash info]
			fscanf(fp, "%[^\n]%*c", buf); //파일 경로
			fscanf(fp, "%[^\n]%*c", buf); //삭제 시간
			sprintf(files[idx], "%s %s ", items[i]->d_name, buf);
			fscanf(fp, "%[^\n]%*c", buf); //수정 시간
			strcat(files[idx], buf);
		}
		fclose(fp);	
		++idx;
	}

	sort_by_dates_modified(files, cnt); //files를 오래된 순으로 정렬
	idx = select_recover_file(files, cnt); //복구 파일 선택 질문 후 결과 리턴

	tmp = strtok(files[idx], " "); //삭제할 파일명 가져오기
	strcpy(fname, tmp);

	//동적 메모리 해제
	for(i = 0; i < cnt; i++)
		free(files[i]);
	free(files);

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);

	return fname; 
}

int get_file_count(char *path, char *fname) //path에 fname과 같은 파일 개수 리턴
{
	struct dirent **items;
	int nitems, i, count = 0;

	nitems = scandir(path, &items, NULL, alphasort);

	for (i = 0; i < nitems; i++)  
		if (strcmp(items[i]->d_name + 2, fname) == 0)
			count++;

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);

	return count;
}

void get_file_name(char *path, char *fname) //path경로에서 "숫자_"가 fname 파일 이름 리턴
{
	struct dirent **items;
	int nitems, i, count;

	nitems = scandir(path, &items, NULL, alphasort);

	for (i = 0; i < nitems; i++)  
		if (strcmp(items[i]->d_name + 2, fname) == 0) {
			strcpy(fname, items[i]->d_name);
			break;
		}

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);
}

void sort_by_dates_modified(char **files, int size) //files를 오래된 순으로 정렬
{
	int i, j;
	char tmp[PATHLEN];
	int time1, time2;

	//버블정렬
	for (i = 0; i < size - 1; i++) {
		for (j = 0; j < size - 1 - i; j++) {
			time1 = get_timer_in_info(files[j]); //삭제시간 - 현재시간을 구함, 음수
			time2 = get_timer_in_info(files[j+1]); //오래된 파일이 더 작은 값을 가짐 

			if(time1 > time2) { //오름차순 정렬
				memcpy(tmp, files[j], PATHLEN);	
				memcpy(files[j], files[j+1], PATHLEN);
				memcpy(files[j+1], tmp, PATHLEN);
			}
		}
	}
}

int get_timer_in_info(char *str) //str에서 삭제시간을 추출 후 deltime - currtime을 리턴
{
	char tmp[PATHLEN];
	char* date;
	char* clock;
	int ret;

	strcpy(tmp, str);
	date = strtok(tmp, " "); //파일명
	date = strtok(NULL, " "); //D or 날짜
	if (strcmp(date, "D") == 0) {
		date = strtok(NULL, " "); //:
		date = strtok(NULL, " "); //날짜
		clock = strtok(NULL, " "); //시각
	}
	else
		clock = strtok(NULL, " "); //시각

	return get_timer(date, clock); //날짜, 시각 문자열과 현재시간과의 차를 구함
}

int select_recover_file(char **files, int size) //복구 파일 선택 질문 후 결과 리턴
{
	int i, ans;
	for (i = 0; i < size; i++)
		printf("%d. %s\n", i + 1, files[i] + 2); //원본 파일명으로 출력하기 위해 +2
	printf("Choose :");

	scanf("%d", &ans);
	getchar();

	if(0 < ans && ans <= size)
		return ans - 1; //인덱스이므로 -1리턴
	else  
		return select_recover_file(files, size); //잘못 입력하면 다시 선택
}


void check_same_recover(char *path) //path 경로의 중복파일을 체크하여 중복 시 앞에 "숫자_"를 붙임
{
	char fname[FILELEN] = "";
	char number[3];
	char *end;
	int num = 2;	

	if (access(path, F_OK) == 0) {
		end = path + strlen(path) - 1;
		while (*(end-1) != '/')
			--end;
		strcpy(fname, end);

		memset(end, 0, strlen(end)); //파일명 지우고	
		strcat(path, "1_"); //숫자_파일명 붙이기
		strcat(path, fname);

		while (access(path, F_OK) == 0) { //파일명이 겹칠경우
			memset(end, 0, strlen(end));	
			sprintf(number, "%d_", num++);
			strcat(path, number); //파일명 앞에 "숫자_"
			strcat(path, fname);
		}	
	}
}

int is_parent_dir(char *path) //path경로의 부모 디렉토리가 존재하지 않으면 false 리턴 
{
	char tmp[PATHLEN];
	char *end;
	
	strcpy(tmp, path);
	end = tmp + strlen(tmp) - 1;

	// '/'와 파일명 제거
	while(*end != '/') 
		*(end--) = 0;
	*end = 0;
	
	if (access(tmp, F_OK) < 0) //부모 디렉토리가 존재하지 않음
		return false;

	return true;
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

		if (items[i]->d_name[0] == '.') //숨김파일 제외
			continue;

		sprintf(childPath, "%s/%s", path, items[i]->d_name);
		lstat(childPath, &statbuf);

		for(j = 0; j < depth; j++) //구조 출력
			printf("|         ");
		printf("|----------%s\n",items[i]->d_name); 

		if (S_ISDIR(statbuf.st_mode))  //디렉토리면 재귀
			print_tree(childPath, depth + 1);
	}

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);
}
