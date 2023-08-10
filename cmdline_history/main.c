#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <termios.h>

#define ASSERT(condition)                                                                          \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "Assertion failed: " #condition "\r\n");                               \
            __builtin_debugtrap();                                                                 \
        }                                                                                          \
    } while (0)

/* This is a POF implementation for a simple "command line history system" */

/* The Issue:
 * By default, the terminal starts out in "cooked/cannonical mode". In this mode, keyboard
 * output is only send to the program when the emulator encounters a linebreak (pressing enter sends
 * a linebreak by default). However we want to perform actions without having to wait for the
 * terminal, so we can view a previous entry by pressing 'UP' or quit the program by hitting 'ESC'
 * This is where platform specific abstraction comes into play.
 *
 * It is up to the terminal emulator to decide, how it wants to process user input and expose it to
 * the runnig application. 'Alacritty' might send the following byte sequence when the user presses
 * the UP key: '^[[A', while 'st' could do things very differently. We would have to implement an
 * additonal layer of abstraction for most terminal emulators. An example: The fish shell handles
 * this task by relying on the curses library:
 * https://github.com/fish-shell/fish-shell/blob/master/src/input.cpp.
 * */

static struct termios term_attributes_default;

static void terminal_disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_attributes_default);
}

static void terminal_enable_raw_mode(void) {
    /* TODO: Figure out how to handle pipes */

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
    /* We are inversing the flags, and therefore doing
     * the opposite */

    /* ECHO     - Enable echo */
    /* ICANON   - Canonical input (erase and kill) */
    /* IEXTEN   - Extended input character (the thing we really care about) */
    new_config.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    /* IXON     - Enable flow control */
    /* INPCK    - Parity check (defunced error checking?) */
    /* ISTRIP   - Strip 8th bit of each input byte */
    /* BRKINT   - Ignore break condition (like Ctrl-C) */
    /* ICRNL    - Translate CR to newline */
    new_config.c_iflag &= ~(IXON | INPCK | ISTRIP | BRKINT | ICRNL);

    /* OPOST    - Post process output (newline being translatet */
    /*            into '\r\n' etc.) */
    new_config.c_oflag &= ~(OPOST);

    /* CS8      - Set character byte size. For us, a byte consists of 8 bits */
    new_config.c_cflag |= (CS8);

    /* Control characters
     * See: http://unixwiz.net/techtips/termios-vmin-vtime.html */
    /* VMIN     - minimun number of bytes to read */
    /* VTIME    - timeout (value * 100ms) */
    /* read() will block until 1 bytes has been read, or 100ms have passed. */
    new_config.c_cc[VMIN] = 1;
    new_config.c_cc[VTIME] = 1;

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

/* What is save to do inside a signal handler?
 * https://stackoverflow.com/questions/16891019/how-to-avoid-using-printf-in-a-signal-handler?noredirect=1&lq=1
 */
static void handle_exit_signal(int signal) { terminate(1); }

/* Ideas about how to handle command line history:
 * 1. When a line is submitted, it is added to a ring-buffer
 *      - Empy lines/only blanks are ignored
 *      - Submitting resets the currently selected entry
 * 2. Moving up/down returnes the next/previous valid entry
 *      - Typing will NOT overwrite the selected entry
 *      - Typed input will be saved when moving up the history
 *      - Moving all the way down again (beyond the newest entry)
 *        will restore saved input
 *
 * - Since the history is implemented as a ring buffer, it can only
 *   grow and is will subsequently overwrite previous entries.
 *
 * The history consists of three functions:
 * - history_add_entry(): Adds an entry onto the list,
 *   potentially overwriting previous entries.
 * - history_get_next_entry(): Returns the next selected entry.
 * - history_get_previous_entry(): Returns the previous selected entry. */

#define HISTORY_MAX_ENTRIES 4
#define INPUT_MAX_LENGTH 64

/* Since our intitial input is not part of the history list, we need an
 * indicator to signal that the list is empty or an index is unset. This allows
 * us to use the 0 index as just another entry, without assigning it any special
 * meaning */
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
    /* This was the last entry */
    HIST_LAST_ENTRY,
    /* This was the first history request */
    HIST_OK_FIRST_SELECTION,
    /* Everything is fine. Got next/previous entry */
    HIST_OK,
} HistoryStatus;

/* In an actual implementation, these variables might be passed to library functions as an opaque
 * pointer */
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

