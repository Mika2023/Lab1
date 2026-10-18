#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include "timer.h"

const double G = 6.67430e-11;
const double EPSILON = 1e-18;

typedef struct
{
    double m;
    double x, y, z;
    double vx, vy, vz;
} Particle;

typedef struct
{
    double fx, fy, fz;
} Force; 

Particle *particles;
Force *global_forces;
Force **thread_forces;

int N;
int nthreads;
double tend;
double time_step;
int num_steps;
int chunk;
FILE *out_fp;

pthread_barrier_t barrier;

void Get_input(const char* input_file);
void Allocate_memory(void);
void Free_memory(void);

void* Thread_work(void* rank);

void Calculate_forces(long my_rank);
void Reduce_forces(int start_i, int end_i);
void Update_particles(int start_i, int end_i);

int main(int argc, char* argv[]) 
{ 
    pthread_t* thread_handles;
    double start, finish;

    if (argc < 6) 
    { 
        printf("Формат входящих данных: <nthreads> <tend> <time_step> <input_file> <output_file>\n");
        return 1;
    }

    nthreads = atoi(argv[1]); 
    tend = atof(argv[2]); 
    time_step = atof(argv[3]); 
    const char* input_file = argv[4]; 
    const char* output_file = argv[5]; 
    
    Get_input(input_file); 
    num_steps = (int) (tend / time_step);
    chunk = (N + nthreads - 1) / nthreads;

    out_fp = fopen(output_file, "w"); 
    if (!out_fp) 
    { 
        perror("Ошибка открытия выходного файла"); 
        return 1; 
    } 
    
    pthread_barrier_init(&barrier, NULL, nthreads); 
    thread_handles = malloc(nthreads * sizeof(pthread_t)); 
    
    GET_TIME(start);
    for (long t = 0; t < nthreads; t++) 
    { 
        pthread_create(&thread_handles[t], NULL, Thread_work, (void*)t); 
    } 
    for (int t = 0; t < nthreads; t++) 
    { 
        pthread_join(thread_handles[t], NULL); 
    } 
    GET_TIME(finish);
    
    printf("Результаты вычислений\n"); 
    printf("Количество точек: %d\n", N); 
    printf("Количество потоков: %d\n", nthreads); 
    printf("Конечное значение времени: %.2f с шагом %.4f\n", tend, time_step); 
    printf("Затраченное время выполнения: %.6f секунд\n", finish - start); 
    
    fclose(out_fp); 
    pthread_barrier_destroy(&barrier); 
    Free_memory();
    free(thread_handles); 
    
    return 0;
}

void Get_input(const char* input_file) {
   FILE* in_fp;
   int i;

   in_fp = fopen(input_file, "r");
   if (!in_fp) {
      perror("Ошибка открытия входного файла");
      exit(1);
   }

   if (fscanf(in_fp, "%d", &N) != 1) {
      fprintf(stderr, "Ошибка чтения числа точек\n");
      exit(1);
   }

   Allocate_memory();

   for (i = 0; i < N; i++) {
        int result_scan = fscanf(in_fp, "%lf %lf %lf %lf %lf %lf %lf", 
         &particles[i].m, 
         &particles[i].x, &particles[i].y, &particles[i].z, 
         &particles[i].vx, &particles[i].vy, &particles[i].vz);

        if (result_scan != 7) {
            fprintf(stderr, "Ошибка чтения данных для частицы %d\n", i);
            exit(1);
        }
    }

   fclose(in_fp);
} 

void Allocate_memory() {
   int t;

   particles = malloc(N * sizeof(Particle));
   global_forces = malloc(N * sizeof(Force));
   thread_forces = malloc(nthreads * sizeof(Force*));
   
   for (t = 0; t < nthreads; t++) {
      thread_forces[t] = malloc(N * sizeof(Force));
   }
} 

void Free_memory() {
   int t;

   free(particles);
   free(global_forces);
   for (t = 0; t < nthreads; t++) {
      free(thread_forces[t]);
   }
   free(thread_forces);
}

void* Thread_work(void* rank) {
   long my_rank = (long) rank;
   int start_i, end_i;

   start_i = my_rank * chunk;
   end_i = (start_i + chunk < N) ? start_i + chunk : N;

   for (int step = 0; step <= num_steps; step++) {
      
      Calculate_forces(my_rank);
      pthread_barrier_wait(&barrier);
      
      Reduce_forces(start_i, end_i);
      pthread_barrier_wait(&barrier);
      
      if (my_rank == 0 && out_fp != NULL) {
         fprintf(out_fp, "%.6f", step * time_step);

         for (int i = 0; i < N; i++) {
            fprintf(out_fp, ",%.6f,%.6f,%.6f", particles[i].x, particles[i].y, particles[i].z);
         }
         fprintf(out_fp, "\n");
      }
      pthread_barrier_wait(&barrier);
      
      Update_particles(start_i, end_i);
      pthread_barrier_wait(&barrier);

   }

   return NULL;
}

void Calculate_forces(long my_rank) {
   int i, j;
   double rx, ry, rz, dist_sq, dist, coef;
   double fx, fy, fz;

   for (i = 0; i < N; i++) {
      thread_forces[my_rank][i].fx = 0.0;
      thread_forces[my_rank][i].fy = 0.0;
      thread_forces[my_rank][i].fz = 0.0;
   }

   for (i = my_rank; i < N - 1; i += nthreads) {
      for (j = i + 1; j < N; j++) {
         rx = particles[j].x - particles[i].x;
         ry = particles[j].y - particles[i].y;
         rz = particles[j].z - particles[i].z;
         dist_sq = rx*rx + ry*ry + rz*rz + EPSILON;
         
         dist = sqrt(dist_sq);
         coef = G * particles[i].m * particles[j].m / (dist_sq * dist);
         
         fx = coef * rx;
         fy = coef * ry;
         fz = coef * rz;
         
         thread_forces[my_rank][i].fx += fx;
         thread_forces[my_rank][i].fy += fy;
         thread_forces[my_rank][i].fz += fz;
         
         thread_forces[my_rank][j].fx -= fx;
         thread_forces[my_rank][j].fy -= fy;
         thread_forces[my_rank][j].fz -= fz;
      }
   }
}

void Reduce_forces(int start_i, int end_i) {
   int i, t;

   for (i = start_i; i < end_i; i++) {
      global_forces[i].fx = 0.0;
      global_forces[i].fy = 0.0;
      global_forces[i].fz = 0.0;
      
      for (t = 0; t < nthreads; t++) {
         global_forces[i].fx += thread_forces[t][i].fx;
         global_forces[i].fy += thread_forces[t][i].fy;
         global_forces[i].fz += thread_forces[t][i].fz;
      }
   }
}

void Update_particles(int start_i, int end_i) {
   for (int i = start_i; i < end_i; i++) {
      particles[i].x += particles[i].vx * time_step;
      particles[i].y += particles[i].vy * time_step;
      particles[i].z += particles[i].vz * time_step;
      
      particles[i].vx += (global_forces[i].fx / particles[i].m) * time_step;
      particles[i].vy += (global_forces[i].fy / particles[i].m) * time_step;
      particles[i].vz += (global_forces[i].fz / particles[i].m) * time_step;
   }
}