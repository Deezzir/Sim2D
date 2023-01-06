CC=gcc
CFLAGS=-I -Wall -Wextra

ifeq ($(DEBUG),1)
    CFLAGS += -O0 -g -DDEBUG
else
    CFLAGS += -O2
endif

VORONOI_PPM_FILE=src/voronoi_ppm.c
MAIN_OPENGL_FILE=src/sim.c
HELPERS_FILE=src/helpers.c

sim: $(MAIN_OPENGL_FILE) $(HELPERS_FILE)
	$(CC) $(CFLAGS) $^ -o $@ -lglfw -lGL -lm

voronoi: $(VORONOI_PPM_FILE)
	$(CC) $(CFLAGS) $^ -o $@ 

clean:
	rm -f *.o voronoi sim

all: voronoi sim