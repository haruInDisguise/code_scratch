#define _GNU_SOURCE
#define __need_IOV_MAX

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define PIPESIZE (1 * 1024 * 1024)
#define VM_SIZE PIPESIZE
#define VM_LIMIT IOV_MAX

#define BUFFER_SIZE PIPESIZE

// Thanks a lot to this thread...
// https://www.reddit.com/r/unix/comments/6gxduc/comment/diua761/?context=8&depth=9

int main(int argc, char **argv) {
    char *msg = NULL;

    if(argc <= 1)
        msg = "y\n";
    else
        msg = argv[1];

    size_t msg_size = 0;
    char *msg_index = msg;
    while (*(msg_index++) != '\0') msg_size++;

    const size_t buffer_total_size = BUFFER_SIZE;
    const size_t buffer_nmemb = ((size_t)buffer_total_size / (size_t)msg_size);
    void *buffer = malloc(buffer_total_size);

    size_t buffer_index = 0;
    while(buffer_index++ < buffer_nmemb)
        memcpy(buffer + msg_size * buffer_index, msg, msg_size);

    fcntl(1, F_SETPIPE_SZ, PIPESIZE);

    // Populate IOV buffers
    struct iovec iov_array[VM_LIMIT];
    for(int i = 0; i < VM_LIMIT; i++) {
        iov_array[i].iov_base = buffer;
        iov_array[i].iov_len = buffer_total_size;
    }

    while(vmsplice(1, iov_array, VM_LIMIT, 0));

    return EXIT_SUCCESS;
}
