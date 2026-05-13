// M3 stub
// intentionally a no-op so the link works
// when M3 implements real mmap parsing, the strong symbol overrides the controller's weak default
#include "config_parser.h"
#include <stdio.h>
int config_parser_load(const char *p, void *o){
	(void)p; (void)o;
	fprintf(stderr,"[config] STUB defaults\n");
	return 0;
}
