#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ESC "\033"

int ansi_enabled = 0;

void enable_ansi_escape_sequences() {
    if (isatty(fileno(stdout))) {
        ansi_enabled = 1;
    }
}

void move_cursor_left(int n) {
    if (ansi_enabled) {
        printf(ESC "[%dD", n);
    } else {
        for (int i = 0; i < n; i++) {
            printf("\b");
        }
    }
}

void move_cursor_right(int n) {
    if (ansi_enabled) {
        printf(ESC "[%dC", n);
    } else {
        for (int i = 0; i < n; i++) {
            printf(" ");
        }
        for (int i = 0; i < n; i++) {
            printf("\b");
        }
    }
}

int main() {
    enable_ansi_escape_sequences();

    char input[1024];
    int cursor_position = 0;

    while (1) {
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "left") == 0) {
            move_cursor_left(1);
            cursor_position--;
        } else if (strcmp(input, "right") == 0) {
            move_cursor_right(1);
            cursor_position++;
        }

        printf("Cursor position: %d\n", cursor_position);
    }

    return 0;
}