static struct {
    InputEntry submission;
} cmdline_state = {0};

/* Avoid adding entries that start with blanks */
static uint8_t entry_is_valid(InputEntry *entry) {
    if (entry->length == 0)
        return 1;

    int is_valid = 0;
    uint32_t index = 0;
    char *buffer = entry->buffer;

    while ((is_valid = !isblank(*buffer++)) && index++ < entry->length) {
        if (is_valid == 1)
            return 1;
    }

    return is_valid;
}

/* Add a new entry to the history list */
void history_add_entry(InputEntry *entry) {
    ASSERT(entry != NULL);

    /* The first item to be added to the list should be at index 0 */
    if (history_state.newest_entry_index == HISTORY_NONE_SELECTED_MARK) {
        history_state.newest_entry_index = 0;
    } else {
        history_state.newest_entry_index =
            (history_state.newest_entry_index + 1) % HISTORY_MAX_ENTRIES;
    }

    // TODO: Ignore duplicate entries
    InputEntry *top_entry = &history_state.history[history_state.newest_entry_index];
    memcpy(top_entry, entry, sizeof(*entry));

    /* Reset the history selection index */
    history_state.selected_entry_index = HISTORY_NONE_SELECTED_MARK;
}

/* Get the next entry from the history buffer and places it into 'entry'.
 * See 'HistoryStatus' for an explanation of the return values. */
HistoryStatus history_get_next_entry(InputEntry **entry) {
    ASSERT(entry != NULL);
    uint32_t index = history_state.selected_entry_index;

    /* The list is empty */
    if (history_state.newest_entry_index == HISTORY_NONE_SELECTED_MARK)
        return HIST_EMPTY;

    /* This is the first selection. Return the newest entry */
    if (index == HISTORY_NONE_SELECTED_MARK) {
        history_state.selected_entry_index = history_state.newest_entry_index;
        *entry = &history_state.history[history_state.selected_entry_index];
        return HIST_OK_FIRST_SELECTION;
    }

    /* Wrap around */
    if (index == 0) {
        index = HISTORY_MAX_ENTRIES - 1;
    } else {
        index -= 1;
    }

    if (index == history_state.newest_entry_index)
        return HIST_END;

    /* There are no more valid entries */
    if (history_state.history[index].length == 0)
        return HIST_END;

    history_state.selected_entry_index = index;
    *entry = &history_state.history[history_state.selected_entry_index];

    return HIST_OK;
}

/* Get the previous entry from the history buffer. */
HistoryStatus history_get_previous_entry(InputEntry **entry) {
    ASSERT(entry != NULL);
    uint32_t index = history_state.selected_entry_index;

    /* List is empty */
    if (history_state.newest_entry_index == HISTORY_NONE_SELECTED_MARK)
        return HIST_EMPTY;

    if (index == HISTORY_NONE_SELECTED_MARK)
        return HIST_LAST_ENTRY;

    /* Reached the end of the list. Restore entry and reset selection. */
    if (index == history_state.newest_entry_index) {
        history_state.selected_entry_index = HISTORY_NONE_SELECTED_MARK;
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
        return HIST_END;

    history_state.selected_entry_index = index;
    *entry = &history_state.history[index];
    return HIST_OK;
}

void input_save_primary(void) {
    memcpy(&input_state.primary_backup, &input_state.primary_entry,
           sizeof(input_state.primary_backup));
}

void input_overwrite_primary(InputEntry *entry) {
    ASSERT(entry != NULL);
    memcpy(&input_state.primary_entry, entry, sizeof(input_state.primary_entry));
}

void input_restore_primary(void) {
    memcpy(&input_state.primary_entry, &input_state.primary_backup,
           sizeof(input_state.primary_entry));
}

void input_clear(void) { memset(&input_state.primary_entry, 0, sizeof(input_state.primary_entry)); }

/* Add a character to the primary input buffer, until INPUT_MAX_LENGTH length has been reached */
void input_add_char(char c) {
    InputEntry *entry = &input_state.primary_entry;

    if (entry->length >= INPUT_MAX_LENGTH - 1)
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

static KeyCode cmdline_read_keycode(void) {
    char c = 0;
    uint32_t nread = 0;

    if ((nread = read(STDIN_FILENO, &c, 1) == -1) && errno != EAGAIN) {
        fprintf(stderr, "Failed to read form stdin\r\n");
        terminate(1);
    }

    if (c == '\x1b') {
        char buffer[3];

        // FIXME: Just returning KEY_ESC means that any multi byte keycode that
        //        is not explicitly handled will terminate the program
        //        if ((nread = read(STDIN_FILENO, buffer, 1) != 1))
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
            case 'C': /* LEFT */
                // TODO: Move the cursor around?
                return KEY_LEFT;
            case 'D': /* RIGHT */
                // TODO: Move the cursor around?
                return KEY_RIGHT;
            }
        }
    }

    return c;
}

