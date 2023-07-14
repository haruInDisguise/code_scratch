#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include <time.h>

#define PIPESIZE (1 * 1024 * 1024)

// Currently, the throughput is measured for every second
// individually, instead of calculating an average value

void print_status(size_t bytes, size_t duration) {
    size_t gbytes_per_second = bytes / (1024 * 1024 * 1024);
    fprintf(stderr, "\33[2K\r%zu GiB/s TIme: %zu", gbytes_per_second, duration);
}

size_t pv_run(void) {
    size_t time_passed = 0,
           time_since_last = 0,
           time_start = time(NULL);

    size_t status = 0,
           bytes_total = 0,
           bytes_since_last = 0;

    fcntl(0, F_SETPIPE_SZ, PIPESIZE);

    while((status = splice(0, NULL, 1, NULL, PIPESIZE, 0 | SPLICE_F_MORE)) ) {
        bytes_total += status;
        time_passed = time(NULL) - time_start;

        if(time_since_last < time_passed) {
            print_status(bytes_total - bytes_since_last, time_passed);

            time_since_last = time_passed;
            bytes_since_last = bytes_total;
        }
    };

    return status;
}

int main(void) {
    size_t status_code = pv_run();

    return status_code;
}
