#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>

// This wordlist is part of 'SecList' and can
// be found here:
// https://github.com/danielmiessler/SecLists/blob/master/Miscellaneous/lang-english.txt
#define WORDLIST_PATH "./assets/lang-english.txt"
#define OUTPUT_PATH "./assets/data.bin"

static inline bool get_next_line(char *buffer, size_t size, char **target_buffer, uint32_t *target_buffer_size) {
    static uintptr_t index = 0;

    if(index >= size)
        return false;

    *target_buffer = buffer + index;

    while(buffer[index++] != '\n')
        *target_buffer_size += 1;

    return true;
}

int main(int argc, char **argv) {
    int wordlist_fd = open(WORDLIST_PATH, O_RDONLY);

    // TODO: Error handeling
    struct stat wordlist_stat;
    stat(WORDLIST_PATH, &wordlist_stat);

    char *wordlist_buffer = mmap(NULL, wordlist_stat.st_size, PROT_READ, MAP_PRIVATE, wordlist_fd, 0);

    void *output_buffer = NULL;
    uintptr_t output_buffer_size = 0;
    uint32_t index = 0;

    bool status = true;

    while(status) {
        char *key = NULL;
        uint32_t key_size = 0;

        status = get_next_line(wordlist_buffer, wordlist_stat.st_size, &key, &key_size);
        output_buffer = realloc(output_buffer, output_buffer_size + key_size + 4);

        memcpy(output_buffer + output_buffer_size, &key_size, sizeof(key_size));
        memcpy(output_buffer + output_buffer_size + 4, key, key_size);

        output_buffer_size += key_size + 4;

        index ++;
    }

    int output_fd = open(OUTPUT_PATH, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR);

    write(output_fd, &index, sizeof(index));
    write(output_fd, output_buffer, output_buffer_size);

    close(output_fd);
    close(wordlist_fd);

    munmap(wordlist_buffer, wordlist_stat.st_size);

    free(output_buffer);

    return 0;
}
