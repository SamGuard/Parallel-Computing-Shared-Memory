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

// Stores all the values in the grid as well as meta data
typedef struct grid {
    double** val;                // Array for each value in cell
    unsigned int width, height;  // Size of the grid
} Grid;

// Inputs to give the threads
typedef struct ThreadArguements {
    unsigned int start, end;  // Range of indexs to calculate values for
    Grid *g0, *g1;            // Pointer to 2 grids
    double*
        maxDiff;  // Max difference between previous cell and current cell value
} ThreadArguements;

void printGrid(Grid* g) {
    for (int x = 0; x < g->width; x++) {
        for (int y = 0; y < g->height; y++) {
            printf("%.2f ", g->val[x][y]);
        }
        printf("\n");
    }
}

// initialises grid
// init decided what pattern to initialise the grid with
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
                if (i == 0 || (j == 0 && i != width - 1)) {
                    g->val[i][j] = 1;
                } else if (i == width - 1 || j == height - 1) {
                    g->val[i][j] = -1;
                } else {
                    g->val[i][j] = 0;//
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
// start is the start index in the grid
// end is the end index in the grid
// grid0 and grid1 are pointers to the current values of the grid. grid0 is
// being read from, grid1 is being written to.
// maxDiff is a pointer to were to store the maximum difference between a cells
// previous value and its current value
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
            continue;
        }
        newVal = (grid0->val[x - 1][y] + grid0->val[x + 1][y] +
                  grid0->val[x][y - 1] + grid0->val[x][y - 1]) /
                 4;  // Take averaage of all 4 surrounding values

        diff = fabs(newVal - grid0->val[x][y]);
        // Next to lines are used to avoid branching
        // isBigger will be 1 or 0
        isBigger = (*maxDiff < diff);
        // This next line essentially choosesn which value to take: maxDiff or
        // diff
        *maxDiff = isBigger * diff + (1 - isBigger) * (*maxDiff);
        // Set new value in other grid
        grid1->val[x][y] = newVal;
    }
}

// threadFunc is the function passed the thread creator.
// args is a pointer to a struct that contains all the information needed to run
// the simulation
void* threadFunc(ThreadArguements* args) {
    Grid* temp;
    Grid* grid0 = args->g0;
    Grid* grid1 = args->g1;
    int start = args->start, end = args->end;
    double* maxDiff = args->maxDiff;
    double diff;  // Stores the biggest difference between a cells previous and
                  // current value
    // Do iteration
    while (1 == 1) {
        pthread_barrier_wait(
            &startOfIterationBarrier);  // Wait until main thread says to go
        if (STOP == TRUE)
            return (void*)0;  // If the global variable STOP is set to TRUE then
                              // exit thread
        diff = 0;
        simThread(start, end, grid0, grid1,
                  &diff);  // This function calculates values between start
                           // index and the end index
        // Swap grid pointers - this avoids having to create a new grid each
        // iteration
        temp = grid1;
        grid1 = grid0;
        grid0 = temp;
        pthread_mutex_lock(&totalDiffMutex);  // Get lock as this is a shared
                                              // variable between threads
        *maxDiff = (*maxDiff < diff) ? diff : *maxDiff;
        pthread_mutex_unlock(&totalDiffMutex);

        pthread_barrier_wait(
            &endOfIterationBarrier);  // Wait for all other threads to finish
                                      // their iteration
    }
    // Exit on condition
}

