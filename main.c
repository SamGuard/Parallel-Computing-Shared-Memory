#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

#define PATTERN_ZERO 0
#define PATTERN_GRADIENT 1
#define PATTERN_RANDOM 2

//#define VERBOSE // Whether to print test info or not

int STOP = FALSE;  // Whether or not the threads should exit
pthread_barrier_t startOfIterationBarrier, endOfIterationBarrier;
pthread_mutex_t totalDiffMutex;

typedef struct grid {
    double** val;                // Array for each value in cell
    unsigned int width, height;  // Size of the grid
} Grid;

// Inputs to give the threads
typedef struct ThreadArguements {
    unsigned int start, end;
    Grid *g0, *g1;    // Pointer to 2 grids
    double* maxDiff;  // Max difference between previous cell and current cell
} ThreadArguements;

void printGrid(Grid* g) {
    for (int x = 0; x < g->width; x++) {
        for (int y = 0; y < g->height; y++) {
            printf("%f ", g->val[x][y]);
        }
        printf("\n");
    }
}

// initialises grid with
void generateGrid(Grid* g, int init) {
    unsigned int width = g->width;
    unsigned int height = g->height;
    // Allocate memory for each grid cell
    g->val = (double**)malloc(width * sizeof(double*));
    if (g->val == NULL) {
        printf("Cannot allocate memory for new grid");
        exit(-1);
        return;
    }
    for (unsigned int i = 0; i < width; i++) {
        g->val[i] = (double*)malloc(height * sizeof(double));
        if (g->val[i] == NULL) {
            printf("Cannot allocate memory for new row of grid");
            exit(-1);
            return;
        }
        for (unsigned int j = 0; j < height; j++) {
            // Initialise each memory location with a value
            // If init == True assign left and top edges to 1 and the other
            // edges to 0. The rest are set to random values
            if (init == PATTERN_RANDOM) {
                g->val[i][j] = (double)(rand() / 10) / (double)(RAND_MAX / 10);
            } else if (init == PATTERN_GRADIENT) {
                if (i == 0 || j == 0) {
                    g->val[i][j] = 1;
                } else if (i == width - 1 || j == height - 1) {
                    g->val[i][j] = 0;
                } else {
                    g->val[i][j] =
                        (double)(rand() / 10) / (double)(RAND_MAX / 10);
                }
            } else if (init == PATTERN_ZERO) {
                g->val[i][j] = 0;
            }
        }
    }
}

// This function applies the relaxation to the grid. Called by each thread once
// per iteration
void simThread(unsigned int start, unsigned int end, Grid* grid0, Grid* grid1,
               double* maxDiff) {
    unsigned int i, x, y;
    double newVal;  // Value to be put into the new grid
    double diff;  // Temp value to store the difference between the new and old
                  // value
    int width = grid0->width;
    int height = grid0->height;
    *maxDiff = 0;  // Pointer value in previous function to store maximum
                   // difference between two cells in an iteration
    int isBigger;
    for (i = start; i < end; i++) {
        x = i / height;
        y = i % height;
        if (x == 0 || y == 0 || x == width - 1 || y == height - 1) {
            grid1->val[x][y] =
                grid0->val[x][y];  // If on boundery just move value
        } else {
            newVal = (grid0->val[x - 1][y] + grid0->val[x + 1][y] +
                      grid0->val[x][y - 1] + grid0->val[x][y - 1]) /
                     4;  // Take averaage of all 4 surrounding values

            diff = fabs(newVal - grid0->val[x][y]);
            isBigger = (*maxDiff < diff);
            *maxDiff = isBigger * diff + (1 - isBigger) * (*maxDiff);
            grid1->val[x][y] = newVal;
        }
    }
}

void* threadFunc(ThreadArguements* args) {
    // Store all information
    Grid* temp;
    Grid* grid0 = args->g0;
    Grid* grid1 = args->g1;
    int start = args->start, end = args->end;
    double* maxDiff = args->maxDiff;
    double diff;
    // Do iteration
    while (1 == 1) {
        pthread_barrier_wait(&startOfIterationBarrier);
        if (STOP == TRUE) return (void*)0;
        diff = 0;
        simThread(start, end, grid0, grid1, &diff);
        // Swap grid pointers
        temp = grid1;
        grid1 = grid0;
        grid0 = temp;
        pthread_mutex_lock(&totalDiffMutex);
        *maxDiff = (*maxDiff < diff) ? diff : *maxDiff;
        pthread_mutex_unlock(&totalDiffMutex);

        pthread_barrier_wait(&endOfIterationBarrier);
    }
    // Exit on condition
}

