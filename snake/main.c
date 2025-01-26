#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
/* #include <signal.h> */

const size_t SCREEN_SIZE = 289;
const size_t SIDE_SIZE = 17;

enum screen_state {
  AVAILABLE = 1,
  USED_BY_SNAKE,
  USED_BY_SNAKE_HEAD,
  USED_BY_FOOD
};
enum snake_movement { LEFT = 0, UP, RIGHT, DOWN };

struct coordinates {
  size_t y;
  size_t x;

  struct coordinates* next;
};

struct coordinates coordinates;

struct snake_struct {
  struct coordinates head;
  /* struct coordinates tail[SCREEN_SIZE]; */
  /* struct coordinates list; */
  size_t list_size;
  /* size_t tail_size; */

  enum snake_movement current_movement;
  /* size_t *locations; */
};

struct snake_screen {
  size_t size;
  size_t screen_state[SCREEN_SIZE];
};

/* struct screen_struct screen_struct; */
/* struct snake_struct snake_struct; */

static void finish(int sig);

/* void update_screen_state( */
/*     enum screen_state *screen_states, */
/*     struct snake_struct *snake); */

void init_screen(
    enum screen_state *screen_states,
    struct snake_struct *snake,
    struct coordinates *food_location);

void render_screen(enum screen_state *screen_states);
void generate_food(
    enum screen_state *screen_states,
    struct coordinates *food_location);
void move_snake(
    enum screen_state *screen_states,
    struct snake_struct *snake);
bool is_game_valid(
    enum screen_state *screen_states,
    struct snake_struct *snake);
struct coordinates index_to_coorindates(size_t index);
size_t coordinates_to_index(size_t y, size_t x);
bool eat_food(struct snake_struct *snake, struct coordinates *food_location);
struct coordinates* add_head(
    struct snake_struct *snake,
    struct coordinates *head);

int main(int argc, char *argv[]) {
  int num = 0;
  srandom(time(NULL));

  enum screen_state screen_states[SCREEN_SIZE];

  struct snake_struct snake = {
    .head = {
      .y = SIDE_SIZE / 2,
      .x = SIDE_SIZE / 2,
      .next = NULL
    },
    .list_size = 1
  };

  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    // Reset the tail to 0
    /* snake.tail_size = 0; */
    /* struct coordinates foobar = { */
    /*   .y = -1, */
    /*   .x = -1 */
    /* }; */
    /* if (i == 0) { */
    /*   foobar.y = SIDE_SIZE / 2; */
    /*   foobar.x = SIDE_SIZE / 2; */
    /* } */
    /* snake.head[i] = foobar; */
    /* snake.tail[i] = { */
    /*   .y = 0, */
    /*   .x = 0 */
    /* }; */

    // Reset the screen
    if (coordinates_to_index(snake.head.y, snake.head.x) == i) {
      screen_states[i] = USED_BY_SNAKE_HEAD;

      continue;
    }

    screen_states[i] = AVAILABLE;
  }
  struct coordinates food_location;
  generate_food(screen_states, &food_location);

  /* initialize your non-curses data structures here */
  (void) signal(SIGINT, finish);
  (void) signal(SIGTERM, finish);

  /* initialize the curses library */
  initscr();

  /* enable keyboard mapping */
  keypad(stdscr, TRUE);

  /* tell curses not to do NL->CR/NL on output */
  (void) nonl();

  /* take input chars one at a time, no wait for \n */
  (void) cbreak();

  /* noecho input - in color */
  (void) noecho();
  /* (void) echo(); */

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
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_WHITE, COLOR_WHITE);
    init_pair(3, COLOR_RED,   COLOR_RED);
    init_pair(4, COLOR_BLUE,  COLOR_BLUE);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);
    /* init_pair(5, COLOR_BLUE, COLOR_BLACK); */
    /* init_pair(6, COLOR_GREEN, COLOR_BLACK); */
    /* init_pair(7, COLOR_BLACK, COLOR_BLACK); */
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
      default:
        snake.current_movement = DOWN;

        break;
    }

    attrset(COLOR_PAIR(num % 8));
    num += 1;

    init_screen(screen_states, &snake, &food_location);
    /* update_screen_state(screen_states, &snake); */
    render_screen(screen_states);
    attrset(COLOR_PAIR(5));
    mvaddch(25, 25, snake.list_size + 48);
    refresh();
    move_snake(screen_states, &snake);

    if (eat_food(&snake, &food_location)) {
      generate_food(screen_states, &food_location);
    }

    if (!is_game_valid(screen_states, &snake)) {
      finish(0);
    }

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

