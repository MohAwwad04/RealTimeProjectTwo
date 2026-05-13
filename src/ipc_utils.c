// (stub for M2, fully working)
#include "ipc_utils.h"
#include <errno.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>

IntersectionState *ipc_attach_shm(int shmid){
	void *p=shmat(shmid,NULL,0); if(p==(void*)-1) return NULL;
		return (IntersectionState*)p;
}

void ipc_detach_shm(IntersectionState *p){
	if(p) shmdt(p);
}

int ipc_sem_wait(int s,int i){
	struct sembuf op={(unsigned short)i,-1,SEM_UNDO};
	while(semop(s,&op,1)==-1){
		if(errno==EINTR) continue;
		return -1;
	}
	return 0;
}

int ipc_sem_post(int s,int i){
	struct sembuf op={(unsigned short)i,+1,SEM_UNDO};
	while(semop(s,&op,1)==-1){
		if(errno==EINTR) continue;
		return -1;
	}
      return 0;
}

int ipc_send(int q,const EventMsg *m){
	size_t sz=sizeof *m-sizeof(long);
	while(msgsnd(q,m,sz,0)==-1){
		if(errno==EINTR) continue;
		if(errno==EAGAIN){
			usleep(1000);
			continue;
		}
		return -1;
	}
	return 0;
}

int ipc_recv(int q,EventMsg *m,long mt){
	size_t sz=sizeof *m-sizeof(long);
	while(msgrcv(q,m,sz,mt,0)==-1){
		if(errno==EINTR) continue;
		return -1;
	}
	return 0;
}