void sim(double endDiff, int PRINT_DATA, unsigned int WORKERS, Grid* g) {
    time_t startTime = time(NULL);
    // Setup
    Grid* grid0 = g;
    Grid* grid1 = (Grid*)malloc(sizeof(Grid));
    if (grid1 == NULL) {
        printf("cannot allocate memory in sim");
        exit(-1);
        return;
    }
    Grid* temp;
    unsigned int iterations = 0;
    int width = grid1->width = g->width;
    int height = grid1->height = g->height;
    generateGrid(grid1, FALSE);  // Create grid filled with 0's
    double maxDiff;

    // Create threads
    int gap = (width * height) / WORKERS;
    ThreadArguements th[WORKERS];
    pthread_t threads[WORKERS];
    pthread_barrier_init(&startOfIterationBarrier, NULL, WORKERS + 1);
    pthread_barrier_init(&endOfIterationBarrier, NULL, WORKERS + 1);

    for (int i = 0; i < WORKERS; i++) {
        th[i].start = i * gap;
        th[i].end = (i + 1) * gap;
        th[i].g0 = grid0;
        th[i].g1 = grid1;
        th[i].maxDiff = &maxDiff;
        if (i != WORKERS - 1) {
            th[i].end = (i + 1) * gap;
        } else {
            th[i].end = width * height;
        }

        pthread_create(&threads[i], NULL, (void*)threadFunc, &th[i]);
    }
    maxDiff = 0;
    pthread_barrier_wait(&startOfIterationBarrier);
    //--------------START OF LOOP------------------------
    while (1) {
        // Wait for all threads to finish here
        pthread_barrier_wait(&endOfIterationBarrier);

// Print out results of iterations
#ifdef VERBOSE
        printf("%f\n", totalDiff);
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                printf("%f ", grid0->val[x][y]);
            }
            printf("\n");
        }
#endif
        // Reset any vars for next iteration
        iterations++;
        if (maxDiff < endDiff) break;
        maxDiff = 0;
        // Set the threads off again
        pthread_barrier_wait(&startOfIterationBarrier);
    }
    //-------------------END OF LOOP---------------------

    STOP = TRUE;  // Global Variable - threads can only read once start of
                  // iteration barrier has been called
    pthread_barrier_wait(
        &startOfIterationBarrier);  // Makes the threads start the loop again,
                                    // causing them to check if STOP == TRUE

#ifdef VERBOSE
    printf("Iterations: %d\n", iterations);
    printf("Time: %ds\n", (int)(time(NULL) - startTime));
    printf("Workers %d\n", WORKERS);
    printf("\n");
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            printf("%f ", grid0->val[x][y]);
        }
        printf("\n");
    }
#else
    printf("%d,%u,%u,%f,%d,%d", WORKERS, width, height, endDiff, iterations,
           (int)(time(NULL) - startTime));
    if (PRINT_DATA == TRUE) {
        printf(",");
        printGrid(grid0);
    }
    printf("\n");
#endif
    pthread_barrier_destroy(&startOfIterationBarrier);
    pthread_barrier_destroy(&endOfIterationBarrier);
    pthread_mutex_destroy(&totalDiffMutex);
}

int main(int argc, char** argv) {
    if (argc != 6) {
        printf(
            "Please inputs 5 arguements: Number of Threads, grid width and "
            "height and precision\n");
        return -1;
    }
    unsigned int WORKERS = atoi(argv[1]);
    unsigned int GRID_WIDTH = atoi(argv[2]);
    unsigned int GRID_HEIGHT = atoi(argv[3]);
    double PRECISION = atof(argv[4]);
    int PRINT_DATA = atoi(argv[5]);

    srand(time(NULL));
    Grid g;
    Grid inputGrid;
    g.width = inputGrid.width = (unsigned int)GRID_WIDTH;
    g.height = inputGrid.height = (unsigned int)GRID_HEIGHT;
    generateGrid(&g, (PRINT_DATA == TRUE) ? PATTERN_GRADIENT : PATTERN_RANDOM);
    generateGrid(&inputGrid, PATTERN_ZERO);

    for (int x = 0; x < GRID_WIDTH; x++) {
        for (int y = 0; y < GRID_HEIGHT; y++) {
            inputGrid.val[x][y] = g.val[x][y];
        }
    }

    sim(PRECISION, PRINT_DATA, WORKERS, &g);

    if(PRINT_DATA == TRUE){
        printf(",");
        printGrid(&inputGrid);
    }

    return 0;
}