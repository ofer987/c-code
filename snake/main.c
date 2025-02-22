#include <locale.h>
#include <stdbool.h>
#include <pthread.h>

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

#define BORDER_HORIZONTAL_SIZE 30
#define BORDER_VERTICAL_SIZE 30

#define SIDE_VERTICAL_START 1
#define SIDE_VERTICAL_END 29

#define SIDE_HORIZONTAL_START 1
#define SIDE_HORIZONTAL_END 29

#define SCREEN_SIZE 900

#define MESSAGE_BOARD_START_COLUMN  0
#define MESSAGE_BOARD_START_ROW     32

#define MESSAGE_BOARD_WIDTH         30
#define MESSAGE_BOARD_HEIGHT        11

#define BORDER_ROW                 "━"
#define BORDER_COLUMN              "┃"
#define BORDER_TOP_LEFT            "┏"
#define BORDER_TOP_RIGHT           "┓"
#define BORDER_BOTTOM_LEFT         "┗"
#define BORDER_BOTTOM_RIGHT        "┛"

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
  USED_BY_TOP_BORDER,
  USED_BY_TOP_RIGHT_BORDER,
  USED_BY_RIGHT_BORDER,
  USED_BY_BOTTOM_RIGHT_BORDER,
  USED_BY_BOTTOM_BORDER,
  USED_BY_BOTTOM_LEFT_BORDER,
  USED_BY_LEFT_BORDER,
  USED_BY_TOP_LEFT_BORDER,
  USED_BY_MESSAGE,
  TOTAL
};

char* screen_chars[TOTAL] = {
  [AVAILABLE] = " ",
  [USED_BY_SNAKE_TAIL] = "#",
  [USED_BY_SNAKE_HEAD] = "@",
  [USED_BY_FOOD] = "$",
  [USED_BY_TOP_BORDER] = BORDER_ROW,
  [USED_BY_TOP_RIGHT_BORDER] = BORDER_TOP_RIGHT,
  [USED_BY_RIGHT_BORDER] = BORDER_COLUMN,
  [USED_BY_BOTTOM_RIGHT_BORDER] = BORDER_BOTTOM_RIGHT,
  [USED_BY_BOTTOM_BORDER] = BORDER_ROW,
  [USED_BY_BOTTOM_LEFT_BORDER] = BORDER_BOTTOM_LEFT,
  [USED_BY_LEFT_BORDER] = BORDER_COLUMN,
  [USED_BY_TOP_LEFT_BORDER] = BORDER_TOP_LEFT,
  [USED_BY_MESSAGE] = " "
};

size_t screen_background_colors[TOTAL] = {
  [AVAILABLE] = TB_DEFAULT,
  [USED_BY_SNAKE_TAIL] = TB_BLACK,
  [USED_BY_SNAKE_HEAD] = TB_BLACK,
  [USED_BY_FOOD] = TB_BLACK,
  [USED_BY_TOP_BORDER] = TB_BLUE,
  [USED_BY_TOP_RIGHT_BORDER] = TB_BLUE,
  [USED_BY_RIGHT_BORDER] = TB_BLUE,
  [USED_BY_BOTTOM_RIGHT_BORDER] = TB_BLUE,
  [USED_BY_BOTTOM_BORDER] = TB_BLUE,
  [USED_BY_BOTTOM_LEFT_BORDER] = TB_BLUE,
  [USED_BY_LEFT_BORDER] = TB_BLUE,
  [USED_BY_TOP_LEFT_BORDER] = TB_BLUE,
  [USED_BY_MESSAGE] = TB_BLACK
};

