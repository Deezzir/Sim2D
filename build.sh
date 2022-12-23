#!/bin/sh

set -xe
gcc -Wall -Wextra -o voronoi_ppm.out    src/main_ppm.c
gcc -Wall -Wextra -o voronoi_opengl.out src/main_opengl.c -O3 -lglfw -lGL -lm