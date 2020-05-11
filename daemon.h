#ifndef DAEMON_H_
#define DAEMON_H_

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

#define ADD 2
#define MODIFY 3
#define DELETE 4

typedef struct Llist {
    struct node *head;
	struct node *cur;
    struct node *tail;
} Llist;

typedef struct node {
    struct node *next;
	struct node *prev;
    char file_name[PATHLEN]; //파일이름 저장
	time_t mtime; //최종 수정시간
} node;

int daemon_init(void); //디몬 프로세스 생성
int search_data(Llist *l, char *fname); //현재 리스트에 있으면 true, 없으면 false 리턴
void mntr_files(char *path, FILE *fp, Llist *list); //재귀를 통해 파일현황 모니터링

void write_log(FILE *fp, struct node *file, int status); //로그파일 작성
void delete_until_speciData(FILE *fp, Llist *list, char *fname); //cur부터 fname 전까지 노드삭제
void delete_remained(FILE *fp, Llist *list); //cur부터 tail까지 리스트에 남은 것 삭제
void add_list(FILE *fp, Llist* list, char *fname); //fname을 이름으로 하는 노드 추가 
int search_data(Llist *l, char *fname); //리스트에 fname이 있으면 true, 없으면 false 리턴
#endif