size_t screen_colors[TOTAL] = {
  [AVAILABLE] = TB_WHITE,
  [USED_BY_SNAKE_TAIL] = TB_BLUE,
  [USED_BY_SNAKE_HEAD] = TB_MAGENTA,
  [USED_BY_FOOD] = TB_GREEN,
  [USED_BY_TOP_BORDER] = TB_WHITE,
  [USED_BY_TOP_RIGHT_BORDER] = TB_WHITE,
  [USED_BY_RIGHT_BORDER] = TB_WHITE,
  [USED_BY_BOTTOM_RIGHT_BORDER] = TB_WHITE,
  [USED_BY_BOTTOM_BORDER] = TB_WHITE,
  [USED_BY_BOTTOM_LEFT_BORDER] = TB_WHITE,
  [USED_BY_LEFT_BORDER] = TB_WHITE,
  [USED_BY_TOP_LEFT_BORDER] = TB_WHITE,
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

struct results_struct {
  size_t result;
  size_t minutes;
  size_t seconds;
};

static void finish(int sig, struct snake_struct *snake, time_t start_time);

void init_screen(
    enum screen_state *screen_states,
    struct snake_struct *snake,
    struct coordinates *food_location);

void eat_food(struct snake_struct *snake, struct coordinates *food_location);
bool is_same_coordinates(
    struct coordinates *coord_01,
    struct coordinates *coord_02);
void render_screen(
    enum screen_state *screen_states,
    struct results_struct *results);
void generate_food(
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

struct results_struct read_file(char *path) {
  size_t result;
  size_t minutes;
  size_t seconds;

  FILE *file = fopen(path, "rt");
  fscanf(
      file,
      "Snake size is %zu\n"
      "The game took %zu minutes and %zu seconds\n",
      &result,
      &minutes,
      &seconds);

  struct results_struct results;
  results.result = result;
  results.minutes = minutes;
  results.seconds = seconds;

  return results;
}

void show_results(struct results_struct *results) {
  tb_printf(MESSAGE_BOARD_START_COLUMN + 3, MESSAGE_BOARD_START_ROW + 8, TB_WHITE, TB_DEFAULT, "The snake is %zu",  results->result);
  tb_printf(MESSAGE_BOARD_START_COLUMN + 3, MESSAGE_BOARD_START_ROW + 9, TB_WHITE, TB_DEFAULT, "Minutes: %zu",  results->minutes);
  tb_printf(MESSAGE_BOARD_START_COLUMN + 3, MESSAGE_BOARD_START_ROW + 10, TB_WHITE, TB_DEFAULT, "Seconds: %zu",  results->seconds);
}

void draw_top_border(enum screen_state *screen_states) {
  size_t x = 0;
  size_t y = 0;
  size_t i = coordinates_to_index(x, y);
  screen_states[i] = USED_BY_TOP_LEFT_BORDER;

  for (size_t x = 1; x < BORDER_HORIZONTAL_SIZE; x += 1) {
    size_t i = coordinates_to_index(x, y);

    screen_states[i] = USED_BY_TOP_BORDER;
  }
}

void draw_right_border(enum screen_state *screen_states) {
  size_t x = BORDER_HORIZONTAL_SIZE - 1;
  size_t y = 0;
  size_t i = coordinates_to_index(x, y);
  screen_states[i] = USED_BY_TOP_RIGHT_BORDER;

  for (size_t y = 1; y < BORDER_HORIZONTAL_SIZE - 1; y += 1) {
    size_t i = coordinates_to_index(x, y);

    screen_states[i] = USED_BY_RIGHT_BORDER;
  }
}

void draw_bottom_border(enum screen_state *screen_states) {
  size_t x = BORDER_HORIZONTAL_SIZE - 1;
  size_t y = BORDER_VERTICAL_SIZE - 1;
  size_t i = coordinates_to_index(x, y);
  screen_states[i] = USED_BY_BOTTOM_RIGHT_BORDER;

  for (size_t x = 0; x < BORDER_HORIZONTAL_SIZE - 1; x += 1) {
    size_t i = coordinates_to_index(x, y);

    screen_states[i] = USED_BY_BOTTOM_BORDER;
  }
}

void draw_left_border(enum screen_state *screen_states) {
  size_t x = 0;
  size_t y = BORDER_VERTICAL_SIZE - 1;
  size_t i = coordinates_to_index(x, y);
  screen_states[i] = USED_BY_BOTTOM_LEFT_BORDER;

  for (size_t y = 1; y < BORDER_VERTICAL_SIZE - 1; y += 1) {
    size_t i = coordinates_to_index(x, y);

    screen_states[i] = USED_BY_LEFT_BORDER;
  }
}

int main(int argc, char *argv[]) {
  setlocale(LC_ALL, "");
  struct results_struct previous_results = read_file("./results.txt");
  printf("Last result was %zu\n", previous_results.result);
  printf("And it took %zu minutes and %zu seconds\n", previous_results.minutes, previous_results.seconds);
  srandom(time(NULL));

  enum screen_state screen_states[SCREEN_SIZE];

  struct coordinates head = {
    .y = BORDER_HORIZONTAL_SIZE / 2,
    .x = BORDER_HORIZONTAL_SIZE / 2,
  };
  struct snake_struct snake = {
    .head = &head,
    .list_size = 1,
    .current_movement = RIGHT,
    .quit = false
  };

  // Reset the screen
  draw_top_border(screen_states);
  draw_right_border(screen_states);
  draw_bottom_border(screen_states);
  draw_left_border(screen_states);

  for (size_t x = SIDE_HORIZONTAL_START; x < SIDE_HORIZONTAL_END; x += 1) {
    for (size_t y = SIDE_VERTICAL_START; y < SIDE_VERTICAL_END; y += 1) {
      size_t i = coordinates_to_index(x, y);

      screen_states[i] = AVAILABLE;
    }
  }
  size_t i = coordinates_to_index(snake.head->x, snake.head->y);
  screen_states[i] = USED_BY_SNAKE_HEAD;

  struct coordinates food_location;
  generate_food(screen_states, &food_location);

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

  time_t start_time = time(NULL);
  struct results_struct results;
  for (;;) {
    usleep(100000);

    time_t end_time = time(NULL);

    time_t game_time = end_time - start_time;
    time_t game_in_minutes = game_time / 60;
    time_t game_in_seconds = game_time % 60;

    results.result = snake.list_size;
    results.minutes = game_in_minutes;
    results.seconds = game_in_seconds;

    init_screen(screen_states, &snake, &food_location);
    render_screen(screen_states, &results);

    if (snake.pause) {
      tb_printf(MESSAGE_START_COLUMN, FIRST_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "You have paused the game");
      tb_printf(MESSAGE_START_COLUMN, SECOND_MESSAGE_LINE, TB_WHITE, TB_DEFAULT, "Press any key to continue");
      render_screen(screen_states, &results);

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
      render_screen(screen_states, &results);

      finish(0, &snake, start_time);
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
      render_screen(screen_states, &results);

      finish(0, &snake, start_time);
    }
  }

  finish(0, &snake, start_time);
}

static void finish(int sig, struct snake_struct *snake, time_t start_time) {
  time_t end_time = time(NULL);
  is_finished = true;

  time_t game_time = end_time - start_time;
  time_t game_in_minutes = game_time / 60;
  time_t game_in_seconds = game_time % 60;

  FILE *results = fopen("./results.txt", "wt");

  fprintf(results, "Snake size is %zu\n", snake->list_size);
  fprintf(results, "The game took %zu minutes and %zu seconds\n", game_in_minutes, game_in_seconds);

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

  draw_top_border(screen_states);
  draw_right_border(screen_states);
  draw_bottom_border(screen_states);
  draw_left_border(screen_states);

  for (size_t x = SIDE_HORIZONTAL_START; x < SIDE_HORIZONTAL_END; x += 1) {
    for (size_t y = SIDE_VERTICAL_START; y < SIDE_VERTICAL_END; y += 1) {
      size_t i = coordinates_to_index(x, y);

      screen_states[i] = AVAILABLE;
    }
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

void render_screen(
    enum screen_state *screen_states,
    struct results_struct *results) {
  for (size_t i = 0; i < SCREEN_SIZE; i += 1) {
    struct coordinates coords = index_to_coorindates(i);

    size_t state = screen_states[i];
    int32_t bg_color = screen_background_colors[state];
    int32_t color = screen_colors[state];
    char* ch = screen_chars[state];

    tb_printf(coords.x, coords.y, color, bg_color, ch);
  }

  render_the_message_box_boarders();
  show_results(results);

  tb_present();
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

  while (true) {
    size_t random_number = random() / (RAND_MAX / available_tiles);

    if (screen_states[random_number] != AVAILABLE) {
      continue;
    }

    struct coordinates result = index_to_coorindates(random_number);
    food_location->y = result.y;
    food_location->x = result.x;

    return;
  }
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
  if (snake_head->y < 0 || snake_head->y >= BORDER_HORIZONTAL_SIZE) {
    return false;
  }

  if (snake_head->x < 0 || snake_head->x >= BORDER_HORIZONTAL_SIZE) {
    return false;
  }

  size_t snake_head_location = coordinates_to_index(
      snake_head->x,
      snake_head->y);
  enum screen_state snake_location = screen_states[snake_head_location];
  switch (snake_location) {
    case USED_BY_SNAKE_TAIL: /* FALLTHROUGH */
    case USED_BY_TOP_BORDER: /* FALLTHROUGH */
    case USED_BY_TOP_RIGHT_BORDER: /* FALLTHROUGH */
    case USED_BY_RIGHT_BORDER: /* FALLTHROUGH */
    case USED_BY_BOTTOM_RIGHT_BORDER: /* FALLTHROUGH */
    case USED_BY_BOTTOM_BORDER: /* FALLTHROUGH */
    case USED_BY_BOTTOM_LEFT_BORDER: /* FALLTHROUGH */
    case USED_BY_LEFT_BORDER: /* FALLTHROUGH */
    case USED_BY_TOP_LEFT_BORDER: /* FALLTHROUGH */
      return false;
    default:
      return true;
  }
}

struct coordinates index_to_coorindates(size_t index) {
  struct coordinates foobar = {
    .x = index % BORDER_HORIZONTAL_SIZE,
    .y = index / BORDER_HORIZONTAL_SIZE
  };

  return foobar;
}

size_t coordinates_to_index(size_t x, size_t y) {
  return BORDER_HORIZONTAL_SIZE * y + x;
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
  size_t color = screen_colors[USED_BY_TOP_BORDER];

  // LEFT Boarder
  for (size_t y = MESSAGE_BOARD_START_ROW + 1; y < MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT; y += 1) {
    tb_printf(MESSAGE_BOARD_START_COLUMN, y, color, TB_DEFAULT, BORDER_COLUMN);
  }

  // TOP Boarder
  for (size_t x = MESSAGE_BOARD_START_COLUMN + 1; x < MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH; x += 1) {
    tb_printf(x, MESSAGE_BOARD_START_ROW, color, TB_DEFAULT, BORDER_ROW);
  }

  // RIGHT Boarder
  for (size_t y = MESSAGE_BOARD_START_ROW + 1; y < MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT; y += 1) {
    tb_printf(MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH, y, color, TB_DEFAULT, BORDER_COLUMN);
  }

  // BOTTOM Boarder
  for (size_t x = MESSAGE_BOARD_START_COLUMN + 1; x < MESSAGE_BOARD_WIDTH; x += 1) {
    tb_printf(x, MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT, color, TB_DEFAULT, BORDER_ROW);
  }

  // TOP-LEFT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN, MESSAGE_BOARD_START_ROW, color, TB_DEFAULT, BORDER_TOP_LEFT);

  // TOP-RIGHT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH, MESSAGE_BOARD_START_ROW, color, TB_DEFAULT, BORDER_TOP_RIGHT);

  // BOTTOM-LEFT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN, MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT, color, TB_DEFAULT, BORDER_BOTTOM_LEFT);

  // BOTTOM-RIGHT Corner
  tb_printf(MESSAGE_BOARD_START_COLUMN + MESSAGE_BOARD_WIDTH, MESSAGE_BOARD_START_ROW + MESSAGE_BOARD_HEIGHT, color, TB_DEFAULT, BORDER_BOTTOM_RIGHT);
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