// Runs the simulation given an input grid.
// endDiff is the precision it should continue until it hits
// VALIDATE_MODE is used to tell the program to print extra data
// WORKERS the amount of threads to use
// g is the input grid
// startTime is the time since the program has started
void sim(double endDiff, int VALIDATE_MODE, unsigned int WORKERS, Grid* g,
         time_t startTime) {
    // Setup
    Grid* grid0 = g;
    Grid* grid1 = (Grid*)malloc(
        sizeof(Grid));  // Create another grid to write data from grid0 to
    if (grid1 == NULL) {
        printf("cannot allocate memory in sim");
        exit(-1);
        return;
    }

    unsigned int iterations = 0;  // Stores the amount of iterations
    int width = grid1->width = g->width;
    int height = grid1->height = g->height;
    generateGrid(grid1, FALSE);  // Create grid filled with 0's
    double maxDiff;  // Stores the maximum difference between a cells previous
                     // and current value

    // Create threads
    int gap = (width * height) / WORKERS;  // Calculate how many cells each
                                           // thread should be allocated each
    ThreadArguements th[WORKERS];
    pthread_t threads[WORKERS];
    // Make 2 barriers that wait for all worker threads and the main thread
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
            th[i].end =
                width * height;  // The final thread takes on the remainder of
                                 // the cells as grid size may not perfectly be
                                 // divided by amount of workers
        }

        pthread_create(&threads[i], NULL, (void*)threadFunc, &th[i]);
    }

    maxDiff = 0;
    pthread_barrier_wait(
        &startOfIterationBarrier);  // Wait until all threads are ready
    //--------------START OF LOOP------------------------
    while (1) {
        // Wait for all threads to finish here
        pthread_barrier_wait(&endOfIterationBarrier);

// Print out results of iterations
#ifdef VERBOSE
        printf("%f\n", maxDiff);
        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                printf("%f ", grid0->val[x][y]);
            }
            printf("\n");
        }
#endif

        iterations++;
        if (maxDiff < endDiff) break;
        maxDiff = 0;
        // Fire off the threads for another iteration
        pthread_barrier_wait(&startOfIterationBarrier);
    }
    //-------------------END OF LOOP---------------------

    STOP = TRUE;  // Global Variable - threads can only read once start of
                  // iteration barrier has been called
    pthread_barrier_wait(
        &startOfIterationBarrier);  // Makes the threads start the loop again,
                                    // causing them to check if STOP == TRUE

#ifdef VERBOSE
    // Debugging data
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
    // Print out all important data
    printf("%d,%u,%u,%f,%d,%d", WORKERS, width, height, endDiff, iterations,
           (int)(time(NULL) - startTime));
    if (VALIDATE_MODE ==
        TRUE) {  // If in validation mode also print the outputted grid
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
    // Check correct command line arguements have been passed in
    if (argc != 6) {
        printf(
            "Please inputs 5 arguements: Number of Threads, grid width and "
            "height and precision\n");
        return -1;
    }

    time_t startTime = time(NULL);

    unsigned int WORKERS = atoi(argv[1]);     // Amount of threads to use
    unsigned int GRID_WIDTH = atoi(argv[2]);  //
    unsigned int GRID_HEIGHT = atoi(argv[3]);
    double PRECISION = atof(argv[4]);
    int VALIDATE_MODE =
        atoi(argv[5]);  // Whether to print extra data or not as well as other
                        // changes to help validation testing

    srand(
        (VALIDATE_MODE == TRUE)
            ? 27
            : time(
                  NULL));  // If validation mode is TRUE then set the seed to 27
    Grid g;
    Grid inputGrid;  // Saves the initial values
    g.width = inputGrid.width = (unsigned int)GRID_WIDTH;
    g.height = inputGrid.height = (unsigned int)GRID_HEIGHT;
    generateGrid(
        &g,
        (VALIDATE_MODE == TRUE)
            ? PATTERN_GRADIENT
            : PATTERN_RANDOM);  // If validation mode is TRUE use the pattern
                                // that should result in a grdient
    generateGrid(&inputGrid, PATTERN_ZERO);  // initialise input grid with zeros

    // Copy data to the input grid
    if (VALIDATE_MODE == TRUE) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            for (int y = 0; y < GRID_HEIGHT; y++) {
                inputGrid.val[x][y] = g.val[x][y];
            }
        }
    }

    sim(PRECISION, VALIDATE_MODE, WORKERS, &g, startTime);

    // print the inital values
    if (VALIDATE_MODE == TRUE) {
        printf(",");
        printGrid(&inputGrid);
    }

    return 0;
}