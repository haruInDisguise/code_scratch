#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PAGE_SIZE 4096

int main(int argc, char **argv) {
    char *msg = NULL;

    if(argc <= 1)
        msg = "yes\n";
    else
        msg = argv[1];

    size_t msg_size = strlen(*argv);

    const size_t buffer_nmemb = 2 * PAGE_SIZE;
    const size_t buffer_total_size = buffer_nmemb * msg_size;
    void *buffer = malloc(buffer_total_size);

    size_t buffer_index = 0;
    while(buffer_index++ < buffer_nmemb)
        memcpy(buffer + buffer_index * msg_size, msg, msg_size);

    while(1)
        write(1, buffer, buffer_total_size);

    return EXIT_SUCCESS;
}
