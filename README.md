# Sim2D

- `voronoi` : The simple C program that will create a Voronoi Diagram image in PPM that will be saved in the root dir. Just a starting point.

- `sim` : The C program that will create some simulations with physics in OpenGL.
  - `Mode 1` : Dynamic Voronoi Diagram
  - `Mode 2` : Atoms - just like the `Mode 1` but with a different graphical representation
  - `Mode 3` : Bubbles - a simulation of bubbles with inelastic collisions and gravity
  
## Quick Start

```console
$ make all
gcc -Wall -Wextra -Iinclude -O2 src/voronoi_ppm.c -o voronoi 
gcc -Wall -Wextra -Iinclude -O2 src/helpers.c src/sim.c src/glextloader.c src/opengl.c src/main.c -o sim -lglfw -lGL -lm

./voronoi & ./sim 
```

### Optional Arguments

```console
usage: sim [-m num] [-c num] [-r num]
       Optionally specify simulation mode: [-m] (1-3). By default Mode 1 is chosen
              Mode 1: - 'Voronoi'
              Mode 2: - 'Atoms'
              Mode 3: - 'Bubbles'
       Optionally specify seed count:      [-c] (1-500)
       Optionally specify seed radius:     [-r] (5-150). Only works with 'voronoi' and 'atoms' modes
```

Sources:

- Elastic collision: [Wiki Page](https://en.wikipedia.org/wiki/Elastic_collision)
- Voronoi Diagram: [Wiki Page](https://en.wikipedia.org/wiki/Voronoi_diagram)

## Voronoi Sim Example with 20 Seeds

![img](assets/voronoi.gif)
