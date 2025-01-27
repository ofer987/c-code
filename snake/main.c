#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#define handle_error_en(en, msg) \
  do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

/* Used as argument to thread_start() */
struct thread_info {
  pthread_t           thread_id;    /* ID returned by pthread_create() */
  size_t              thread_num;   /* Application-defined thread # */
  char                *argv_string; /* From command-line argument */
  struct snake_struct *snake;
};

const size_t SIDE_SIZE = 17;
const size_t SCREEN_SIZE = SIDE_SIZE * SIDE_SIZE;

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
  bool   quit;

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
static void* keyboard_input(void *arg);

pthread_cond_t keyboard_thread_started = PTHREAD_COND_INITIALIZER;
pthread_mutex_t render_lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
  srandom(time(NULL));

  enum screen_state screen_states[SCREEN_SIZE];

  struct coordinates head = {
    .y = SIDE_SIZE / 2,
    .x = SIDE_SIZE / 2,
  };
  struct snake_struct snake = {
    .head = &head,
    .list_size = 1,
    .current_movement = RIGHT,
    .quit = false
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

  /* curs_threads */
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
  printf("here2\n");

  // Create pthreads to capture keyboard input
  int                 s;
  pthread_attr_t      attr;
  printf("here2\n");
  struct thread_info *tinfo;

  s = pthread_attr_init(&attr);
  if (s != 0) {
    handle_error_en(s, "pthread_attr_init");
  }

  tinfo = calloc(1, sizeof(*tinfo));
  if (tinfo == NULL) {
    handle_error("calloc");
  }
  printf("here2\n");

  tinfo[0].thread_num = 1;
  tinfo[0].snake = &snake;
  s = pthread_create(&tinfo->thread_id,
      &attr,
      &keyboard_input,
      &tinfo[0]);
  if (s != 0) {
    handle_error_en(s, "pthread_create");
  }
  s = pthread_attr_destroy(&attr);
  if (s != 0) {
    handle_error_en(s, "pthread_attr_destroy");
  }

  printf("here1\n");

  pthread_mutex_lock(&render_lock);
  pthread_cond_wait(&keyboard_thread_started, &render_lock);
  refresh();
  init_screen(screen_states, &snake, &food_location);
  render_screen(screen_states);
  for (;;) {
    clock_t start_time = clock();
    clock_t difference;
    do {
      clock_t end_time = clock();

      difference = end_time - start_time;
    } while (difference < 100000);

    if (move_snake(screen_states, &snake, &food_location)) {
      generate_food(screen_states, &food_location);
    }

    refresh();
    init_screen(screen_states, &snake, &food_location);
    render_screen(screen_states);

    if (!is_game_valid(screen_states, &snake)) {
      finish(0);
    }

    if (snake.quit) {
      finish(0);
    }
  }

  finish(0);
}

static void finish(int sig) {
  endwin();

  /* void *res; */
  /* int s = pthread_join(tinfo->thread_id, &res); */
  /* if (s != 0) { */
  /*   handle_error_en(s, "pthread_join"); */
  /* } */
  /* free(res); */
  /* res = NULL; */
  /* tinfo = NULL; */

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
    mvaddch(coords.y, coords.x, ' ');
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

  exit(value);
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

static void* keyboard_input(void *arg) {
  struct thread_info *info = arg;
  struct snake_struct *snake = info->snake;

  if (snake == NULL) {
    exit(EXIT_FAILURE);
  }

  pthread_cond_signal(&keyboard_thread_started);

  timeout(200);
  for (;;) {
    size_t ch = getch();     /* refresh, accept single keystroke of input */
    switch (ch) {
      case 'q':
        snake->quit = true;

        break;
      case KEY_LEFT:
      case 'h':
        if (snake->current_movement != RIGHT) {
          snake->current_movement = LEFT;
        }

        break;
      case KEY_UP:
      case 'k':
        if (snake->current_movement == DOWN) {
          snake->current_movement = UP;
        }

        break;
      case KEY_RIGHT:
      case 'l':
        if (snake->current_movement == LEFT) {
          snake->current_movement = RIGHT;
        }

        break;
      case KEY_DOWN:
      case 'j':
        if (snake->current_movement == UP) {
          snake->current_movement = DOWN;
        }

        break;
    }
  }

  return NULL;
}

