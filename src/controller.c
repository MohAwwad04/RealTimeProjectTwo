// M1 FSM

#include "common.h"
#include "safety.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/time.h>

typedef struct {
	int min_green_duration, yellow_duration, all_red_duration;
	int ped_cross_duration, emergency_hold;
	int max_vehicle_wait,   max_ped_wait, emergency_response;
	} ControllerCfg;

static ControllerCfg cfg = {8,3,2,10,12,60,30,6};

__attribute__((weak))
int config_parser_load(const char *p, ControllerCfg *o){
	(void)p;
	(void)o;
	return 0;}

static int shmid,semid,qid;
static IntersectionState *st;
static volatile sig_atomic_t want_shutdown=0;

static int sem_op(int i,int d){
	struct sembuf op={(unsigned short)i,(short)d,SEM_UNDO};
	while(semop(semid,&op,1)==-1){
		if(errno==EINTR) continue;
	return -1;
	}
	return 0;
}

#define LOCK()   sem_op(SEM_STATE_MUTEX,-1)
#define UNLOCK() sem_op(SEM_STATE_MUTEX,+1)
#define POST()   sem_op(SEM_EVENT_AVAILABLE,+1)

static void send_msg(long mt,EventKind k,int dir,int arg,const char *tx){
	EventMsg m;
	memset(&m,0,sizeof m);

	m.mtype=mt;
	m.event_kind=k;
	m.direction=dir;
	m.arg=arg;

	m.timestamp=time(NULL);
	if(tx) snprintf(m.text,sizeof m.text,"%s",tx);
	while(msgsnd(qid,&m,sizeof m-sizeof(long),0)==-1){
		if(errno==EINTR) continue;
		if(errno==EAGAIN){
			usleep(1000);
			continue;
		}
		return;
	}
}
static void log_line(const char *t){
	send_msg(MQ_LOGGER,EV_LOG,-1,0,t);
}
static void cmd_light(Direction d, LightColor c){
	send_msg(MQ_LIGHT(d), EV_CMD_SET_COLOR, d, c, NULL);
}

static void on_alarm(int s){
	(void)s;
	EventMsg m;
	memset(&m,0,sizeof m);

	m.mtype=MQ_CONTROLLER; m.event_kind=EV_TICK; m.direction=-1;
	m.timestamp=time(NULL);
	(void)msgsnd(qid,&m,sizeof m-sizeof(long),IPC_NOWAIT);
}

static void install_timer(void){
	struct sigaction sa; memset(&sa,0,sizeof sa);
	sa.sa_handler=on_alarm; sa.sa_flags=SA_RESTART;
	sigaction(SIGALRM,&sa,NULL);

	struct itimerval it;
	it.it_value.tv_sec=0;
	it.it_value.tv_usec=100*1000;
	it.it_interval=it.it_value;

	setitimer(ITIMER_REAL,&it,NULL);
}
static void on_term(int s){
	(void)s;
	want_shutdown=1;
}

static void set_phase(Phase p){
	st->current_phase=p;
	st->phase_started_at=time(NULL);
}

static int phase_elapsed(void){
return (int)(time(NULL)-st->phase_started_at);
}

static int ped_ns(void){
	return st->ped_request[DIR_N]||st->ped_request[DIR_S];
}
static int ped_ew(void){
	return st->ped_request[DIR_E]||st->ped_request[DIR_W];
}

static void drive(Direction d, LightColor c){
	if(st->light[d]==c) return;
	if(!safety_legal_transition(st->light[d],c)){
	log_line(safety_last_violation());
	return;}

	st->light[d]=c;
	cmd_light(d,c);
}

