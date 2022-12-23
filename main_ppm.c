#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// General colors (0xAABBGGRR)
#define BLUE_COLOR 			0xFFFF0000
#define RED_COLOR 			0xFF0000FF
#define GREEN_COLOR 		0xFF00FF00
#define WHITE_COLOR 		0xFFFFFFFF
#define BLACK_COLOR 		0x00000000

// Palette colors (0xAABBGGRR)
#define LAVENDER_INDIGO 	0xFFE05F9B
#define BATTERY_BLUE 		0xFFD8A416
#define SKY_BLUE 			0xFFE8DB60
#define KIWI 				0xFF46D38B
#define MINION_YELLOW 		0xFF48DFEF
#define DEEP_SAFFRON 		0xFF2CA5F9
#define SINOPIA 			0xFF124ED6
#define DARK_RED 			0xFF00008B
#define MIDNIGHT_BLUE 		0xFF701919
#define LIGHT_SEE_GREEN 	0xFFAAB220
#define DARK_OLIVE_GREEN	0xFF2F6B55
#define CORAL				0xFF507FFF

// Image properties
#define WIDTH  2560
#define HEIGHT 1080
#define BACKGROUND_COLOR WHITE_COLOR
#define OUTPUT_FILE_PATH "output.ppm"

// Voronoi properties
#define SEED_COUNT 		 	15
#define SEED_MARK_RADIUS	3
#define SEED_MARK_COLOR BLACK_COLOR

typedef uint32_t Color32;
typedef struct {
	uint16_t x, y;
} Point32;

static Color32 	image[HEIGHT][WIDTH];
static Point32	seeds[SEED_COUNT];
static int 		depth[HEIGHT][WIDTH];

static Color32 palette[] = {
	BLUE_COLOR,
	RED_COLOR,
	GREEN_COLOR,
	LAVENDER_INDIGO,
	BATTERY_BLUE,
	SKY_BLUE,
	KIWI,
	MINION_YELLOW,
	DEEP_SAFFRON,
	SINOPIA,
	DARK_RED,
	MIDNIGHT_BLUE,
	LIGHT_SEE_GREEN,
	DARK_OLIVE_GREEN,
	CORAL
};
#define PALLETE_COUNT (sizeof(palette) / sizeof(palette[0]))

int sqr_dist(int x0, int y0, int x1, int y1) {
	int dx = x0 - x1;
	int dy = y0 - y1;
	return dx * dx + dy * dy;
}

void save_image_as_ppm(const char* file_path) {
	FILE* f = fopen(file_path, "wb");
	if (f == NULL) {
		fprintf(stderr, "[ERROR]: Could not open '%s': %s\n", file_path, strerror(errno));
		exit(1);
	}
	fprintf(f, "P6\n%d %d 255\n", WIDTH, HEIGHT);

	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			uint32_t pixel = image[y][x];
			// 0xAABBGGRR
			uint8_t bytes[3] = {
				(pixel & 0x0000FF) >> 8 * 0,
				(pixel & 0x00FF00) >> 8 * 1,
				(pixel & 0xFF0000) >> 8 * 2,
			};
			fwrite(bytes, sizeof(bytes), 1, f);
			assert(!ferror(f));
		}
	}

	int ret = fclose(f);
	assert(ret == 0);
}

void fill_image(Color32 color) {
    for(size_t y = 0; y < HEIGHT; y++) {
        for(size_t x = 0; x < WIDTH; x++) {
            image[y][x] = color;
        }
    }
}

void fill_circle(Point32 p, int radius, Color32 color) {
    int x0 = p.x - radius;
    int x1 = p.x + radius;
    int y0 = p.y - radius;
    int y1 = p.y + radius;

	for (int x = x0; x <= x1 && x >= 0 && x < WIDTH; ++x) {
		for (int y = y0; y <= y1 && y >= 0 && y < HEIGHT; ++y) {
			if (sqr_dist(p.x, p.y, x, y) <= radius * radius) {
				image[y][x] = color;
			}
		}
	}
}

void generate_rand_seeds(void) {
	for (size_t i = 0; i < SEED_COUNT; i++) {
		seeds[i].x = rand() % WIDTH;
		seeds[i].y = rand() % HEIGHT;
	}
}

void render_seed_marks(void) {
	for (size_t i = 0; i < SEED_COUNT; i++) {
		fill_circle(seeds[i], SEED_MARK_RADIUS, SEED_MARK_COLOR);
	}
}

Color32 point_to_color(Point32 p) {
	assert(p.x >= 0 && p.x < UINT16_MAX);
	assert(p.y >= 0 && p.y < UINT16_MAX);

	return ((uint16_t) p.y << (8 * 2)) | (uint16_t) p.x;
}

Point32 color_to_point(Color32 c) {
	return (Point32) {
		.x =  (c & 0x0000FFFF) >> (8 * 0),
		.y =  (c & 0xFFFF0000) >> (8 * 2)
	};
}

void apply_next_seed(size_t seed_idx) {
	Point32 next_seed = seeds[seed_idx];
	Color32 color = palette[seed_idx % PALLETE_COUNT];

	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			int d = sqr_dist(next_seed.x, next_seed.y, x, y);
			if (d < depth[y][x]) {
				depth[y][x] = d;
				image[y][x] = color;  
			}
		}
	}
}

void render_voronoi(void) {
	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			depth[y][x] = INT_MAX;
		}
	}

	for (size_t i = 0; i < SEED_COUNT; i++) {
		apply_next_seed(i);
	}
}

int main(void) {
	srand(time(0));

	generate_rand_seeds();
	fill_image(BACKGROUND_COLOR);

	render_voronoi();
	render_seed_marks();

	save_image_as_ppm(OUTPUT_FILE_PATH);

	return 0;
}