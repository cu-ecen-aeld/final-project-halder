#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define PAGE_SIZE 4096

volatile sig_atomic_t running = 1;

void handle_signal(int signal)
{
    (void)signal;
    running = 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <memory_mb> <cpu_percent>\n", argv[0]);
        fprintf(stderr, "Example: %s 300 80\n", argv[0]);
        return 1;
    }

    long memory_mb = atol(argv[1]);
    long cpu_percent = atol(argv[2]);

    if (memory_mb <= 0 || memory_mb > 1000) {
        fprintf(stderr, "Memory must be between 0 MB and 1000 MB.\n");
        return 1;
    }

    if (cpu_percent < 0 || cpu_percent > 100) {
        fprintf(stderr, "CPU percentage must be between 0 and 100.\n");
        return 1;
    }

    size_t memory_size = (size_t)memory_mb * 1024 * 1024;

    printf("System monitor demo\n");
    printf("-------------------\n");
    printf("Memory: %ld MB\n", memory_mb);
    printf("CPU load: %ld%%\n", cpu_percent);
    printf("Press Ctrl+C to stop.\n\n");

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    char *memory = malloc(memory_size);

    if (memory == NULL) {
        perror("malloc");
        return 1;
    }

    /*
     * Touch every memory page.
     *
     * malloc() only reserves virtual address space initially.
     * Writing to every page forces Linux to actually back the
     * allocation with physical memory.
     */
    printf("Allocating %ld MB...\n", memory_mb);

    for (size_t i = 0; i < memory_size; i += PAGE_SIZE) {
        memory[i] = 1;
    }

    printf("Memory allocated successfully.\n");
    printf("Starting workload...\n\n");


    /*
     * CPU load control.
     *
     * We divide each 100 ms period into:
     *
     *     cpu_percent % busy
     *     remaining % sleeping
     *
     * For example:
     *
     *     100% -> 100 ms busy
     *      50% ->  50 ms busy + 50 ms sleep
     *      20% ->  20 ms busy + 80 ms sleep
     */
    const long period_us = 100000;

    while (running) {

        /*
         * CPU workload duration.
         */
        long busy_us = period_us * cpu_percent / 100;

        struct timespec start;
        clock_gettime(CLOCK_MONOTONIC, &start);

        /*
         * Perform calculations until the requested amount of
         * CPU time has elapsed.
         */
        volatile unsigned long counter = 0;

        while (running) {
            for (unsigned long i = 0; i < 10000; i++) {
                counter += i * i;
            }

            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);

            long elapsed_us = (now.tv_sec - start.tv_sec) * 1000000L + (now.tv_nsec - start.tv_nsec) / 1000;

            if (elapsed_us >= busy_us) {
                break;
            }
        }

        /*
         * Sleep for the remainder of the 100 ms period.
         */
        long sleep_us = period_us - busy_us;

        if (running && sleep_us > 0) {
            struct timespec sleep_time = {
                .tv_sec = sleep_us / 1000000,
                .tv_nsec = (sleep_us % 1000000) * 1000
            };

            nanosleep(&sleep_time, NULL);
        }

        /*
         * Periodically touch the allocated memory.
         *
         * This keeps the allocation active without continuously
         * consuming all CPU time just doing memory accesses.
         */
        if (running) {
            for (size_t i = 0; i < memory_size; i += PAGE_SIZE) {
                memory[i]++;
            }
        }
    }

    printf("\nStopping workload...\n");

    free(memory);

    printf("Memory released.\n");

    return 0;
}
