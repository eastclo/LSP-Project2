#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_mntr.h"

#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

int main(void)
{
	struct timeval begin_t, end_t;
	gettimeofday(&begin_t, NULL); //프로그램 시작시간 측정

	ssu_mntr();

	gettimeofday(&end_t, NULL); //프로그램 종료시간 측정
	ssu_runtime(&begin_t, &end_t); //수행시간 출력함수 호출

	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t) //프로그램 수행시간 출력함수
{
	end_t->tv_sec -= begin_t->tv_sec;  //시작시간 초와 종료시간초의 차이 계산

	if(end_t->tv_usec < begin_t->tv_usec){  //시작시과 종료시간 micro초차이 계산
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}

	end_t->tv_usec -= begin_t->tv_usec;
	printf("Runtime: %ld:%06ld(sec:usec)\n", end_t->tv_sec, end_t->tv_usec);
}
