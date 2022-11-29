#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#define BLUE_COLOR  0xFFFF0000
#define RED_COLOR   0xFF0000FF
#define GREEN_COLOR 0xFF00FF00
#define WHITE_COLOR 0xFFFFFFFF
#define BLACK_COLOR 0x00000000

#define	LAVENDER_INDIGO  0xFFE05F9B
#define	BATTERY_BLUE     0xFFD8A416
#define	SKY_BLUE         0xFFE8DB60
#define	KIWI             0xFF46D38B
#define	MINION_YELLOW    0xFF48DFEF
#define	DEEP_SAFFRON     0xFF2CA5F9
#define	SINOPIA          0xFF124ED6
#define DARK_RED         0xFF00008B
#define MIDNIGHT_BLUE    0xFF701919
#define LIGHT_SEE_GREEN  0xFFAAB220	
#define DARK_OLIVE_GREEN 0xFF2F6B55 	
#define CORAL            0xFF507FFF

#define WIDTH  800
#define HEIGHT 600
#define BACKGROUND_COLOR WHITE_COLOR
#define OUTPUT_FILE_PATH "output.ppm"

#define SEED_COUNT 15
#define SEED_MARK_RADIUS 3
#define SEED_MARK_COLOR  BLACK_COLOR

typedef uint32_t Color32;
typedef struct {
    int x, y;
} Point;

static Color32 image[HEIGHT][WIDTH];
static Point   seeds[SEED_COUNT];

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
#define palette_count (sizeof(palette)/sizeof(palette[0]))

int sqr_dist(int x0, int y0, int x1, int y1) {
    int dx = x0 - x1;
    int dy = y0 - y1;
    return dx*dx + dy*dy;
}

void save_image_as_ppm(const char* file_path) {
    FILE *f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "[ERROR]: Could not open '%s': %s\n", file_path, strerror(errno));
        exit(1);
    } 
    fprintf(f, "P6\n%d %d 255\n", WIDTH, HEIGHT);

    for(size_t y = 0; y < HEIGHT; y++) {
        for(size_t x = 0; x < WIDTH; x++) {
            uint32_t pixel = image[y][x];
            // 0xAABBGGRR
            uint8_t bytes[3] = {
                (pixel&0x0000FF)>>8*0,
                (pixel&0x00FF00)>>8*1,
                (pixel&0xFF0000)>>8*2,
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

void fill_circle(int cx, int cy, int radius, Color32 color) {
    int x0 = cx - radius;
    int x1 = cx + radius;
    int y0 = cy - radius;
    int y1 = cy + radius;

    for (int x = x0; x <= x1 && x >= 0 && x < WIDTH; ++x) {
        for (int y = y0; y <= y1 && y >= 0 && y < HEIGHT; ++y) {
            if (sqr_dist(cx, cy, x, y) <= radius * radius) {
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
        fill_circle(seeds[i].x, seeds[i].y, SEED_MARK_RADIUS, SEED_MARK_COLOR);
    }
}

void render_voronoi(void) {
    for (size_t y = 0; y < HEIGHT; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            int j = 0; // closest seed idx 
            for (size_t i = 1; i < SEED_COUNT; i++) {
                if (sqr_dist(seeds[i].x, seeds[i].y, x, y) < sqr_dist(seeds[j].x, seeds[j].y, x, y)) {
                    j = i;
                }
                image[y][x] = palette[j%palette_count];
            }
        }
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