static void enter_ns_green(void){
	drive(DIR_E,RED);
	drive(DIR_W,RED);

	drive(DIR_N,GREEN);
	drive(DIR_S,GREEN);

	set_phase(PHASE_NS_GREEN);
	log_line("phase=NS_GREEN");
}
static void enter_ns_yellow(void){
	drive(DIR_N,YELLOW);
	drive(DIR_S,YELLOW);

	set_phase(PHASE_NS_YELLOW);
	log_line("phase=NS_YELLOW");
}
static void enter_ew_green(void){
	drive(DIR_N,RED);
	drive(DIR_S,RED);

	drive(DIR_E,GREEN);
	drive(DIR_W,GREEN);

	set_phase(PHASE_EW_GREEN);
	log_line("phase=EW_GREEN");
}
static void enter_ew_yellow(void){
	drive(DIR_E,YELLOW);
	drive(DIR_W,YELLOW);

	set_phase(PHASE_EW_YELLOW);
	log_line("phase=EW_YELLOW");
}
static void enter_all_red(void){
	for(int d=0;d<N_LIGHTS;++d) drive((Direction)d,RED);

	set_phase(PHASE_ALL_RED);
	log_line("phase=ALL_RED");
}
static void enter_ped(Phase p){
	for(int d=0;d<N_LIGHTS;++d) drive((Direction)d,RED);
	if(p==PHASE_PED_NS){
		if(st->ped_request[DIR_N]) st->ped_request[DIR_N]=2;
		if(st->ped_request[DIR_S]) st->ped_request[DIR_S]=2;
	} else {
		if(st->ped_request[DIR_E]) st->ped_request[DIR_E]=2;
		if(st->ped_request[DIR_W]) st->ped_request[DIR_W]=2;
	}
	set_phase(p);
	log_line(p==PHASE_PED_NS?"phase=PED_NS":"phase=PED_EW");
}
static void enter_emergency(Direction ed){
	for(int d=0;d<N_LIGHTS;++d)
		if(d!=ed) drive((Direction)d,RED);

	drive(ed,GREEN);

	set_phase(PHASE_EMERGENCY);
	log_line("phase=EMERGENCY");
}
static void fsm_step(void){
	if(st->emergency_active){
		if(st->current_phase!=PHASE_EMERGENCY){
			if(st->current_phase==PHASE_NS_GREEN){
				enter_ns_yellow();
				return;}
			if(st->current_phase==PHASE_EW_GREEN){
				enter_ew_yellow();
				return;}
			if(st->current_phase==PHASE_NS_YELLOW && phase_elapsed()>=cfg.yellow_duration){
				enter_all_red();
				return;}
			if(st->current_phase==PHASE_EW_YELLOW && phase_elapsed()>=cfg.yellow_duration){
				enter_all_red();
				return;}
			if(st->current_phase==PHASE_ALL_RED){
				enter_emergency(st->emergency_direction);
				return;}
			if(st->current_phase==PHASE_PED_NS||st->current_phase==PHASE_PED_EW){
				enter_all_red();
				return;}
		}
		return;
	}
	if(ped_ns() && st->current_phase==PHASE_ALL_RED && phase_elapsed()>=cfg.all_red_duration){
		enter_ped(PHASE_PED_NS);
		return;}
	if(ped_ew() && st->current_phase==PHASE_ALL_RED && phase_elapsed()>=cfg.all_red_duration){
		enter_ped(PHASE_PED_EW);
		return;}
	switch(st->current_phase){
		case PHASE_ALL_RED:
			if(phase_elapsed()>=cfg.all_red_duration){
				int nw=st->vehicles_waiting[DIR_N]+st->vehicles_waiting[DIR_S];
				int ew=st->vehicles_waiting[DIR_E]+st->vehicles_waiting[DIR_W];
				if(ew>nw) enter_ew_green();
				else enter_ns_green();}
			break;
		case PHASE_NS_GREEN:
			if(phase_elapsed()>=cfg.min_green_duration && (st->vehicles_waiting[DIR_E]+st->vehicles_waiting[DIR_W]>0 || ped_ew()))
				enter_ns_yellow();
			break;
		case PHASE_NS_YELLOW:
			if(phase_elapsed()>=cfg.yellow_duration)
				enter_all_red();
			break;
		case PHASE_EW_GREEN:
			if(phase_elapsed()>=cfg.min_green_duration && (st->vehicles_waiting[DIR_N]+st->vehicles_waiting[DIR_S]>0 || ped_ns()))
				enter_ew_yellow();
			break;
		case PHASE_EW_YELLOW:
			if(phase_elapsed()>=cfg.yellow_duration)
				enter_all_red();
			break;
		case PHASE_PED_NS:
		case PHASE_PED_EW:
			if(phase_elapsed()>=cfg.ped_cross_duration){
				if(st->current_phase==PHASE_PED_NS){
					st->ped_request[DIR_N]=st->ped_request[DIR_S]=0;
				}else{
					st->ped_request[DIR_E]=st->ped_request[DIR_W]=0;
				}
				enter_all_red();
			}
			break;
		case PHASE_EMERGENCY:
			break;
	}
}

static void handle_event(const EventMsg *m){
	switch(m->event_kind){
		case EV_VEHICLE_ARRIVE:
			if(m->direction>=0&&m->direction<N_LIGHTS) st->vehicles_waiting[m->direction]++;
			break;
		case EV_PED_PRESS:
			if(m->direction>=0&&m->direction<N_LIGHTS && st->ped_request[m->direction]==0){
				st->ped_request[m->direction]=1;
				st->ped_request_time[m->direction]=time(NULL);
			}
			break;
		case EV_EMERGENCY:
			if(!st->emergency_active && m->direction>=0 && m->direction<N_LIGHTS){
				st->emergency_active=1;
				st->emergency_direction=(Direction)m->direction;
				log_line("EV_EMERGENCY received");
			}
			break;
		case EV_EMERGENCY_END:
			st->emergency_active=0;
			log_line("EV_EMERGENCY_END");
			enter_all_red();
			break;
		case EV_LIGHT_ERR:
			log_line("EV_LIGHT_ERR");
			safety_force_all_red(st);
			break;
		case EV_SHUTDOWN:
			want_shutdown=1;
			break;
		default: break;
	}
}

int main(int argc, char *argv[]){
	if(argc<5){
		fprintf(stderr,"controller usage\n");
		return 1;}

	shmid=atoi(argv[1]);
	semid=atoi(argv[2]);
	qid=atoi(argv[3]);

	st=(IntersectionState*)shmat(shmid,NULL,0);
	if(st==(void*)-1){
		perror("shmat ctrl");
		return 1;}
	(void)config_parser_load(argv[4], &cfg);

	struct sigaction sa;
	memset(&sa,0,sizeof sa);
	sa.sa_handler=on_term;

	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);

	install_timer();
	log_line("controller online");
	EventMsg m;
	while(!want_shutdown && !st->shutdown_requested){
		ssize_t r=msgrcv(qid,&m,sizeof m-sizeof(long),MQ_CONTROLLER,0);
		if(r==-1){
			if(errno==EINTR) continue;
			if(errno==EIDRM) break;
			break;}
		if(LOCK()==-1) break;

		handle_event(&m);
		fsm_step();
		if(!safety_check(st)){
			log_line(safety_last_violation());
			safety_force_all_red(st);}
		st->tick++;

		UNLOCK();
		POST();
		}

	log_line("controller shutting down");
	shmdt(st);
	return 0;
}
