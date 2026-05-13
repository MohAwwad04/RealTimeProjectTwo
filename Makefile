CC	= gcc
CFLAGS	= -Wall -Wextra -g -O0 -Iinclude
COMMON	= src/ipc_utils.o src/safety.o
BINS	= traffic_system controller light vehicle pedestrian emergency logger gui

.PHONY:	all clean ipcclean
all:	$(BINS)

traffic_system:
	src/main.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
controller:
	src/controller.o src/config_parser.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
light:
	src/light.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
vehicle:
	src/vehicle.o src/config_parser.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
pedestrian:
	src/pedestrian.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
emergency:
	src/emergency.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
logger:
	src/logger.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^
gui:
	src/gui.o $(COMMON)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o:
	src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(BINS) traffic.log
ipcclean:
	bash tools/ipc_cleanup.sh
