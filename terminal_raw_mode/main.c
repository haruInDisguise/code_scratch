#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <termios.h>

#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        if (!condition) {                                                                          \
            fprintf(stderr, "Assertion failed: " #condition "\r\n");                               \
            __builtin_debugbreak();                                                                \
        }                                                                                          \
    } while (0)

/* The Issue:
 * By default, the terminal starts out in "cannonical mode". In this mode, keyboard
 * output is only send to the program when the emulator encounters a linebreak (pressing enter sends
 * a linebreak by default). However we want to perform actions without having to wait for the
 * terminal, so we can view a previous entry by pressing 'UP' or quit the program by hitting 'ESC'
 * This is where platform specific abstraction comes into play. */

struct termios term_attributes_default;

static void terminal_disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attributes_default);
}

static void terminal_enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &term_attributes_default) == -1) {
        fprintf(stderr, "error: Failed to enable raw mode\n");
        exit(1);
    }

    struct termios new_config = term_attributes_default;

    /* Some flags are redundant only useful for
     * enabling raw mode on older/legacy terminals
     * and therfore extend compatibility
     * See: https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html */

    /* For an indepth explanation, see termios(3) */

    /* ECHO     - Enable echo
     * ICANON   - Canonical input (erase and kill)
     * IEXTEN   - Extended input character (the thing we really care about) */
    new_config.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    // IXON     - Enable flow control
    // ICRNL    - Translate CR to newline
    // INPCK    - Parity check (defunced error checking?)
    // ISTRIP   - Strip 8th bit of each input byte
    // BRKINT   - Ignore break condition (like Ctrl-C)
    new_config.c_iflag &= ~(IXON | INPCK | ISTRIP | BRKINT | ICRNL);

    // OPOST    - Post process output (newline being translatet
    //            into '\r\n' etc.)
    new_config.c_oflag &= ~(OPOST);

    // CS8      - Set character byte size. A byte probably likely of 8 bits.
    new_config.c_cflag |= (CS8);

    // Control characters
    // See: http://unixwiz.net/techtips/termios-vmin-vtime.html
    // VMIN     -
    // VTIME    - timeout (value * 100ms)
    // We will return after 1 bytes has been read
    new_config.c_cc[VMIN] = 1;
    new_config.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_config) == -1) {
        fprintf(stderr, "error: Unable to apply new config\n");
        exit(1);
    }
}

/* Restore cooked mode */
static void terminate(uint8_t status) {
    terminal_disable_raw_mode();
    exit(status);
}

static void handle_sigint(int signal) { terminate(1); }

/* Ideas about how to handle command line history:
 * 1. When a line is submitted, it is added to a ring-buffer
 *      - Submitting resets 'selected_entry'
 * 2. Moving up/down returnes the next/previous valid entry.
 *      - Typing will overwrite the selected entry
 *      -
 * - Submitting the input line
 * - Since the history is implemented as a ring buffer, it can only
 *   grow and is will subsequently overwrite previous entries.
 *
 * To acomplish this task, we will implement three functions:
 * - history_add_entry(): Adds an entry onto the list,
 *   potentially overwriting previous entries.
 * - history_get_next_entry(): Returns the next selected entry.
 * - history_get_previous_entry(): Returns the previous selected entry. */
#define HISTORY_MAX_ENTRIES 32
#define INPUT_MAX_LENGTH 64

#define HISTORY_NONE_SELECTED_MARK (uint32_t)(-1)

typedef enum {
    KEY_UP = 512,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,

    KEY_ESC = '\x1b',
    KEY_ENTER = '\x0d',
    KEY_DEL = '\x7f',
} KeyCode;

typedef enum {
    /* The list is empty*/
    HIST_EMPTY,
    /* Reached the end of the list */
    HIST_END,
    /* There are no more valid entries */
    HIST_INVALID,
    /* This was the last entry */
    HIST_LAST_ENTRY,
    /* This was the first history request */
    HIST_OK_FIRST_SELECTION,
    /* Everything is fine. Got next/previous entry */
    HIST_OK,
} HistoryStatus;

typedef struct {
    char buffer[INPUT_MAX_LENGTH];
    uint32_t length;
} InputEntry;

static struct {
    InputEntry primary_entry;  /* This entry is directly modified by 'input_*()' */
    InputEntry primary_backup; /* A backup of the primary entry */
} input_state = {0};

