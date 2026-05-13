// Drains MQ_LOGGER and writes timestamped lines to traffic.log and stdout.
// That alone lets you watch the FSM in real time.
// M4 will add TIMING_VIOLATION detection by comparing phase_started_at against configured durations.
#include "common.h"
#include "ipc_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

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

	FILE *f=fopen("traffic.log","a");
	if(!f) return 1;

	EventMsg m;
	while(!we && !st->shutdown_requested){
		if(ipc_recv(qid,&m,MQ_LOGGER)==-1) break;
		time_t t=m.timestamp?m.timestamp:time(NULL);
		struct tm tmv;
		localtime_r(&t,&tmv);

		char ts[32]; strftime(ts,sizeof ts,"%Y-%m-%d %H:%M:%S",&tmv);
		fprintf(f, "[%s] kind=%d dir=%d arg=%d %s\n",ts,m.event_kind,m.direction,m.arg,m.text);
		fprintf(stdout,"[%s] kind=%d dir=%d arg=%d %s\n",ts,m.event_kind,m.direction,m.arg,m.text);

		fflush(f);
		fflush(stdout);
	}
	fclose(f);
	ipc_detach_shm(st);
	return 0;
}
