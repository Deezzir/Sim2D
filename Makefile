CC=gcc
CFLAGS=-Wall -Wextra -Iinclude

ifeq ($(DEBUG),1)
    CFLAGS+=-O0 -g -DDEBUG
else
    CFLAGS+=-O2
endif

VORONOI_PPM_FILE=src/voronoi_ppm.c

MAIN_FILE=src/main.c
SIM_FILE=src/sim.c
GLEXTLOADER_FILE=src/glextloader.c
OPENGL_FILE=src/opengl.c
HELPERS_FILE=src/helpers.c
HEADERS=include/*.h

sim: $(HELPERS_FILE) $(SIM_FILE) $(GLEXTLOADER_FILE) $(OPENGL_FILE) $(MAIN_FILE) $(HEADERS)
	$(CC) $(CFLAGS) $^ -o $@ -lglfw -lGL -lm

voronoi: $(VORONOI_PPM_FILE)
	$(CC) $(CFLAGS) $^ -o $@ 

clean:
	rm -f *.o voronoi sim

all: voronoi sim