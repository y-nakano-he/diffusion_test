#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>

#define NX 20000
#define NY 20000

float data[2][NY][NX];

double time_diff_sec(struct timeval st, struct timeval et)
{
    return (double)(et.tv_sec-st.tv_sec)+(et.tv_usec-st.tv_usec)/1000000.0;
}

void init()
{
    int x, y;
    int cx = NX/2, cy = 0; /* center of ink */
    int rad = (NX+NY)/8; /* radius of ink */
    
    #pragma omp parallel for schedule(static) private(x)
    for(y = 0; y < NY; y++) {
        for(x = 0; x < NX; x++) {
            float v = 0.0;
            if (((x-cx)*(x-cx)+(y-cy)*(y-cy)) < rad*rad) {
                v = 1.0;
            }
            data[0][y][x] = v;
            data[1][y][x] = v;
        }
    }
    return;
}

/* Calculate for one time step */
/* Input: data[t%2], Output: data[(t+1)%2] */
void calc(int nt)
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
    init();
    gettimeofday(&t1, NULL);
    calc(nt);
    gettimeofday(&t2, NULL);

    {
        double sec;
        double gflops;
        int op_per_point = 5; // 4 add & 1 multiply per point
        #pragma omp parallel
        {
            #pragma omp master
            {
            threads = omp_get_num_threads();
            }
        }
        sec = time_diff_sec(t1, t2);
        gflops = ((double)NX*NY*nt*op_per_point)/sec/1e+9;
        printf("%d,%d,%d,%d,%.6f,%.3f\n", threads, nt, NX, NY, sec, gflops);
    }

    return 0;
}
