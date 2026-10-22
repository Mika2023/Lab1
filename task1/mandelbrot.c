#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "../timer.h"

#define X_MIN (-2.0)
#define X_MAX 0.5
#define Y_MIN (-1.25)
#define Y_MAX 1.25
#define COUNTER_MAX 1000
#define BUFFER_SIZE 64
#define OUT_FILE "mandelbrot.csv"

int thread_count;
int n_points;

double* x;
double* y;

unsigned int base_seed;

void* routine(void* rank);

int parse_positive(const char* str, int* value){
	char* end;
	errno = 0;
	long v = strtol(str, &end, 10);
	if(errno != 0 || end == str || *end != '\0' || v <= 0 || v > INT_MAX)
		return -1;
	*value = (int)v;
	return 0;
}

int main(int argc, char** argv){

	if(argc != 3){
		fprintf(stderr, "usage: %s nthreads npoints\n", argv[0]);
		return 1;
	}
	if(parse_positive(argv[1], &thread_count) != 0){
		fprintf(stderr, "Error: nthreads must be a positive integer\n");
		return 1;
	}
	if(parse_positive(argv[2], &n_points) != 0){
		fprintf(stderr, "Error: npoints must be a positive integer\n");
		return 1;
	}

	x = malloc(n_points*sizeof(double));
	y = malloc(n_points*sizeof(double));
	pthread_t* thread_handles = malloc(thread_count*sizeof(pthread_t));
	if(x == NULL || y == NULL || thread_handles == NULL){
		perror("malloc");
		free(x);
		free(y);
		free(thread_handles);
		return 1;
	}

	base_seed = (unsigned int)time(NULL);

	 int created = 0;


	double start_time, end_time;
    GET_TIME(start_time);

	for(long i = 0; i < thread_count; ++i){
		int err = pthread_create(&thread_handles[i], NULL, routine, (void*)i);
		if(err != 0){
			break;
		}
		 created++;
	}

	int failed = (created != thread_count);
	for(int i = 0; i < created; ++i){
		int err = pthread_join(thread_handles[i], NULL);
		if(err != 0){
			failed = 1;
		}
	}

	GET_TIME(end_time);
	printf("Затраченное время выполнения: %e\n", end_time - start_time);

	if(failed){
		free(x);
		free(y);
		free(thread_handles);
		return 1;
	}

	FILE* f = fopen(OUT_FILE, "w");
	if(f == NULL){
		free(x);
		free(y);
		free(thread_handles);
		return 1;
	}

	fprintf(f, "x,y\n");
	for(int i = 0; i < n_points; ++i)
		fprintf(f, "%.17g,%.17g\n", x[i], y[i]);

	if(fclose(f) != 0){
		free(x);
		free(y);
		free(thread_handles);
		return 1;
	}

	free(x);
	free(y);
	free(thread_handles);

	return 0;
}

void* routine(void* rank){
    long my_rank = (long)rank;

    int q = n_points / thread_count;
    int rem = n_points % thread_count;
    int my_points = q + (my_rank < rem ? 1 : 0);
    int my_first_i = my_rank*q + (my_rank < rem ? my_rank : rem);

    unsigned int my_seed = base_seed + (unsigned int)my_rank*7919u;

    double local_buffer_x[BUFFER_SIZE]; 
    double local_buffer_y[BUFFER_SIZE];
    int local_count = 0;

    int points_found = 0;
    
    while(points_found < my_points){
        double r_x = (double)rand_r(&my_seed)/(double)RAND_MAX*(X_MAX - X_MIN) + X_MIN;
        double r_y = (double)rand_r(&my_seed)/(double)RAND_MAX*(Y_MAX - Y_MIN) + Y_MIN;

        double z_x = 0.0;
        double z_y = 0.0;
        int inside = 1;

        for(int counter = 0; counter < COUNTER_MAX; ++counter){
            double t_x = z_x*z_x - z_y*z_y + r_x;
            double t_y = 2.0*z_x*z_y + r_y;
            z_x = t_x;
            z_y = t_y;
            if(z_x*z_x + z_y*z_y > 4.0){
                inside = 0;
                break;
            }
        }

        if(inside){
            local_buffer_x[local_count] = r_x;
            local_buffer_y[local_count] = r_y;
            local_count++;
            points_found++;

            if (local_count == BUFFER_SIZE) {
                memcpy(&x[my_first_i], local_buffer_x, BUFFER_SIZE * sizeof(double));
                memcpy(&y[my_first_i], local_buffer_y, BUFFER_SIZE * sizeof(double));
                
                my_first_i += BUFFER_SIZE;
                local_count = 0;
            }
        }
    }

    if (local_count > 0) {
        memcpy(&x[my_first_i], local_buffer_x, local_count * sizeof(double));
        memcpy(&y[my_first_i], local_buffer_y, local_count * sizeof(double));
    }

    return NULL;
}
