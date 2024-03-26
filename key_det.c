#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <errno.h>
#include <ctype.h>

#define CTRL_KEY(k) ((k) & 0x1f) // Bitwise AND with 0x1f to clear the 5th and 6th bits
#define CTRL_(k) ((k) & (0x1f))

typedef struct {
    int idx;
    int size;
    int render_size;
    char *chars;
    char *render;
    unsigned char *highlight;
    int hl_open_comment;
} editor_row;

typedef struct {
    char *file_type;
    char **file_match;
    char **keywords;
    char *sl_comment_start;                  /* single line */
    char *ml_comment_start, *ml_comment_end; /* multiple lines */
    int flags;
} editor_syntax;

/* clang-format off */
enum editor_key {
    BACKSPACE = 0x7f,
    ARROW_LEFT = 0x3e8, ARROW_RIGHT, ARROW_UP, ARROW_DOWN,
    PAGE_UP, PAGE_DOWN,
    HOME_KEY, END_KEY, DEL_KEY,
};

enum editor_highlight {
    NORMAL,     MATCH,
    SL_COMMENT, ML_COMMENT,
    KEYWORD_1,  KEYWORD_2,  KEYWORD_3,
    STRING,     NUMBER,
};

struct {
    int cursor_x, cursor_y, render_x;
    int row_offset, col_offset;
    int screen_rows, screen_cols;
    int num_rows;
    editor_row *row;
    int modified;
    char *file_name;
    char status_msg[80];
    time_t status_msg_time;
    char *copied_char_buffer;
    editor_syntax *syntax;
    struct termios orig_termios;
} ec = {
    /* editor config */
    .cursor_x = 0,         .cursor_y = 0,        .render_x = 0,
    .row_offset = 0,       .col_offset = 0,      .num_rows = 0,
    .row = NULL,           .modified = 0,        .file_name = NULL,
    .status_msg[0] = '\0', .status_msg_time = 0, .copied_char_buffer = NULL,
    .syntax = NULL,
};

// Function to enable raw mode
void enable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to disable raw mode
void disable_raw_mode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void clear_screen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

void panic(const char *s)
{
    clear_screen();
    perror(s);
    puts("\r\n");
    exit(1);
}

int read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if ((nread == -1) && (errno != EAGAIN))
            printf("Error reading input\n");
    }
    if (c == '\x1b') {
        char seq[3];
        if ((read(STDIN_FILENO, &seq[0], 1) != 1) ||
            (read(STDIN_FILENO, &seq[1], 1) != 1))
            return '\x1b';
        if (seq[0] == '[') {
            if (isdigit(seq[1])) {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '1':
                    case '7':
                        return HOME_KEY;
                    case '4':
                    case '8':
                        return END_KEY;
                    case '3':
                        return DEL_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    }
                }
            } else {
                switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }
        return '\x1b';
    }
    return c;
}

void open_buffer()
{
    if (write(STDOUT_FILENO, "\x1b[?47h", 6) == -1)
        panic("Error changing terminal buffer");
}

void disable_raw_mode1()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &ec.orig_termios) == -1)
        panic("Failed to disable raw mode");
}

void enable_raw_mode1()
{
    if (tcgetattr(STDIN_FILENO, &ec.orig_termios) == -1)
        printf("Failed to get current terminal state\n");
    atexit(disable_raw_mode);
    struct termios raw = ec.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    open_buffer();
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        panic("Failed to set raw mode");
}


void process_key()
{
    static int indent_level = 0;
    int c = read_key();
    switch (c) {
    case CTRL_('q'):
        printf("Ctrl+Q detected!\n");
        break;
    case CTRL_('p'):
        printf("Ctrl+P detected!\n");
        break;
    }
}

int main() {
    enable_raw_mode();

    //int fd = fcntl(STDIN_FILENO, F_GETFL);
    //fcntl(STDIN_FILENO, F_SETFL, fd | O_NONBLOCK);

    //while(1)
    //    process_key();

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == CTRL_KEY('p')) {
                printf("Ctrl+P detected!\n");
                break;
            } else {
                printf("Ctrl+Q detected!\n");
                break;
            }
        }
    }

    disable_raw_mode();

    return 0;
}
