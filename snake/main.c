#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#define TB_IMPL
#include "./termbox2.h"

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

#define SIDE_SIZE 30
#define SCREEN_SIZE 900

#define MESSAGE_BOARD_START_COLUMN  0
#define MESSAGE_BOARD_START_ROW     32

#define MESSAGE_BOARD_WIDTH         30
#define MESSAGE_BOARD_HEIGHT        11

#define BOARDER_ROW                 "-"
#define BOARDER_COLUMN              "|"
#define BOARDER_TOP_LEFT            "/"
#define BOARDER_TOP_RIGHT           "\\"
#define BOARDER_BOTTOM_LEFT         "\\"
#define BOARDER_BOTTOM_RIGHT        "/"

#define EMPTY_MESSAGE \
  "                                                                                "
#define MESSAGE_START_COLUMN 3
#define FIRST_MESSAGE_LINE 37
#define SECOND_MESSAGE_LINE 38

enum screen_state {
  AVAILABLE = 1,
  USED_BY_SNAKE_TAIL,
  USED_BY_SNAKE_HEAD,
  USED_BY_FOOD,
  USED_BY_BOARDER,
  USED_BY_MESSAGE,
  TOTAL
};

size_t screen_colors[TOTAL] = {
  [AVAILABLE] = TB_WHITE,
  [USED_BY_SNAKE_TAIL] = TB_GREEN,
  [USED_BY_SNAKE_HEAD] = TB_RED,
  [USED_BY_FOOD] = TB_BLUE,
  [USED_BY_BOARDER] = 9,
  [USED_BY_MESSAGE] = TB_WHITE
};

enum snake_movement { LEFT = 0, UP, RIGHT, DOWN };

struct coordinates {
  size_t x;
  size_t y;

  struct coordinates* next;
};

struct coordinates coordinates;