static struct {
    uint32_t selected_entry_index;
    uint32_t newest_entry_index;
    InputEntry history[HISTORY_MAX_ENTRIES];
} history_state = {
    .selected_entry_index = HISTORY_NONE_SELECTED_MARK,
    .newest_entry_index = HISTORY_NONE_SELECTED_MARK,
};

/* Avoid adding empty entries */
static uint8_t entry_is_valid(InputEntry *entry) {
    if (entry->length <= 0)
        return 1;

    int is_valid = 0;
    uint32_t index = 0;
    char *buffer = entry->buffer;

    while ((is_valid = isblank(*buffer++)) && index++ < entry->length) {
        if (is_valid == 0)
            return 1;
    }

    return 0;
}

void history_add_entry(InputEntry *entry) {
    assert(entry != NULL);

    /* The first item to be added to the list should be at index 0 */
    if (history_state.newest_entry_index == HISTORY_NONE_SELECTED_MARK) {
        history_state.newest_entry_index = 0;
    } else {
        history_state.newest_entry_index =
            (history_state.newest_entry_index + 1) % HISTORY_MAX_ENTRIES;
    }

    InputEntry *top_entry = &history_state.history[history_state.newest_entry_index];
    memcpy(top_entry, entry, sizeof(*entry));

    /* Reset the history selection index */
    history_state.selected_entry_index = HISTORY_NONE_SELECTED_MARK;
}

/* Get the next entry from the history buffer and places it into 'entry'.
 * See 'HistoryStatus' for an explanation of the return values. */
HistoryStatus history_get_next_entry(InputEntry **entry) {
    uint32_t index = history_state.selected_entry_index;

    /* The list is empty */
    if (history_state.newest_entry_index == HISTORY_NONE_SELECTED_MARK) {
        return HIST_EMPTY;
    }

    /* This is the first selection. Return the newest entry */
    if (index == HISTORY_NONE_SELECTED_MARK) {
        history_state.selected_entry_index = history_state.newest_entry_index;
        *entry = &history_state.history[history_state.selected_entry_index];
        return HIST_OK_FIRST_SELECTION;
    }

    /* Wrap around */
    if (index == 0) {
        index = HISTORY_MAX_ENTRIES;
    } else {
        index -= 1;
    }

    if (history_state.history[index].length == 0)
        return HIST_INVALID;

    if (index == history_state.newest_entry_index)
        return HIST_END;

    history_state.selected_entry_index = index;
    *entry = &history_state.history[history_state.selected_entry_index];

    return HIST_OK;
}

/* Get the previous entry from the history buffer. Return NULL if we have reached
 * the newest entry. */
HistoryStatus history_get_previous_entry(InputEntry **entry) {
    uint32_t index = history_state.selected_entry_index;

    /* List is empty */
    if (history_state.newest_entry_index == HISTORY_NONE_SELECTED_MARK)
        return HIST_EMPTY;

    if (index == HISTORY_NONE_SELECTED_MARK) {
        return HIST_LAST_ENTRY;
    }

    /* Wrap around */
    if (index >= HISTORY_MAX_ENTRIES - 1) {
        index = 0;
    } else {
        index += 1;
    }

    /* There are no more valid entries */
    if (history_state.history[index].length == 0)
        return HIST_INVALID;

    /* Reached the end of the list */
    if (index == history_state.newest_entry_index)
        return HIST_END;

    history_state.selected_entry_index = index;
    *entry = &history_state.history[index];
    return HIST_OK;
}

void input_save_primary(void) {
    memcpy(&input_state.primary_backup, &input_state.primary_entry,
           sizeof(input_state.primary_entry));
}

/* Overwrite the primary_entry with the provided entry. */
void input_overwrite_primary(InputEntry *entry) {
    memcpy(&input_state.primary_entry, entry, sizeof(*entry));
}

void input_restore_primary(void) {
    if (history_state.selected_entry_index == history_state.newest_entry_index) {
        if (input_state.primary_backup.length != 0) {
            memcpy(&input_state.primary_entry, &input_state.primary_backup,
                   sizeof(input_state.primary_backup));
            memset(&input_state.primary_backup, 0, sizeof(input_state.primary_backup));
        }
    }
}

void input_clear(void) { memset(&input_state.primary_entry, 0, sizeof(input_state.primary_entry)); }

/* Add a character to the primary input buffer.
 * NOTE: This function does nothing if the max input length has
 * been reached. */
