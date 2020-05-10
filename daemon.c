#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include "daemon.h"

int main(void)
{
	FILE *fp;
	char buf[BUFLEN];
	char mntrDir[BUFLEN];
	char logFile[BUFLEN];

	getcwd(buf, BUFLEN);
	strcpy(mntrDir, buf);
	strcpy(logFile, buf);
	strcat(mntrDir, "/check");
	strcat(logFile, "/log.txt");

	//	if(daemon_init() < 0) {
	//		fprintf(stderr, "daemon process isn't created\n");
	//		exit(1);
	//	}

	fp = fopen(logFile, "w+"); //log.txt파일 오픈
	setbuf(fp, NULL);

	Llist* list = (Llist*)malloc(sizeof(Llist)); //모니터링시 파일과 수정시간을 기록할 리스트 생성
	list->head = NULL;
	list->tail = NULL;

	//모니터링 시작
	while (1) {
		list->cur = list->head;
		mntr_files(mntrDir, fp, list);	
		usleep(500);
	}
	exit(0);
}

int daemon_init(void) //디몬프로세스 생성
{
	pid_t pid;
	int fd, maxfd;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}

	else if (pid != 0) //1. 백그라운드 실행
		exit(0);

	setsid(); //2. 새 세션생성
	signal(SIGTTIN, SIG_IGN); //3. 입출력 시그널 무시
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	for (fd = 0; fd < maxfd; fd++) //6. 오픈된 모든 fd닫기
		close(fd);

	umask(0); //4. 파일모드 생성 마스크 해제
	chdir("/"); //5. 현재 디렉토리를 루트 디렉토리로 설정
	fd = open("/dev/null", O_RDWR); //7. 표준 입출력과 표준에러를 /dev/null로 재지정
	dup(0);
	dup(0);
	return 0;
}

void mntr_files(char *path, FILE *fp, Llist* list)
{
	struct stat statbuf;
	struct dirent **items;
	int nitems, i;

	nitems = scandir(path, &items, NULL, alphasort); //알파벳 순서로 items에 저장

	for (i = 0; i < nitems; i++) { //하위 파일 출력
		char childPath[PATHLEN];

		if ((!strcmp(items[i]->d_name, ".")) || (!strcmp(items[i]->d_name, ".."))) //현재, 부모디렉토리 제외
			continue;

		strcpy(childPath, path); strcat(childPath, "/"); strcat(childPath, items[i]->d_name);
		lstat(childPath, &statbuf);

		if (list->cur == NULL) //cur == NULL이면 childPath추가
			add_list(fp, list, childPath);	
		else if (strcmp(list->cur->file_name, childPath) == 0) {//리스트에 이미 있으면 수정시간만 확인 
			if (list->cur->mtime != statbuf.st_mtime && list->cur->mtime < statbuf.st_mtime) { //수정시간이 다를 경우 
				list->cur->mtime = statbuf.st_mtime; //수정 시간 변경 
				write_log(fp, list->cur, MODIFY); //로그 기록 
				list->cur->mtime = statbuf.st_mtime; //리스트 갱신 
			}
		}
		else { //리스트에 없을 때
			if (search_data(list, childPath))  //list->cur삭제, childPath까지
				delete_until_speciData(fp, list, childPath); //삭제 및 삭제 로그 기록
			else { //childPath추가
				add_list(fp, list, childPath);
				list->cur->mtime = statbuf.st_mtime; //리스트 수정시간 갱신
			}
		}

		list->cur = list->cur->next; //cur이동
		if (S_ISDIR(statbuf.st_mode)) //디렉토리면 재귀
			mntr_files(childPath, fp, list);  
	}  

	for(i = 0; i < nitems; i++)
		free(items[i]);
	free(items);
}

void write_log(FILE *fp, node *file, int status)
{
	char buf[BUFLEN]; 
	char filePath[PATHLEN]; 
	char *fileName;
	struct tm *now;
	int i;

	now = localtime(&file->mtime);
	strcpy(filePath, file->file_name);
	fileName = filePath + strlen(filePath) - 1;
	while (*fileName != '/')
		--fileName;
	++fileName; //파일 이름만 가져오기

	switch (status) {
		case ADD: //추가
			sprintf(buf, "[%d-%02d-%02d %02d:%02d:%02d][create_%s]\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday+1, now->tm_hour, now->tm_min, now->tm_sec, fileName);
			break;
		case MODIFY: //수정
			sprintf(buf, "[%d-%02d-%02d %02d:%02d:%02d][modify_%s]\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday+1, now->tm_hour, now->tm_min, now->tm_sec, fileName);
			break;
		case DELETE: //삭제
			sprintf(buf, "[%d-%02d-%02d %02d:%02d:%02d][delete_%s]\n", now->tm_year+1900, now->tm_mon+1, now->tm_mday+1, now->tm_hour, now->tm_min, now->tm_sec, fileName);
			break;
	}

	fwrite(buf, strlen(buf), 1, fp);  //log.txt에 작성
}

void delete_until_speciData(FILE *fp, Llist *list, char *fname) 
{
	if(list->cur->prev == NULL) { //prev가 없을 때(cur가 head일 때)
		while (strcmp(list->cur->file_name, fname) != 0) {
			list->cur->mtime = time(NULL);
			write_log(fp, list->cur, DELETE); //삭제 로그 기록
			node *tmp = list->cur;
			list->cur = list->cur->next;
			free(tmp);
		}
		list->head = list->cur;
		list->cur->prev = NULL;
	}
	else {
		node *prev_file = list->cur->prev;
		while (strcmp(prev_file->next->file_name, fname) != 0) {
			prev_file->next->mtime = time(NULL);
			write_log(fp, prev_file->next, DELETE); //삭제 로그 기록
			node *tmp = prev_file->next;
			prev_file->next->next->prev = prev_file;
			prev_file->next = prev_file->next->next;
			free(tmp);
		}
		list->cur = prev_file->next; //list->cur == fname 
	}
}

void add_list(FILE *fp, Llist* list, char *fname)
{
	node *tmp = (node*)malloc(sizeof(node));
	strcpy(tmp->file_name, fname);
	tmp->mtime = time(NULL);
	if(list->head == NULL && list->tail == NULL) {
		tmp->prev = NULL;
		tmp->next = NULL;
		list->head = tmp;
		list->tail = tmp;
	}
	else if (list->cur == NULL) { //tail에 추가 해야하는 경우
		node *prev_file = list->tail;	
		prev_file->next = tmp;
		tmp->prev = prev_file;
		tmp->next = NULL;
		list->tail = tmp;
	}
	else if(list->cur->prev == NULL) { //list->cur가 head일 경우
		tmp->next = list->cur;
		tmp->prev = NULL;
		list->cur->prev = tmp;
		list->head = tmp;
	}
	else { //리스트 중간에 insert
		node* prev_file = list->cur->prev;
		prev_file->next = tmp;
		tmp->prev = prev_file;
		tmp->next = list->cur;
		list->cur->prev = tmp;
	}
	list->cur = tmp; //list->cur == fname
	write_log(fp, list->cur, ADD);
}

int search_data(Llist *l, char *fname) //리스트에 fname이 있으면 true, 없으면 false 리턴
{
	node *cur = l->head;
	while(cur != NULL) {
		if(strcmp(cur->file_name, fname) == 0) return true;
		cur = cur->next;
	}
	return false;
}
