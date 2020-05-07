#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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

	if(daemon_init() < 0) {
		fprintf(stderr, "daemon process isn't created\n");
		exit(1);
	}

	//log.txt파일 오픈
	//모니터링 시작
	while (1) {

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
