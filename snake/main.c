#include <stdlib.h>
#include <curses.h>
#include <signal.h>

static void finish(int sig);

int main(int argc, char *argv[]) {
  int num = 0;

  /* initialize your non-curses data structures here */
  (void) signal(SIGINT, finish);
  (void) signal(SIGTERM, finish);

  /* initialize the curses library */
  (void) initscr();

  /* enable keyboard mapping */
  keypad(stdscr, TRUE);

  /* tell curses not to do NL->CR/NL on output */
  (void) nonl();

  /* take input chars one at a time, no wait for \n */
  (void) cbreak();

  /* echo input - in color */
  (void) echo();

  if (has_colors()) {
    start_color();

    /*
     * Simple color assignment, often all we need.  Color pair 0 cannot
     * be redefined.  This example uses the same value for the color
     * pair as for the foreground color, though of course that is not
     * necessary:
     */
    use_default_colors();
    init_pair(1, COLOR_RED,     COLOR_WHITE);
    init_pair(2, COLOR_GREEN,   COLOR_RED);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_CYAN,    COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);
  }

  for (;;) {
    char ch = getch();     /* refresh, accept single keystroke of input */
    attrset(COLOR_PAIR(num % 8));
    num += 1;

    if (ch == 'q') {
      finish(0);
    }
  }

  finish(0);
}

static void finish(int sig) {
  endwin();

  if (sig == 15) {
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
