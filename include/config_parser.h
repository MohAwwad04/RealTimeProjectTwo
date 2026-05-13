// Defines the Config struct M3 will populate.
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H
typedef struct {
	int min_green_duration, yellow_duration, all_red_duration;
	int ped_cross_duration, emergency_hold;
	int max_vehicle_wait,   max_ped_wait, emergency_response;
	int arrival_rate_n, arrival_rate_s, arrival_rate_e, arrival_rate_w;
	char log_file[128]; int log_to_stdout;
} Config;
int config_parser_load(const char *path, void *out);
#endif