void init_screen(
    enum screen_state screen_states[SCREEN_SIZE],
    struct snake_struct *snake,
    struct coordinates *food_location) {

  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    screen_states[i] = AVAILABLE;
  }

  bool is_head = true;
  struct coordinates *snake_location = &snake->head;
  while (snake_location != NULL) {
    size_t index = coordinates_to_index(snake_location->y, snake_location->x);
    if (is_head) {
      screen_states[index] = USED_BY_SNAKE_HEAD;
    } else {
      screen_states[index] = USED_BY_SNAKE;
    }
  }

  size_t index = coordinates_to_index(food_location->y, food_location->x);
  screen_states[index] = USED_BY_FOOD;
}

/* void update_screen_state( */
/*     enum screen_state *screen_states, */
/*     struct snake_struct *snake) { */
/*   // TODO(ofer987): fix later */
/*   // Should be based on the **relative location** and */
/*   // the snake's **current_movement** */
/*   size_t snake_head_location = coordinates_to_index( */
/*       snake->head.y, */
/*       snake->head.x); */
/*   if (screen_states[snake_head_location] == USED_BY_FOOD) { */
/*     #<{(| snake->tail_size += 1; |)}># */
/*     snake->list_size += 1; */
/*     generate_food(screen_states); */
/*  */
/*     return; */
/*   } */
/*  */
/*   #<{(| snake->tail_size += 1; |)}># */
/*   #<{(| for (size_t i = 0; i < snake->tail_size; i += 1) { |)}># */
/*   #<{(|   size_t snake_tail_index = coordinates_to_index( |)}># */
/*   #<{(|       snake->tail[i].y, |)}># */
/*   #<{(|       snake->tail[i].x); |)}># */
/*   #<{(|  |)}># */
/*   #<{(|   screen_states[snake_tail_index] = USED_BY_SNAKE; |)}># */
/*   #<{(| } |)}># */
/*   #<{(| screen_states[snake_head_location] = USED_BY_SNAKE_HEAD; |)}># */
/*  */
/*   return; */
/* } */

// TODO(ofer987): should I use an array instead?
void render_screen(enum screen_state *screen_states) {
  /* mvaddstr(100, 50, "foobar"); */
  /* mvaddch(100, 50, snake.tail_size + 48); */
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    size_t state = screen_states[i];
    attrset(COLOR_PAIR(state));
    /* mvaddstr(25, 25, "foobar"); */
    /* attr_set(a, c, o); */

    struct coordinates coords = index_to_coorindates(i);
    /* attron(COLOR_PAIR(screen_states[i % 4])); */
    mvaddch(coords.y, coords.x, state + 48);
    /* sleep(1); */
    /* mvchgat(y, x, 0, COLOR_PAIR(screen_states[i % 4]), 0, NULL); */
    /* move(y, x); */
    /* bkgd(i % 4); */
    /* addch('0'); */
    /* attroff(COLOR_PAIR(screen_states[i % 4])); */

    /* if (screen_states[i % 4] == AVAILABLE) { */
    /*   addch('1'); */
    /* } */
    /*  */
    /* if (screen_states[i % 4] == USED_BY_FOOD) { */
    /*   addch('2'); */
    /* } */
    /* addch(i + 65); */
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

void generate_food(
    enum screen_state *screen_states,
    struct coordinates *food_location) {
  size_t available_tiles = 0;

  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    if (screen_states[i] == AVAILABLE) {
      available_tiles += 1;
    }
  }

  size_t random_number = random() / (RAND_MAX / available_tiles);
  struct coordinates result = index_to_coorindates(random_number);
  food_location->y = result.y;
  food_location->x = result.x;

  available_tiles = 0;
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    if (random_number == available_tiles) {
      screen_states[i] = USED_BY_FOOD;

      break;
    }

    if (screen_states[i] == AVAILABLE) {
      available_tiles += 1;
    }
  }
}

