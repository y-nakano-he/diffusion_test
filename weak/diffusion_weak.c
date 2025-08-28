#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>
#include <math.h>

double time_diff_sec(struct timeval st, struct timeval et)
{
    return (double)(et.tv_sec-st.tv_sec)+(et.tv_usec-st.tv_usec)/1000000.0;
}

/* 2面分mallocで確保 */
void *malloc_data(int NY, int NX)
{
    void *ptr = malloc(2 * sizeof (float[NY][NX]));
    if (ptr == NULL) { perror("malloc"); return NULL; }
    return ptr;
}

void init(int NY, int NX, float data[2][NY][NX])
{
    int cx = NX/2, cy = 0;           /* center of ink */
    int rad = (NX + NY) / 8;         /* radius of ink */
    long r2 = (long)rad * rad;

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < NY; y++) {
        for (int x = 0; x < NX; x++) {
            long dx = x - cx;
            long dy = y - cy;
            float v = 0.0;
            if(dx*dx + dy*dy < r2) {
                v = 1.0;
            }
            data[0][y][x] = v;
            data[1][y][x] = v;
        }
    }
}

/* Calculate for one time step */
/* Input: data[t%2], Output: data[(t+1)%2] */
void calc(int nt, int NY, int NX, float data[2][NY][NX])
{
    int t, x, y;

    for (t = 0; t < nt; t++) {
        int from = t%2;
        int to = (t+1)%2;

        #pragma omp parallel for schedule(static) private(x)
        for (y = 1; y < NY-1; y++) {
            for (x = 1; x < NX-1; x++) {
                data[to][y][x] = 0.2 * (data[from][y][x]
                                        + data[from][y][x-1]
                                        + data[from][y][x+1]
                                        + data[from][y-1][x]
                                        + data[from][y+1][x]);
            }
        }
    }
    return;
}

int  main(int argc, char *argv[])
{
    struct timeval t1, t2;
    int nt = 20; /* number of time steps */

    int threads = 1;
    #pragma omp parallel
    { 
        #pragma omp master
        threads = omp_get_num_threads(); 
    }

    /* N = NB * √(threads) */
    int NB = 2000;
    int NX = (int)lround(NB * sqrt((double)threads));
    int NY = NX;

    /* メモリ確保と初期化 */
    float (*data)[NY][NX] = malloc_data(NY, NX);
    if(data == NULL) return 1;
    init(NY, NX, data);

    /* 計測実行 */
    gettimeofday(&t1, NULL);
    calc(nt, NY, NX, data);
    gettimeofday(&t2, NULL);

    int op_per_point = 5; // 4 add & 1 multiply per point
    double sec = time_diff_sec(t1, t2);
    double gflops = ((double)NX*NY*nt*op_per_point)/sec/1e+9;
    double elements_per_node = ((double)NX * (double)NY) / (double)threads;

    printf("%d,%d,%d,%d,%.1f,%.6f,%.3f\n",
        threads, nt, NX, NY, elements_per_node, sec, gflops);

    free(data);
    return 0;
}
