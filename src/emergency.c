// Idle loop. Just enough so main.c's fork() succeeds and the child stays alive until shutdown.
// M3 will add raw-mode keyboard reading and random/manual event generation.
#include "common.h"
#include "ipc_utils.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t we=0;

static void ot(int s){
	(void)s;
	we=1;
}

int main(int argc, char *argv[]){
	if(argc<5) return 1;
	IntersectionState *st=ipc_attach_shm(atoi(argv[1])); if(!st) return 1;
	signal(SIGTERM,ot); signal(SIGINT,ot);
	while(!we && !st->shutdown_requested) sleep(1);
	ipc_detach_shm(st);
	return 0;
}
