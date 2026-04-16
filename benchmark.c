/*  CS519, Spring 2026: Project 2
    Written by: Arthur Levitsky and Alexander Wu
    Description: Benchmarking program for observing page faults with and without extents.
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

# define SYS_PAGE_SIZE sysconf(_SC_PAGESIZE) // Typically 4KiB page size.
# define PAGES 16384L
# define SYS_ENABLE_EXTENT 449

void arg_check(int argc, char *argv[]);
double get_total_time_ms(struct timespec start, struct timespec end);

int main(int argc, char *argv[]) {
    pid_t pid = getpid();
    struct timespec t_start, t_end;
    struct rusage u_start, u_end; 
    long buffer_size, minor_faults, major_faults;
    char *u_buffer; 

    arg_check(argc, argv);

    if (strcmp(argv[1], "true") == 0) {
        if (syscall(SYS_ENABLE_EXTENT) != 0) {
            perror("Error calling sys_enable_extent!");
        }
    }

    buffer_size = SYS_PAGE_SIZE * PAGES;

    printf("PID: %d\n", pid);
    printf("Total Pages: %ld\n", PAGES);
    printf("System Page Size (B): %ld\n", SYS_PAGE_SIZE);
    printf("Total Buffer Size (B): %ld\n", buffer_size);
    printf("Total Buffer Size (MiB): %f\n", (float) buffer_size / (float) (1024L * 1024L));

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    getrusage(RUSAGE_SELF, &u_start);

    // Using malloc() causes a page fault on the first byte offset.
    u_buffer = malloc(buffer_size);
    if (u_buffer == NULL) {
        perror("couldn't allocate memory to u_buffer");
        exit(1);
    }

    // Skip the first byte offset and start at 4KiB offset. Iterate in 4KiB steps and insert value 'X' to force page faults.
    for (size_t b_offset = SYS_PAGE_SIZE; b_offset < buffer_size; b_offset += SYS_PAGE_SIZE) {
        u_buffer[b_offset] = 'X';
    }

    getrusage(RUSAGE_SELF, &u_end);
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    minor_faults = u_end.ru_minflt - u_start.ru_minflt;
    major_faults = u_end.ru_majflt - u_start.ru_majflt;

    printf("Minor Page Faults: %ld\n", minor_faults);
    printf("Major Page Faults: %ld\n", major_faults);
    printf("Total time (ms): %f\n", get_total_time_ms(t_start, t_end));

    free(u_buffer);

    return 0; 
}

void arg_check(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Usage: %s [USE_EXTENTS]\n", argv[0]);
        printf("-------------------------------------------------------\n");
        printf("USE_EXTENTS - Set boolean to use extents.   (true or false)\n");
        exit(1);
    }
    if (argc != 2) {
        printf("Insufficient parameters! (Required 1 but provided %d)\n", argc - 1);
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
