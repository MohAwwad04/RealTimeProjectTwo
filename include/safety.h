// the safety predicate
// make sure the system never allow conflicting greens, skipped yellows,
// ped-vs-vehicle conflicts.
// puttigng them in one header lets both M1's FSM and M2's light processes
// call the same predicates - defense-in-depth.
#ifndef SAFETY.H
#define SAFETY.H
#include "common.h"

int safety_check			(const IntersectionState *s);
int safety_no_conflicting_greens	(const IntersectionState *s);
int safety_legal_transition		(LightColor prev, LightColor next);
int sefety_ped_only_when_red		(const IntersectionState *s);
int safety_emergency_isolated		(const IntersectionState *s);
const char *safety_last_violation(void);
void safety_force_all_red		(IntersecitonState *s);
#endif