void input_add_char(char c) {
    InputEntry *entry = &input_state.primary_entry;

    if (entry->length >= INPUT_MAX_LENGTH)
        return;

    entry->buffer[entry->length++] = c;
}

/* Remove a character from the primary input buffer.
 * NOTE: This function does nothing if there are no
 * characters left. */
void input_delete_char(void) {
    InputEntry *entry = &input_state.primary_entry;

    if (entry->length <= 0)
        return;

    entry->buffer[--entry->length] = '\0';
}

void cmdline_clear(void) {
    char *clear_sequence = "\r\x1b[2K";
    write(STDOUT_FILENO, clear_sequence, strlen(clear_sequence));
}

void cmdline_render_entry(InputEntry *entry) {
    cmdline_clear();

    if (entry != NULL) {
        write(STDOUT_FILENO, entry->buffer, entry->length);
    }
}

KeyCode cmdline_read_keycode(void) {
    char c = 0;
    uint32_t nread = 0;

    if ((nread = read(STDIN_FILENO, &c, 1) == -1) && errno != EAGAIN) {
        fprintf(stderr, "Failed to read form stdin\r\n");
        terminate(1);
    }

    if (c == '\x1b') {
        char buffer[3];

        // FIXME: We are reading in data in intervals of 100ms, which might
        //        leads to noticeable delays when just hitting ESC to terminate
        //        the program
        // FIXME: Just returning KEY_ESC means that any multy byte keycode that
        //        is not
        //        explicitly handled will terminate the program
        if ((nread = read(STDIN_FILENO, buffer, 1) != 1))
            return KEY_ESC; /* ESC */
        if ((nread = read(STDIN_FILENO, buffer + 1, 1) != 1))
            return KEY_ESC; /* ESC */

        if (buffer[0] == '[') {
            switch (buffer[1]) {
            case 'A': /* UP */
                return KEY_UP;
            case 'B': /* DOWN */
                return KEY_DOWN;
            case 'C': /* DOWN */
                return KEY_LEFT;
            case 'D': /* DOWN */
                return KEY_RIGHT;
            }
        }
    }

    return c;
}

void cmdline_update(unsigned char *is_running) {
    InputEntry *current_entry = &input_state.primary_entry;
    HistoryStatus hist_status = HIST_OK;
    uint8_t input_changed = 0;

    KeyCode c = cmdline_read_keycode();

    // TODO: Clean this mess...
    switch (c) {
    case KEY_UP:
        hist_status = history_get_next_entry(&current_entry);
        switch (hist_status) {
        case HIST_EMPTY:
            break;
        case HIST_END:
            break;
        case HIST_INVALID:
            break;
        case HIST_LAST_ENTRY:
            break;
        case HIST_OK_FIRST_SELECTION:
            input_save_primary();
        case HIST_OK:
            input_overwrite_primary(current_entry);
            input_changed = 1;
            break;
        }

        break;
    case KEY_DOWN:
        hist_status = history_get_previous_entry(&current_entry);
        switch (hist_status) {
        case HIST_EMPTY:
            break;
        case HIST_END:
            break;
        case HIST_INVALID:
            break;
        case HIST_OK_FIRST_SELECTION:
            break;
        case HIST_LAST_ENTRY:
            break;
            input_restore_primary();
            input_changed = 1;
            break;
        case HIST_OK:
            input_overwrite_primary(current_entry);
            input_changed = 1;
            break;
        }
        break;
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_ESC:
        break;
    case KEY_ENTER:
        puts("ENTER\r");
        if (entry_is_valid(current_entry) == 0) {
            history_add_entry(current_entry);
        }

        input_clear();
        input_changed = 1;
        /* Parse input */
        break;
    case KEY_DEL:
        input_delete_char();
        input_changed = 1;
        break;
    }

    if (c == KEY_ESC) {
        *is_running = 0;
        return;
    }

    if (c >= 0x20 && c < 0x7f) {
        input_add_char(c);
        input_changed = 1;
    }

    if (input_changed == 1) {
        cmdline_render_entry(current_entry);
    }
}

int main(void) {
    uint8_t is_running = 1;

    terminal_enable_raw_mode();

    signal(SIGINT, handle_sigint);

    // TODO: Figure out a proper way to debug this thing
    char *input = "one\r\ntwo\r\nthree";
    write(STDIN_FILENO, input, strlen(input));

    while (is_running) {
        cmdline_update(&is_running);
    }

    terminal_disable_raw_mode();

    return 0;
}
