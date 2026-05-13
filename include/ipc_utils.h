#ifndef IPC_UTILS_H
#define IPC_UTILS_H
#include "common.h"
// ipc_attach_shm/detach_shm → shmat/shmdt with NULL check.
IntersectionState *ipc_attach_shm(int shmid);
void ipc_detach_shm(IntersectionState *p);
// ipc_sem_wait/post → SEM_UNDO-protected ops with EINTR retry.
int ipc_sem_wait(int semid, int idx);
int ipc_sem_post(int semid, int idx);
// ipc_send/recv → msgsnd/msgrcv with EINTR + EAGAIN retries.
int ipc_send(int qid, const EventMsg *m);
int ipc_recv(int qid, EventMsg *m, long mtype);
#endif
