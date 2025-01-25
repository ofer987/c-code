#include <stdlib.h>
#include <curses.h>
#include <signal.h>

const size_t SCREEN_SIZE = 256;

enum screen_state {
  AVAILABLE = 0,
  USED_BY_SNAKE,
  USED_BY_SNAKE_HEAD,
  USED_BY_FOOD
};
enum snake_movement { LEFT = 0, UP, RIGHT, DOWN };

struct snake_struct {
  size_t head;
  size_t size;
  enum snake_movement current_movement;
  /* size_t *locations; */
};

struct screen_struct {
  size_t size;
  size_t screen_state[SCREEN_SIZE];
};

struct screen_struct screen_struct;
struct snake_struct snake_struct;

static void finish(int sig);

void update_screen_state(
    enum screen_state *screen_states,
    struct snake_struct *snake);

void render_screen(enum screen_state *screen_states);

int main(int argc, char *argv[]) {
  int num = 0;

  enum screen_state screen_states[SCREEN_SIZE];

  struct snake_struct snake = {
    .head = SCREEN_SIZE / 2,
    .size = 1
      /* .locations = screen[SCREEN_SIZE / 2] */
  };

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
    // Use default color for available space (i.e., `0`)
    init_pair(1, COLOR_WHITE, COLOR_WHITE);
    init_pair(2, COLOR_RED,   COLOR_RED);
    init_pair(3, COLOR_BLACK, COLOR_BLACK);
  }

  /* size_t screen_size[SCREEN_SIZE]; */

  for (;;) {
    size_t ch = getch();     /* refresh, accept single keystroke of input */
    switch (ch) {
      case KEY_LEFT:
      case 'h':
        snake.current_movement = LEFT;

        break;
      case KEY_UP:
      case 'k':
        snake.current_movement = UP;

        break;
      case KEY_RIGHT:
      case 'l':
        snake.current_movement = RIGHT;

        break;
      case KEY_DOWN:
      case 'j':
        snake.current_movement = DOWN;

        break;
    }

    attrset(COLOR_PAIR(num % 8));
    num += 1;

    update_screen_state(screen_states, &snake);
    render_screen(screen_states);

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

void update_screen_state(
    enum screen_state *screen_states,
    struct snake_struct *snake) {
  // TODO(ofer987): fix later
  // Should be based on the **relative location** and
  // the snake's **current_movement**
  if (screen_states[snake->head] != USED_BY_FOOD) {
    return;
  }

  snake->size += 1;
  screen_states[snake->head] = USED_BY_SNAKE;

  return;
}

// TODO(ofer987): should I use an array instead?
void render_screen(enum screen_state *screen_states) {
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    attrset(COLOR_PAIR(screen_states[i]));
    /* attr_set(COLOR_PAIR((int) (screen_states[i]))); */

    /* attr_g, cp, o); */
    /* switch (screen_states[i]) { */
    /*   case AVAILABLE: */
    /*     attrset(COLOR_PAIR(0)); */
    /*  */
    /*     break; */
    /*   case USED_BY_SNAKE: */
    /*     attrset(COLOR_PAIR(USED)); */
  }
  }
