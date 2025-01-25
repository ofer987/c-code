#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <stdio.h>
#include <math.h>

const size_t SCREEN_SIZE = 289;
const size_t SIDE_SIZE = 17;

enum screen_state {
  AVAILABLE = 1,
  USED_BY_SNAKE_TAIL,
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
  struct coordinates *head;
  size_t list_size;

  enum snake_movement current_movement;
};

struct snake_screen {
  size_t size;
  size_t screen_state[SCREEN_SIZE];
};

static void finish(int sig);

void init_screen(
    enum screen_state *screen_states,
    struct snake_struct *snake,
    struct coordinates *food_location);

void render_screen(enum screen_state *screen_states);
bool generate_food(
    enum screen_state *screen_states,
    struct coordinates *food_location);
bool move_snake(
    enum screen_state *screen_states,
    struct snake_struct *snake,
    struct coordinates *food_location);
bool is_game_valid(
    enum screen_state *screen_states,
    struct snake_struct *snake);
struct coordinates index_to_coorindates(size_t index);
size_t coordinates_to_index(size_t y, size_t x);
struct coordinates* add_head(
    struct snake_struct *snake,
    struct coordinates *head);
void display_snake_coordinates(struct snake_struct *snake);
char* itoa(size_t value);

int main(int argc, char *argv[]) {
  srandom(time(NULL));

  enum screen_state screen_states[SCREEN_SIZE];

  struct coordinates head = {
    .y = SIDE_SIZE / 2,
    .x = SIDE_SIZE / 2,
  };
  struct snake_struct snake = {
    .head = &head,
    .list_size = 1
  };

  // Reset the screen
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    if (coordinates_to_index(snake.head->y, snake.head->x) == i) {
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

    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_CYAN,  COLOR_CYAN);
    init_pair(3, COLOR_RED,   COLOR_RED);
    init_pair(4, COLOR_BLUE,  COLOR_BLUE);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);
  }

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

    if (move_snake(screen_states, &snake, &food_location)) {
      generate_food(screen_states, &food_location);
    }
    // display_snake_coordinates(&snake);
    refresh();
    init_screen(screen_states, &snake, &food_location);
    render_screen(screen_states);
    /* attrset(COLOR_PAIR(5)); */

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

  struct coordinates *snake_location = snake->head;
  size_t snake_colour = USED_BY_SNAKE_HEAD;
  while (snake_location != NULL) {
    size_t index = coordinates_to_index(snake_location->y, snake_location->x);
    screen_states[index] = snake_colour;
    snake_colour = USED_BY_SNAKE_TAIL;

    snake_location = snake_location->next;
  }

  size_t index = coordinates_to_index(food_location->y, food_location->x);
  screen_states[index] = USED_BY_FOOD;
}

void render_screen(enum screen_state *screen_states) {
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    size_t state = screen_states[i];
    attrset(COLOR_PAIR(state));

    struct coordinates coords = index_to_coorindates(i);
    mvaddch(coords.y, coords.x, state + 48);
  }
}

bool generate_food(
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

  return true;
}

bool move_snake(
    enum screen_state *screen_states,
    struct snake_struct *snake,
    struct coordinates *food_location) {
  struct coordinates new_head;

  if (snake->current_movement == LEFT) {
    new_head.y = snake->head->y;
    new_head.x = snake->head->x - 1;
  } else if (snake->current_movement == UP) {
    new_head.y = snake->head->y - 1;
    new_head.x = snake->head->x;
  } else if (snake->current_movement == RIGHT) {
    new_head.y = snake->head->y;
    new_head.x = snake->head->x + 1;
  } else {
    new_head.y = snake->head->y + 1;
    new_head.x = snake->head->x;
  }

  if (new_head.y == food_location->y &&
      new_head.x == food_location->x) {
    struct coordinates *added_head = malloc(sizeof(struct coordinates));
    added_head->y = food_location->y;
    added_head->x = food_location->x;

    add_head(snake, added_head);

    return true;
  }
  size_t previous_y = new_head.y;
  size_t previous_x = new_head.x;
  struct coordinates *current = snake->head;
  do {
    size_t temp_y = current->y;
    size_t temp_x = current->x;

    current->y = previous_y;
    current->x = previous_x;

    previous_y = temp_y;
    previous_x = temp_x;

    current = current->next;
  } while (current != NULL);

  return false;
}

bool is_game_valid(
    enum screen_state *screen_states,
    struct snake_struct *snake) {
  if (snake->head->y < 0 || snake->head->y >= SIDE_SIZE) {
    return false;
  }

  if (snake->head->x < 0 || snake->head->x >= SIDE_SIZE) {
    return false;
  }

  size_t snake_head_location = coordinates_to_index(
      snake->head->y,
      snake->head->x);
  enum screen_state snake_location = screen_states[snake_head_location];
  if (snake_location == USED_BY_SNAKE_TAIL) {
    return false;
  }

  return true;
}

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
  bool new_head = false;

  if (snake->current_movement == LEFT
      && snake->head->y == food_location->y
      && snake->head->x + 1 == food_location->x) {
    new_head = true;
  } else if (snake->current_movement == UP
      && snake->head->y - 1 == food_location->y
      && snake->head->x == food_location->x) {
    new_head = true;
  } else if (snake->current_movement == RIGHT
      && snake->head->y == food_location->y
      && snake->head->x - 1 == food_location->x) {
    new_head = true;
  } else if (snake->current_movement == DOWN
      && snake->head->y + 1 == food_location->y
      && snake->head->x == food_location->x) {
    new_head = true;
  }

  if (new_head) {
    struct coordinates *new_head = malloc(sizeof(struct coordinates));
    new_head->y = food_location->y;
    new_head->x = food_location->x;
    new_head->next = NULL;

    add_head(snake, new_head);

    return true;
  }

  return false;
}

struct coordinates* add_head(
    struct snake_struct *snake,
    struct coordinates *head) {
  struct coordinates *temp = snake->head;
  snake->head = head;
  head->next = temp;

  snake->list_size += 1;

  return head;
}

void display_snake_coordinates(struct snake_struct *snake) {
  struct coordinates *current = snake->head;

  size_t height = 0;
  do {
    char* y_coordinate = itoa(current->y);
    char* x_coordinate = itoa(current->x);
    mvaddstr(31 + height, 0, "y: ");
    mvaddstr(31 + height, 4, y_coordinate);

    mvaddstr(31 + height, 10, "x: ");
    mvaddstr(31 + height, 14, x_coordinate);
    current = current->next;

    height += 1;
  } while (current != NULL);
}

char* itoa(size_t value) {
  size_t length = 0;

  size_t copy_of_value = value;
  while (copy_of_value) {
    length += 1;

    copy_of_value /= 10;
  }

  char *result = malloc((length + 1) * sizeof(char));
  size_t remainder = value;
  for (size_t i = length; i >= 1; i -= 1) {
    double_t division = pow(10, i - 1);

    size_t quotient = remainder / (size_t)division;
    remainder = remainder % (size_t)division;
    char ch = quotient + 48;
    result[length - i] = ch;
  }

  result[length] = '\0';

  return result;
}