struct snake_struct {
  struct coordinates *head;
  size_t list_size;
  bool   quit;
  bool   pause;

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

void eat_food(struct snake_struct *snake, struct coordinates *food_location);
bool is_same_coordinates(
    struct coordinates *coord_01,
    struct coordinates *coord_02);
void render_screen(enum screen_state *screen_states);
bool generate_food(
    enum screen_state *screen_states,
    struct coordinates *food_location);
struct coordinates new_head_coordinates(struct snake_struct *snake);
void move_snake(struct coordinates *new_head, struct snake_struct *snake);
bool is_game_valid(
    enum screen_state *screen_states,
    struct coordinates *snake_head);
struct coordinates index_to_coorindates(size_t index);
size_t coordinates_to_index(size_t x, size_t y);
struct coordinates* add_head(
    struct snake_struct *snake,
    struct coordinates *head);
void display_snake_coordinates(struct snake_struct *snake);

static void render_the_message_box_boarders();
static void* keyboard_input(void *arg);

static void turn_left(struct snake_struct *snake);
static void turn_right(struct snake_struct *snake);
static void turn_up(struct snake_struct *snake);
static void turn_down(struct snake_struct *snake);
static void reinit_snake(struct snake_struct *snake);

static bool is_finished = false;
pthread_t keyboard_thread_id;
pthread_cond_t keyboard_thread_started_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t render_lock_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    if (coordinates_to_index(snake.head->x, snake.head->y) == i) {
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

  /* initialize the termbox library */
  if (tb_init() != 0) {
    exit(EXIT_FAILURE);
  }

  // Create pthreads to capture keyboard input
  int                 s;
  pthread_attr_t      attr;
  struct thread_info *tinfo;

  s = pthread_attr_init(&attr);
  if (s != 0) {
    handle_error_en(s, "pthread_attr_init");
  }

  tinfo = calloc(1, sizeof(*tinfo));
  if (tinfo == NULL) {
    handle_error("calloc");
  }

  tinfo[0].thread_num = 1;
  tinfo[0].snake = &snake;
  s = pthread_create(&keyboard_thread_id,
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

  for (;;) {
    usleep(100000);

    init_screen(screen_states, &snake, &food_location);
    render_screen(screen_states);

    if (snake.pause) {
      tb_printf(MESSAGE_START_COLUMN, FIRST_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "You have paused the game");
      tb_printf(MESSAGE_START_COLUMN, SECOND_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "Press any key to continue");
      render_screen(screen_states);

      // Remove the message
      tb_printf(MESSAGE_START_COLUMN, FIRST_MESSAGE_LINE, TB_DEFAULT, TB_DEFAULT, EMPTY_MESSAGE);
      tb_printf(MESSAGE_START_COLUMN, SECOND_MESSAGE_LINE, TB_DEFAULT, TB_DEFAULT, EMPTY_MESSAGE);

      continue;
    }

    // Get the snake's head's new coordinates
    struct coordinates new_head = new_head_coordinates(&snake);
    if (!is_game_valid(screen_states, &new_head)) {
      tb_printf(MESSAGE_START_COLUMN, FIRST_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "You have lost :(");
      tb_printf(MESSAGE_START_COLUMN, SECOND_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "Press any key to quit");
      render_screen(screen_states);

      finish(0);
    }

    // Eat the food; or
    // Move the snake
    if (is_same_coordinates(&new_head, &food_location)) {
      eat_food(&snake, &food_location);

      generate_food(screen_states, &food_location);
    } else {
      move_snake(&new_head, &snake);
    }

    if (snake.quit) {
      tb_printf(MESSAGE_START_COLUMN, FIRST_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "You have decided to leave");
      tb_printf(MESSAGE_START_COLUMN, SECOND_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "Press any key to quit");
      render_screen(screen_states);

      finish(0);
    }
  }

  finish(0);
}

static void finish(int sig) {
  is_finished = true;

  printf("signal is %d", sig);
  if (sig == SIGKILL) {
    exit(EXIT_FAILURE);
  }

  // terminate the keyboard_input thread
  pthread_join(keyboard_thread_id, NULL);
  pthread_cond_destroy(&keyboard_thread_started_cond);
  pthread_mutex_destroy(&render_lock_mutex);

  // shutdown termbox2
  tb_shutdown();

  exit(EXIT_SUCCESS);
}

void init_screen(
    enum screen_state *screen_states,
    struct snake_struct *snake,
    struct coordinates *food_location) {

  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    screen_states[i] = AVAILABLE;
  }

  struct coordinates *snake_location = snake->head;
  size_t snake_colour = USED_BY_SNAKE_HEAD;
  while (snake_location != NULL) {
    size_t index = coordinates_to_index(snake_location->x, snake_location->y);
    screen_states[index] = snake_colour;
    snake_colour = USED_BY_SNAKE_TAIL;

    snake_location = snake_location->next;
  }

  size_t index = coordinates_to_index(food_location->x, food_location->y);
  screen_states[index] = USED_BY_FOOD;
}

void render_screen(enum screen_state *screen_states) {
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    struct coordinates coords = index_to_coorindates(i);

    size_t state = screen_states[i];
    int32_t color = screen_colors[state];

    tb_printf(coords.x, coords.y, color, color, " ");
  }

  render_the_message_box_boarders();

  tb_present();
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

struct coordinates new_head_coordinates(struct snake_struct *snake) {
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

  return new_head;
}

void move_snake(struct coordinates *new_head, struct snake_struct *snake) {
  size_t previous_y = new_head->y;
  size_t previous_x = new_head->x;
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
}

bool is_game_valid(
    enum screen_state *screen_states,
    struct coordinates *snake_head) {
  if (snake_head->y < 0 || snake_head->y >= SIDE_SIZE) {
    return false;
  }

  if (snake_head->x < 0 || snake_head->x >= SIDE_SIZE) {
    return false;
  }

  size_t snake_head_location = coordinates_to_index(
      snake_head->x,
      snake_head->y);
  enum screen_state snake_location = screen_states[snake_head_location];
  if (snake_location == USED_BY_SNAKE_TAIL) {
    return false;
  }

  return true;
}

struct coordinates index_to_coorindates(size_t index) {
  struct coordinates foobar = {
    .x = index % SIDE_SIZE,
    .y = index / SIDE_SIZE
  };

