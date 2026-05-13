#include "common.h"
#include "safety.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MAX_CHILDREN 16

static int g_shmid=-1, g_semid=-1, g_qid=-1;
// initialise intersection state in shm ->
static IntersectionState *g_state=NULL;
static pid_t g_children[MAX_CHILDREN];
static int g_n_children=0;
static volatile sig_atomic_t g_shutdown=0;

static void on_signal(int s){
	(void)s;
	g_shutdown=1;
	if (g_state) g_state->shutdown_requested=1;
	}

static void cleanup_ipc(void){
	if(g_state && g_state!=(void*) -1) shmdt (g_state);
	if(g_shmid !=-1) shmctl(g_shmid, IPC_RMID, NULL);
	if(g_shmid !=-1) semctl(g_semid, 0, IPC_RMID, 0);
	if(g_qid != -1) msgctl(g_qid, IPC_RMID, NULL);
	}

// spawn children via fork() + execv()
static pid_t spawn(const char *p, char *const av[]);{
	pid_t pid=fork();
	if (pid==-1){
		perror("fork");
		cleanup_ipc();
		exit(1);}
	if (pid==0){
		execv(p, av);
		fprintf(stderr, "execv %s: %s\n", p, strerror(errno));
		_exit(127);}
	g_children[g_n_children++]=pid;
	return pid;
	}

int main(int argc, char *argv[]){
// argv parsing -> reject any invocation isn't "./traffic_system config.txt"
	if (argc!=2){
		fprintf(stderr, "Usage: %s <config.txt>\n", argv[0];
		return 1;}
	const char *cfg=argv[1];
	if (access(cfg, R_OK)1=0){
		perror(cfg);
		return 1;}

	key_t key = ftok(IPC_KEY_PATH, IPC_KEY_ID);
	if(key==-1){
		perror("ftok");
		return 1;}
	// create shmget IPC opject with permissions 0600 (owner-only)
	// same for semget & msgget
	// same pattern as the shipped parent.c
	g_shmid=shmget(key, sizeof(IntersectionState), IPC_CREAT|0600);
	if(g_shmid==-1){
		perror("shmget");
		return 1; }
	// initialise IntersectionState in shm
	// all lights RED,  phase ALL_RED, counters zero
	// children attach later and see a known state
	g_state=(IntersectionState*)shmat(g_shmid, NULL, 0);
	if(g_state==(void*)-1){
		perror("shmat");
		cleanup_ipc();
		return 1;}
	memset(g_state,0,sizeof *g_state);
	for(int d=0; d<N_LIGHTS; ++d) g_state->light[d]=RED;
	g_state->current_phase=PHASE_ALL_RED;
	g_state->phase_started_at=time(NULL);

	g_semid=semget(key, N_SEMS, IPC_CREAT|0600);
	if(g_semid==-1){
		perror("semget");
		cleanup_ipc();
		return 1; }
	// set all the semaphores to {1,0,0)
	// - mutex available, no events pending, no shutdown
	unsigned short iv[N_SEMS]={1,0,0};
	union semun a; a.array=iv;
	if(semctl(g_semid,0,SETALL,a)==-1){
		perror("SETALL");
		cleanup_ipc();
		return 1; }

	g_qid=msgget(key, IPC_CREAT|0600);
	if(g_qid==-1){
		perror("msgget");
		cleanup_ipc();
		return 1;}
	// install SIGINT/SIGTERM handler
	// - on ctrl-c the supervisor flips shutdown_requested in shm and start draining
	struct sigaction sa;
	memset(&sa,0,sizeof sa);
	sa.sa_handler=on_signal;
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);

	char shm[16], sem[16], q[16];
	snprintf(shm,sizeof shm,"%d",g_shmid);
	snprintf(sem,sizeof sem,"%d",g_semid);
	snprintf(q,  sizeof q,  "%d",g_qid);
	// spawn 10 children via spawn (fork + execv)
	// controller + 4 lights + vehicle + pedestrain + emergency + logger + guo
	// each gets the IPC ids as argv strings
	char *ctl[] = {"./controller", shm,sem,q,(char*)cfg,NULL};
	spawn("./controller", ctl);
	static const char *ds[N_LIGHTS]={"0","1","2","3"};
	for(int d=0; d<N_LIGHTS; ++d){
		char *la[]={"./light",shm,sem,q,(char*)ds[d],NULL};
		spawn("./light", la);
		}
	char *v[] ={"./vehicle",   shm,sem,q,(char*)cfg,NULL};
	char *pd[]={"./pedestrian",shm,sem,q,(char*)cfg,NULL};
	char *em[]={"./emergency", shm,sem,q,(char*)cfg,NULL};
	char *lg[]={"./logger",    shm,sem,q,(char*)cfg,NULL};
	char *gu[]={"./gui",       shm,sem,q,(char*)cfg,NULL};
	spawn("./vehicle",v);
	spawn("./pedestrian",pd);
	spawn("./emergency",em);
	spawn("./logger",lg);
	spawn("./gui",gu);

	fprintf(stderr,"[main] %d children launched\n", g_n_children);
	while(!g_shutdown){
		int st; pid_t p=wait(&st);
		if(p==-1){
			if(errno==EINTR) continue;
			break;}
		fprintf(stderr,"[main] child %d exited\n",(int)p); break;
	}
	g_state->shutdown_requested=1;
	for(int i=0;i<g_n_children;++i) kill(g_children[i], SIGTERM);
	sleep(1);
	for(int i=0;i<g_n_children;++i) kill(g_children[i], SIGKILL);
	// supervisor loop -> wait until either ctrl-c or any child dies
	// if anything dies we tear the whole system down
	// (matches kill other when one dies, from parent.c)
	while(wait(NULL)>0){}
	// SIGTERM everyone -> 1 s grace -> SIGKILL stragglers -> IPC_RMID on all 3 objects
	// no SysV IPC is left after exit
	cleanup_ipc();
	fprintf(stderr,"[main] clean exit\n");
	return 0;
}
