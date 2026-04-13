/* CS519, Spring 2026: Project 2
    Written by: Arthur Levitsky and Alexander Wu
    Description: Benchmarking program for observing page faults with and without extents using mult-threading.
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <pthread.h>

#define SYS_PAGE_SIZE sysconf(_SC_PAGESIZE) 
#define PAGES 16384L
#define SYS_ENABLE_EXTENT 449
#define NUM_THREADS 1

// Structure to pass data to each thread
typedef struct {
    char *buffer;
    size_t start_offset;
    size_t end_offset;
} thread_data_t;

void arg_check(int argc, char *argv[]);
double get_total_time_ms(struct timespec start, struct timespec end);

// Thread function to trigger page faults
void *touch_pages(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;

    for (size_t i = data->start_offset; i < data->end_offset; i += SYS_PAGE_SIZE) {
        data->buffer[i] = 'X';
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pid_t pid = getpid();
    struct timespec t_start, t_end;
    struct rusage u_start, u_end; 
    long buffer_size, minor_faults;
    char *u_buffer; 
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_args[NUM_THREADS];

    arg_check(argc, argv);

    // System call to enable extents for current process.
    if (strcmp(argv[1], "true") == 0) {
        if (syscall(SYS_ENABLE_EXTENT) != 0) {
            perror("Error calling sys_enable_extent!");
        }
    }

    buffer_size = SYS_PAGE_SIZE * PAGES;
    long pages_per_thread = PAGES / NUM_THREADS;

    printf("PID: %d | Threads: %d\n", pid, NUM_THREADS);
    printf("Total Pages: %ld\n", PAGES);
    printf("Pages Per Thread: %ld\n", pages_per_thread);
    printf("Total Buffer Size (B): %ld\n", buffer_size);
    printf("Total Buffer Size (MiB): %f\n", (float) buffer_size / (float) (1024L * 1024L));

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    getrusage(RUSAGE_SELF, &u_start);

    u_buffer = malloc(buffer_size);
    if (u_buffer == NULL) {
        perror("couldn't allocate memory to u_buffer");
        exit(1);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].buffer = u_buffer;
        thread_args[i].start_offset = i * pages_per_thread * SYS_PAGE_SIZE;
        
        // Handle the last thread's end boundary
        if (i == NUM_THREADS - 1) {
            thread_args[i].end_offset = buffer_size;
        } else {
            thread_args[i].end_offset = (i + 1) * pages_per_thread * SYS_PAGE_SIZE;
        }
        pthread_create(&threads[i], NULL, touch_pages, &thread_args[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL); 
    }
    getrusage(RUSAGE_SELF, &u_end);
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    minor_faults = u_end.ru_minflt - u_start.ru_minflt;

    printf("Minor Page Faults: %ld\n", minor_faults);
    printf("Total time (ms): %f\n", get_total_time_ms(t_start, t_end));

    free(u_buffer);
    return 0; 
}

void arg_check(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [USE_EXTENTS]\n", argv[0]);
        exit(1);
    }
    if (strcmp(argv[1], "true") != 0 && strcmp(argv[1], "false") != 0) {
        printf("Incompatible USE_EXTENTS! (true or false)\n");
        exit(1);
    }
}

double get_total_time_ms(struct timespec start, struct timespec end) {
    return ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec)) / 1e6;
}