void move_snake(
    enum screen_state *screen_states,
    struct snake_struct *snake) {
  struct coordinates new_head;

  if (snake->current_movement == LEFT) {
    new_head.y = snake->head.y;
    new_head.x = snake->head.x - 1;
  } else if (snake->current_movement == UP) {
    new_head.y = snake->head.y - 1;
    new_head.x = snake->head.x;
  } else if (snake->current_movement == RIGHT) {
    new_head.y = snake->head.y;
    new_head.x = snake->head.x + 1;
  } else {
    new_head.y = snake->head.y + 1;
    new_head.x = snake->head.x;
  }

  /* snake->tail_size += 1; */

  /* struct coordinates *previous = &(snake->head); */
  /* struct coordinates *previous = &(snake->head); */
  /* struct coordinates *previous; */
  size_t index = coordinates_to_index(snake->head.y, snake->head.x);
  screen_states[index] = AVAILABLE;

  /* snake->head = new_head; */
  /* index = coordinates_to_index(snake->head.y, snake->head.x); */
  /* screen_states[index] = USED_BY_SNAKE_HEAD; */

  struct coordinates *previous = &new_head;
  struct coordinates *current = &snake->head;
  /* snake->head.y = new_head.y; */
  /* snake->head.x = new_head.x; */
  /* snake->head.next = current; */
  mvaddch(25, 25, snake->list_size + 48);

  /* snake->list[0] = snake->head; */
  /* snake->tail_size += 1; */
  do {
    current->y = previous->y;
    current->x = previous->x;

    previous = current;
    current = current->next;
  } while (current != NULL);

  return;
}

bool is_game_valid(
    enum screen_state *screen_states,
    struct snake_struct *snake) {
  if (snake->head.y < 0 || snake->head.y >= SIDE_SIZE) {
    return false;
  }

  if (snake->head.x < 0 || snake->head.x >= SIDE_SIZE) {
    return false;
  }

  size_t snake_head_location = coordinates_to_index(
      snake->head.y,
      snake->head.x);
  enum screen_state snake_location = screen_states[snake_head_location];
  if (snake_location == USED_BY_SNAKE) {
    return false;
  }

  return true;
}

// TODO(ofer987): Finish later
/* char* itoa(size_t value) { */
/*   if (value < 0) { */
/*     return NULL; */
/*   } */
/*  */
/*   for (size_t i = value; i >= 0; i /= 10) { */
/*     size_t chiffre = i % 10; */
/*  */
/*     char ch = chiffre + 48; */
/*   } */
/* } */

struct coordinates index_to_coorindates(size_t index) {
  struct coordinates foobar = {
    .y = index / SIDE_SIZE,
    .x = index % SIDE_SIZE
  };

  return foobar;
}

size_t coordinates_to_index(size_t y, size_t x) {
  return SIDE_SIZE * y + x;
}

bool eat_food(struct snake_struct *snake, struct coordinates *food_location) {
  size_t snake_head_index = coordinates_to_index(snake->head.y, snake->head.x);
  size_t food_location_index = coordinates_to_index(
      food_location->y,
      food_location->x);

  if (snake_head_index == food_location_index) {
    /* snake->list_size += 1; */
    /*  */
    struct coordinates new_head = {
      .y = food_location->y,
      .x = food_location->x,
      .next = NULL
    };
    add_head(snake, &new_head);

    return true;
  }

  return false;
}

struct coordinates* add_head(
    struct snake_struct *snake,
    struct coordinates *head) {

  /* struct coordinates *result = head; */
  head->next = &snake->head;
  snake->head = *head;

  snake->list_size += 1;

  return head;
}
