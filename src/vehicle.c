// M3 stub
// Round-robins one arrival per direction every 2 seconds.
// This is only so M1 can demo the FSM cycling.
// M3 will replace it with Poisson arrivals driven by ARRIVAL_RATE_N/S/E/W from the config.
#include "common.h"
#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t we=0;

static void ot(int s){
	(void)s;
	we=1;
}

int main(int argc, char *argv[]){
	if(argc<5) return 1;
	int shmid=atoi(argv[1]), qid=atoi(argv[3]);
	IntersectionState *st=ipc_attach_shm(shmid);
	if(!st) return 1;

	signal(SIGTERM,ot);
	signal(SIGINT,ot);

	int dir=0;
	while(!we && !st->shutdown_requested){
		EventMsg m; memset(&m,0,sizeof m);
		m.mtype=MQ_CONTROLLER;
		m.event_kind=EV_VEHICLE_ARRIVE;

		m.direction=dir;
		m.arg=1;
		m.timestamp=time(NULL);

		ipc_send(qid,&m);
		dir=(dir+1)%N_LIGHTS;
		sleep(2);
	}
	ipc_detach_shm(st);
	return 0;
}
