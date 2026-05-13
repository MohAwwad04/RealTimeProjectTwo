// Blocks on msgrcv(MQ_LIGHT(dir)) → on EV_CMD_SET_COLOR runs safety_legal_transition → updates shm
// under the mutex → ACKs the controller (EV_LIGHT_ACK) or refuses (EV_LIGHT_ERR).
// This is enough for end-to-end smoke testing;
// M2 will add per-light logging and richer error paths.
#include "common.h"
#include "safety.h"
#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static volatile sig_atomic_t we=0;
static void ot(int s){ (void)s; we=1; }
int main(int argc, char *argv[]){
	if(argc<5) return 1;
	int shmid=atoi(argv[1]), semid=atoi(argv[2]), qid=atoi(argv[3]), dir=atoi(argv[4]);
	if(dir<0||dir>=N_LIGHTS) return 1;
	IntersectionState *st=ipc_attach_shm(shmid); if(!st) return 1;
	signal(SIGTERM,ot); signal(SIGINT,ot);
	EventMsg m;
	while(!we && !st->shutdown_requested){
		if(ipc_recv(qid,&m,MQ_LIGHT(dir))==-1) break;
		if(m.event_kind!=EV_CMD_SET_COLOR) continue;
		LightColor next=(LightColor)m.arg;
		ipc_sem_wait(semid,SEM_STATE_MUTEX);
		LightColor prev=st->light[dir];
		int ok=safety_legal_transition(prev,next);
		if(ok) st->light[dir]=next;
		ipc_sem_post(semid,SEM_STATE_MUTEX);
		EventMsg r; memset(&r,0,sizeof r);
		r.mtype=MQ_CONTROLLER; r.event_kind=ok?EV_LIGHT_ACK:EV_LIGHT_ERR;
		r.direction=dir; r.arg=next;
		ipc_send(qid,&r);
	}
	ipc_detach_shm(st);
	return 0;
}