static inline void cmdline_handle_key_up(InputEntry *current_entry, uint8_t *entry_modified) {
    ASSERT(current_entry != NULL);
    ASSERT(entry_modified != NULL);
    HistoryStatus hist_status = history_get_next_entry(&current_entry);

    switch (hist_status) {
    case HIST_EMPTY:
        // TODO
        break;
    case HIST_END:
        // TODO
        break;
    case HIST_LAST_ENTRY:
        // TODO
        break;
    case HIST_OK_FIRST_SELECTION:
        input_save_primary();
    case HIST_OK:
        input_overwrite_primary(current_entry);
        *entry_modified = 1;
        break;
    }
}

static inline void cmdline_handle_key_down(InputEntry *current_entry, uint8_t *entry_modified) {
    ASSERT(current_entry != NULL);
    ASSERT(entry_modified != NULL);
    HistoryStatus hist_status = history_get_previous_entry(&current_entry);

    switch (hist_status) {
    case HIST_EMPTY:
        break;
    case HIST_END:
        break;
    case HIST_OK_FIRST_SELECTION:
        break;
    case HIST_LAST_ENTRY:
        input_restore_primary();
        *entry_modified = 1;
        isalnum('s');
        break;
    case HIST_OK:
        input_overwrite_primary(current_entry);
        *entry_modified = 1;
        break;
    }
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

/* Read and process input */
void cmdline_update(unsigned char *is_running) {
    ASSERT(is_running != NULL);
    InputEntry *current_input = &input_state.primary_entry;
    uint8_t input_modified = 0;

    memset(&cmdline_state.submission, 0, sizeof(cmdline_state.submission));

    KeyCode c = cmdline_read_keycode();

    // TODO: Clean this mess...
    switch (c) {
    case KEY_UP:
        cmdline_handle_key_up(current_input, &input_modified);
        break;
    case KEY_DOWN:
        cmdline_handle_key_down(current_input, &input_modified);
        break;
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_ESC:
        *is_running = 0;
        return;
    case KEY_ENTER:
        if (entry_is_valid(current_input) == 1) {
            history_add_entry(current_input);
            memcpy(&cmdline_state.submission, &input_state.primary_entry,
                   sizeof(cmdline_state.submission));
        }

        input_clear();
        input_modified = 1;
        break;
    case KEY_DEL:
        input_delete_char();
        input_modified = 1;
        break;
    }

    if (c >= 0x20 && c < 0x7f) {
        input_add_char(c);
        input_modified = 1;
    }

    if (input_modified == 1) {
        cmdline_render_entry(current_input);
    }
}

uint8_t cmdline_has_submission(char **string, uint32_t *string_length) {
    if (cmdline_state.submission.length > 0) {
        *string = cmdline_state.submission.buffer;
        *string_length = cmdline_state.submission.length;

        return 1;
    }

    return 0;
}

#define DEBUG_HIST_ADD_ENTRY(value)                                                                \
    history_add_entry(&(InputEntry){.buffer = (value), .length = strlen((value))})

int main(void) {
    uint8_t is_running = 1;

    terminal_enable_raw_mode();

    signal(SIGINT, handle_exit_signal);
    signal(SIGABRT, handle_exit_signal);

    DEBUG_HIST_ADD_ENTRY("1. one");
    DEBUG_HIST_ADD_ENTRY("2. I am the oldest entry!");
    DEBUG_HIST_ADD_ENTRY("3. three");
    DEBUG_HIST_ADD_ENTRY("4. four");
    DEBUG_HIST_ADD_ENTRY("5. I am the newest, and I overwrote the first entry!");

    char *string;
    uint32_t string_length;

    while (is_running) {
        cmdline_update(&is_running);

        if (cmdline_has_submission(&string, &string_length)) {
            printf("Got submission: %s\r\n", string);
        }
    }

    terminal_disable_raw_mode();

    return 0;
}
