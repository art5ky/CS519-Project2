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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/syscall.h>

# define SYS_PAGE_SIZE sysconf(_SC_PAGESIZE) // Typically 4KiB page size.
# define SYS_ENABLE_EXTENT 449

void arg_check(int argc, char *argv[]);
double get_total_time_ms(struct timespec start, struct timespec end);

int main(int argc, char *argv[]) {
    pid_t pid = getpid();
    struct timespec t_start, t_end;
    struct rusage u_start, u_end;
    long minor_faults, major_faults;
    
    struct stat st;
    long long f_size;
    char *mm_addr;
    int fd;

    arg_check(argc, argv);

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Failed to open file!");
        return 1;
    }

    if (fstat(fd, &st) == -1) {
        perror("Failed to get size of file!");
        close(fd);
        return 1; 
    }

    f_size = st.st_size;

    mm_addr = mmap(NULL, f_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mm_addr == MAP_FAILED) {
        perror("Memory map failed!");
        close(fd);
        return 1; 
    }
    close(fd);

    if (syscall(SYS_ENABLE_EXTENT) != 0) {
        perror("Error calling sys_enable_extent!");
    }

    clock_gettime(CLOCK_MONOTONIC, &t_start);
    getrusage(RUSAGE_SELF, &u_start);

    for (size_t mm_offset = 0; mm_offset < f_size; mm_offset += SYS_PAGE_SIZE) {
        mm_addr[mm_offset] = 'X';
    }

    getrusage(RUSAGE_SELF, &u_end);
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    if (munmap(mm_addr, f_size) == -1) {
        perror("Failed to deallocate memory map!");
    }

    minor_faults = u_end.ru_minflt - u_start.ru_minflt;
    major_faults = u_end.ru_majflt - u_start.ru_majflt;

    printf("PID: %d\n", pid);
    printf("Total Pages: %lld\n", f_size / SYS_PAGE_SIZE);
    printf("System Page Size (B): %ld\n", SYS_PAGE_SIZE);
    printf("File Size (B): %lld\n", f_size);
    printf("File Size (MiB): %f\n", (float) f_size / (float) (1024L * 1024L));
    printf("Minor Page Faults: %ld\n", minor_faults);
    printf("Major Page Faults: %ld\n", major_faults);
    printf("Total time (ms): %f\n", get_total_time_ms(t_start, t_end));

    return 0; 
}

void arg_check(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Usage: %s [FILE_PATH]\n", argv[0]);
        printf("-------------------------------------------------------\n");
        printf("FILE_PATH - Required relative binary file path.  (e.g. test_files/tf_4MB.bin)\n");
        exit(1);
    }
    if (argc != 2) {
        printf("Insufficient parameters! (Required 1 but provided %d)\n", argc - 1);
        exit(1); 
    }
}

double get_total_time_ms(struct timespec start, struct timespec end) {
    return ((end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec)) / 1e6;
}
