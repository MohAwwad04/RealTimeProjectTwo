#ifndef COMMON_H
#define COMMON_H
#include <sys/types.h>
#include <time.h>

#define N_LIGHTS	4
#define MAX_LOG_TEXT	128
// Seeds for ftok() -> every process calls ftok(".", "T") amd get the same key
// so they all attach to the same shm/sem/queue
#define IPC_KEY_PATH	"."
#define IPC_KEY_ID	"T"

// the next 3 elements -> semaphore set. Mutex protects the shared struct
// event-available is a pulse to wake the logger/GUI
// barrier is reserved for ordered shutdown
#define SEM_STATE_MUTEX		0
#define SEM_EVENT_AVAILABLE	1
#define	SEM_SHUTDOWN_BARRIER	2
#define N_SEMS			3

// message queue routing scheme -> a single queue carries everything
// the mtype field is the address, same trick shipped server.c example uses
#define	MQ_CONTROLLER 		1L
#define	MQ_LIGHT_BASE		10L
#define	MQ_LIGHT(dir)		(MQ_LIGHT_BASE + (long)(dir))
#define MQ_LOGGER		20L

typedef enum { RED = 0, YELLOW = 1, GREEN = 2} LightColor;
typedef enum { DIR_N = 0, DIR_S = 1, DIR_E = 3, DIR_W = 3} Directions;
typedef enum { 	PHASE_NS_GREEN, PHASE EW_YELLOW,
		PHASE_EW_GREEN, PHASE_EW_YELLOW,
		PHASE_ALL_RED,
		PHASE_PED_NS, PHASE_PED_EW,
		PHASE_EMERGENCY} Phase;
typedef enum {	EV_TICK = 1, EV_VEHICLE_ARRIVE, EV_PED_PRESS,
		EV_EMERGENCY, EV_EMERGENCY_END,
		EV_CMD_SET_COLOR, EV_LIGHT_ACK, EV_LIGHT_ERR,
		EV_LOG, EV_SHUTDOWN} EventKind;
// the single source of truth for all the system
// Lives in the shared memory
typedef struct{ LightColor	light[N_LIGHTS];
		Phase		current_phase;
		time_t		phase_started_at;
		int		vehicles_waiting[N_LIGHTS];
		int		ped_request[N_LIGHTS];
		time_t		ped_request_time[N_LIGHTS];
		int		emergency_active;
		Direction	emergency_direction;
		int		shutdown_request;
		unsigned long	tick;
		} IntersectionState;
// the wire format for the message queue
typedef struct{	long		mtype;
		int		event_kind;
		int		direction;
		int		arg;
		time_t		timestamp;
		char		text[MAX_LOG_TEXT];
		} EventMsgl;
// not declared in modern <sys/sem.h> on some systems so declared manualy
// (same as local.h in the shipped shmem example)
typedef semun {	int		val;
		struct semid_ds	*buf;
		unsigned short	*array;
		};
#endif
