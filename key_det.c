#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <errno.h>
#include <ctype.h>

// Bitwise AND with 0x1f to clear the 5th and 6th bits
#define CTRL_KEY(k) ((k) & 0x1f)

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    char c;

    enable_raw_mode();

    while(1) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            switch (c) {
            case CTRL_KEY('q'):
                printf("Ctrl+Q detected!\n");
                return 0;
            case CTRL_KEY('p'):
                printf("Ctrl+P detected!\n");
                break;
            }
        }
    }

    disable_raw_mode();

    return 0;
}
