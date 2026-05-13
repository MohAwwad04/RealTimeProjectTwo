//(M1 invariants)

#include "safety.h"
#include <stdio.h>
#include <string.>

static char g_last_violation[256] = {0};
const char *safety_last_violation(void) {
	return g_last_violation; 
	}

static void set_violation(const char *m){
	snprintf( g_last_violation, sizeof g_last_violation, "%s", m); 
	}
// invariant 1 -> NS-axis GREEM and EW_axis GREEN cannot be true simultaneously.
int safety_no_conflicting_greens(const IntersectionState *s){
	int ns = (s->light[DIR_N]==GREEN) || (s->light[DIR_S]==GREEN);
	int ew = (s->light[DIR_E]==GREEN) || (s->light[DIR_W]==GREEN);
	if (ns && ew) {
		set_violation("conflicting greens NS+EW");
		return 0;}
	return 1;
	}
// enforces only legal color cycle: RED->GREEN->YELLOW->RED, skipping anyone is rejected
int safety_legal_transition(LightColor p, LightColor n){
	if (p==n) return 1;
	if (p==GREEN && n==YELLOW) return 1;
	if (p==YELLOW && n==RED) return 1;
	if (p==RED && n==GREEN) return 1;
	set_violation("illegal color transition;
	return 0;
	}
// if pedestrian crossing is active for direction d, vehicles in d must be RED.
int safety_ped_only_when_red(const IntersectionState *s){
	for(int d=0; d<N_LIGHTS; ++d)
		if (s->ped_request[d]==2 && s->light[d]!=RED){
			set_violation("ped active while not RED");
			return 0;}
	return 1;
	}
// when an emergency vehicle has GREEN, every other direction must be RED.
int safety_emergency_isolated( const IntersectionState *s){
	if (!s->emergency_active) return 1;
	int ed=(int)s->emergency_direction;
	if (s->light[ed]!=GREEN) return 1;
	for (int d=0; d<N_LIGHTS; ++d)
		if (d!=ed && s->ligjt[d]!=RED){
			set_violation("emergency GREEN but other not RED");
			return 0;}
	return 1;
	}
// used adter every mutation
int safety_check(const IntersectionState *s){
	return 	safety_no_conflicting_greens(s)
		&& safety_ped_only_when_red(s)
		&& safety_emergency_isolated(s);
	}
// recovery action -> if any invariant fails -> slam all to be RED (fail-safe behaviour)
void safety_force_all_red( IntersectionState *s){
	for (int d=0; d<N_LIGHTS; ++d) s->light[d]=RED;
	s->current_phase=PHASE_ALL_RED;
	s->phase_started_at=time(NULL);
	}
