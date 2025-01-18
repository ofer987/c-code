#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#ifndef nullptr
# define nullptr NULL
#endif

size_t fib(size_t n, int* buffer) {
  int i = INT_MAX;
  int j = INT_MIN;

  printf("int max is %d\n", i);
  printf("int min is %d\n", j);

  if (buffer[n] != 0) {
    printf("using buffer %zu\n", n);

    return buffer[n];
  }

  printf("not using buffer %zu\n", n);

  if (n < 3) {
    buffer[n] = 1;

    return 1;
  } else {
    size_t result = fib(n-1, buffer) + fib(n-2, buffer);

    buffer[n] = result;

    return result;
  }
}

size_t max(size_t* numbers, size_t length);

int main(int argc, char *argv[]) {
  clock_t start = clock();
  div_t result = div(5, 2);

  printf("Result is %d / %d\n", result.quot, result.rem);

  size_t fibonacci_sequences_length = argc - 1;
  size_t* fibonacci_sequences = malloc(argc * sizeof(size_t));

  for (int i = 1; i < argc; ++i) {
    size_t const n = strtoull(argv[i], nullptr, 0);  // arg -> size_t
    fibonacci_sequences[i - 1] = n;
  }

  printf("There are %d argc\n", argc);
  size_t maximum_fibonacci_number = max(fibonacci_sequences,
fibonacci_sequences_length);
  int* buffer = malloc(sizeof(size_t) * maximum_fibonacci_number);

  for (size_t i = 0; i < fibonacci_sequences_length; i += 1) {
    size_t length = fibonacci_sequences[i];
    size_t result = fib(length, buffer);

    printf("fib(%zu) is %zu\n", length, result);
  }

  free(fibonacci_sequences);

  clock_t end = clock();
  printf("It took %ld microseconds", end - start);

  return EXIT_SUCCESS;
}

size_t max(size_t* numbers, size_t length) {
  size_t max_number = 0;

  for (size_t i = 0; i < length; i += 1) {
    if (numbers[i] > max_number) {
      max_number = numbers[i];
    }
  }

  return max_number;
}