  return foobar;
}

size_t coordinates_to_index(size_t x, size_t y) {
  return SIDE_SIZE * y + x;
}

void eat_food(struct snake_struct *snake, struct coordinates *food_location) {
  struct coordinates *new_head = malloc(sizeof(struct coordinates));
  new_head->y = food_location->y;
  new_head->x = food_location->x;
  new_head->next = NULL;

  add_head(snake, new_head);
}

bool is_same_coordinates(
    struct coordinates *coord_01,
    struct coordinates *coord_02) {
  return coord_01->x == coord_02->x && coord_01->y == coord_02->y;
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

static void render_the_message_box_boarders() {
  size_t color = screen_colors[USED_BY_BOARDER];

  // LEFT Boarder
  for (size_t y = MESSAGE_BOARD_START_ROW + 1; y < MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT; y += 1) {
    tb_printf(MESSAGE_BOARD_START_COLUMN, y, color, TB_DEFAULT, BOARDER_COLUMN);
  }

  // TOP Boarder
  for (size_t x = MESSAGE_BOARD_START_COLUMN + 1; x < MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH; x += 1) {
    tb_printf(x, MESSAGE_BOARD_START_ROW, color, TB_DEFAULT, BOARDER_ROW);
  }

  // RIGHT Boarder
  for (size_t y = MESSAGE_BOARD_START_ROW + 1; y < MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT; y += 1) {
    tb_printf(MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH, y, color, TB_DEFAULT, BOARDER_COLUMN);
  }

  // BOTTOM Boarder
  for (size_t x = MESSAGE_BOARD_START_COLUMN + 1; x < MESSAGE_BOARD_WIDTH; x += 1) {
    tb_printf(x, MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT, color, TB_DEFAULT, BOARDER_ROW);
  }

  // TOP-LEFT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN, MESSAGE_BOARD_START_ROW, color, TB_DEFAULT, BOARDER_TOP_LEFT);

  // TOP-RIGHT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH, MESSAGE_BOARD_START_ROW, color, TB_DEFAULT, BOARDER_TOP_RIGHT);

  // BOTTOM-LEFT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN, MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT, color, TB_DEFAULT, BOARDER_BOTTOM_LEFT);

  // BOTTOM-RIGHT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH, MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT, color, TB_DEFAULT, BOARDER_BOTTOM_RIGHT);
}

static void turn_left(struct snake_struct *snake) {
  if (snake->current_movement != RIGHT) {
    snake->current_movement = LEFT;
  }
}

static void turn_right(struct snake_struct *snake) {
  if (snake->current_movement != LEFT) {
    snake->current_movement = RIGHT;
  }
}

static void turn_up(struct snake_struct *snake) {
  if (snake->current_movement != DOWN) {
    snake->current_movement = UP;
  }
}

static void turn_down(struct snake_struct *snake) {
  if (snake->current_movement != UP) {
    snake->current_movement = DOWN;
  }
}
static void reinit_snake(struct snake_struct *snake) {
  snake->pause = false;
  snake->quit = false;
}

static void* keyboard_input(void *arg) {
  struct thread_info *info = arg;
  struct snake_struct *snake = info->snake;

  if (snake == NULL) {
    exit(EXIT_FAILURE);
  }

  pthread_cond_signal(&keyboard_thread_started_cond);

  struct tb_event ev;
  while (!is_finished) {
    tb_poll_event(&ev);
    if (snake->pause || snake->quit) {
      reinit_snake(snake);

      continue;
    }

    reinit_snake(snake);

    char ch = ev.ch;
    switch (ch) {
      case 'p':
        snake->pause = true;

        break;
      case 'q':
        snake->quit = true;

        break;
      case 'h':
        turn_left(snake);

        break;
      case 'k':
        turn_up(snake);

        break;
      case 'l':
        turn_right(snake);

        break;
      case 'j':
        turn_down(snake);

        break;
    }

    switch (ev.key) {
      case TB_KEY_ARROW_LEFT:
        turn_left(snake);

        break;
      case TB_KEY_ARROW_UP:
        turn_up(snake);

        break;
      case TB_KEY_ARROW_RIGHT:
        turn_right(snake);

        break;
      case TB_KEY_ARROW_DOWN:
        turn_down(snake);

        break;
    }
  }

  return NULL;
}
