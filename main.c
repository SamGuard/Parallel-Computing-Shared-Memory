#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define WORKERS 16

#define VERBOSE
#define FILE_OUT

int STOP = 0;
pthread_barrier_t startOfIterationBarrier, endOfIterationBarrier;
pthread_mutex_t totalDiffMutex;

typedef struct grid {
    float** val;
    unsigned int width, height;
} Grid;

typedef struct ThreadArguements {
    unsigned int start, end;
    Grid *g0, *g1;
    double* totalDiff;
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

void *threadFunc(ThreadArguements* args) {
    // Store all information
    Grid* temp;
    Grid* grid0 = args->g0;
    Grid* grid1 = args->g1;
    int start = args->start, end = args->end;
    double* totalDiff = args->totalDiff;
    double diff;
    // Do iteration
    while (STOP == 0) {
        pthread_barrier_wait(&startOfIterationBarrier);

        diff = 0;
        simThread(start, end, grid0, grid1, &diff);
        // Swap grid pointers
        temp = grid1;
        grid1 = grid0;
        grid0 = temp;
        pthread_mutex_lock(&totalDiffMutex);
        *totalDiff += diff;
        pthread_mutex_unlock(&totalDiffMutex);

        pthread_barrier_wait(&endOfIterationBarrier);
    }
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
    generateGrid(grid1, FALSE);  // Create grid filled with 0's
    double totalDiff;
    totalDiff = endDiff + 1;

    // Create threads
    int gap = width * height / WORKERS;
    ThreadArguements th[WORKERS];
    pthread_t threads[WORKERS];
    pthread_barrier_init(&startOfIterationBarrier, NULL, WORKERS + 1);
    pthread_barrier_init(&endOfIterationBarrier, NULL, WORKERS + 1);

    for (int i = 0; i < WORKERS; i++) {
        th[i].start = i * gap;
        th[i].end = (i + 1) * gap;
        th[i].g0 = grid0;
        th[i].g1 = grid1;
        th[i].totalDiff = &totalDiff;
        if (i != WORKERS - 1) {
            th[i].end = (i + 1) * gap;
        } else {
            th[i].end = width * height;
        }

        pthread_create(&threads[i], NULL, threadFunc, &th[i]);
    }
    totalDiff = 0;
    pthread_barrier_wait(&startOfIterationBarrier);
    while (totalDiff > endDiff) {
        //Wait for all threads to finish here
        pthread_barrier_wait(&endOfIterationBarrier);
        //Print out results of iterations

        #ifdef VERBOSE
        printf("%f\n", totalDiff);
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                printf("%f ", grid0->val[x][y]);
            }
            printf("\n");
        }
        #endif
        //Reset any vars for next iteration
        totalDiff = 0;
        //Set the threads off again
        pthread_barrier_wait(&startOfIterationBarrier);
    }


}

int main() {
    srand(time(NULL));
    Grid g;
    g.width = 16u;
    g.height = 16u;
    generateGrid(&g, TRUE);
    sim(0.0000001, &g);

    return 0;
}