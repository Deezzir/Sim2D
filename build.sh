#!/bin/sh

set -xe
gcc -Wall -Wextra -o voronoi_ppm.out    main_ppm.c
gcc -Wall -Wextra -o voronoi_opengl.out main_opengl.c -O3 -lglfw -lGL -lm