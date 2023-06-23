#include <ncurses.h>
#include <stdlib.h>

#define LINES_PER_PAGE 30
#define TOTAL_LINES 100

void print_scrollable_text(WINDOW *win, const char **text, int num_lines) {
    WINDOW *pad;
    int pad_line = 0;

    // Create pad
    pad = newpad(num_lines, COLS - 2);

    // Add lines to the pad
    for (int i = 0; i < num_lines; i++) {
        mvwprintw(pad, pad_line++, 0, "%s", text[i]);
    }

    int pad_top = 0;
    int ch;

    // Initial display of the pad
    prefresh(pad, pad_top, 0, 1, 1, LINES_PER_PAGE, COLS - 2);
    
    keypad(pad, TRUE);  // Enable function keys on our pad window

    while (1) {
        ch = wgetch(pad);  // Read from pad
        switch (ch) {
            case KEY_UP:
                if (pad_top > 0) {
                    pad_top--;
                }
                break;
            case KEY_DOWN:
                if (pad_top < num_lines - LINES_PER_PAGE) {
                    pad_top++;
                }
                break;
            case 'q':
                // Exit the loop on 'q'
                delwin(pad);
                return;
        }

        prefresh(pad, pad_top, 0, 1, 1, LINES_PER_PAGE, COLS - 2);
    }
}

int main() {
    initscr();
    noecho();
    keypad(stdscr, TRUE);

    const char **text = malloc(sizeof(char *) * TOTAL_LINES);

    for (int i = 0; i < TOTAL_LINES; i++) {
        text[i] = malloc(80 * sizeof(char));
        sprintf(text[i], "This is line %d of the scrollable text", i + 1);
    }

    print_scrollable_text(stdscr, text, TOTAL_LINES);

    for (int i = 0; i < TOTAL_LINES; i++) {
        free(text[i]);
    }
    free(text);

    endwin();

    return 0;
}
