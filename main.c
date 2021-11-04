#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define WORKERS 16

typedef struct grid {
    float** val;
    unsigned int width, height;
} Grid;

typedef struct ThreadArguements {
    unsigned int start, end;
    Grid *g0, *g1;
} ThreadArguements;

void generateGrid(Grid* g, int init) {
    unsigned int width = g->width;
    unsigned int height = g->height;
    g->val = (float**)malloc(width * sizeof(float*));
    if (g->val == NULL) {
        printf("Cannot allocate memory for new grid");
        exit(-1);
        return;
    }

    for (unsigned int i = 0; i < width; i++) {
        g->val[i] = (float*)malloc(height * sizeof(float));
        if (g->val[i] == NULL) {
            printf("Cannot allocate memory for new row of grid");
            exit(-1);
            return;
        }
        for (unsigned int j = 0; j < height; j++) {
            if (init == TRUE) {
                if (i == 0 || j == 0) {
                    g->val[i][j] = 1;
                } else if (i == width - 1 || j == height - 1) {
                    g->val[i][j] = 0;
                } else {
                    g->val[i][j] =
                        (float)(rand() / 10) / (float)(RAND_MAX / 10);
                }
            } else {
                g->val[i][j] = 0;
            }
        }
    }
}

void simThread(unsigned int start, unsigned int end, Grid* grid0, Grid* grid1,
               double* diff) {
    unsigned int i, x, y;
    float newVal;
    int width = grid0->width;
    int height = grid0->height;
    for (i = start; i < end; i++) {
        x = i % width;
        y = i / width;
        if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
            grid1->val[x][y] = grid0->val[x][y];
        } else {
            newVal = (grid0->val[x][y] + grid0->val[x - 1][y - 1] +
                      grid0->val[x + 1][y - 1] + grid0->val[x - 1][y + 1] +
                      grid0->val[x + 1][y + 1]) /
                     5;
            *diff += fabs((double)newVal - (double)grid0->val[x][y]);
            grid1->val[x][y] = newVal;
        }
    }
}

void threadFunc(ThreadArguements* args) {
    // Store all information

    // Do iteration

    // Wait for other threads
    //Swap grid pointers
    temp = grid1;
    grid1 = grid0;
    grid0 = temp;
    // Exit on condition
}

void sim(double endDiff, Grid* g) {
    // Setup
    Grid* grid0 = g;
    Grid* grid1 = (Grid*)malloc(sizeof(Grid));
    if (grid1 == NULL) {
        printf("cannot allocate memory in sim");
        exit(-1);
        return;
    }
    Grid* temp;
    int width = grid1->width = g->width;
    int height = grid1->height = g->height;
    generateGrid(grid1, FALSE);
    double totalDiff;
    totalDiff = endDiff + 1;
    int gap = width * height / WORKERS;
    ThreadArguements th;
    //---------------------
    while (totalDiff > endDiff) {
        totalDiff = 0;

        for (int i = 0; i < WORKERS; i++) {
            th.start = i * gap;
            th.end = (i + 1) * gap;
            th.g0 = grid0;
            th.g1 = grid1;
            if (i != WORKERS - 1) {
                th.end = (i + 1) * gap;
            } else {
                th.end = width * height;
            }
        }
        printf("%f\n", totalDiff);

        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                printf("%f ", grid0->val[x][y]);
            }
            printf("\n");
        }
    }
}

int main() {
    srand(time(NULL));
    Grid g;
    g.width = 8u;
    g.height = 8u;
    generateGrid(&g, TRUE);
    sim(0.0000001, &g);

    return 0;
}