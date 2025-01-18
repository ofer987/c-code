#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/* #include <limits.h> */

#ifndef nullptr
# define nullptr NULL
#endif

/* 0: 0 
 * 1: 1
 * 2: 1
 * 3: 2 
 * 4: 3 */
size_t fib(size_t number) {
  size_t x1 = 1;
  // For now
  size_t x2 = number%2 ? 1 : 2;

  for (size_t i = (number-1)/2; i; i -= 1) {
    x1 += x2;
    x2 += x1;
  }

  return x1;
}

int main(int argc, char* argv[argc]) {
  clock_t start = clock();
  size_t const number = strtoull(argv[1], nullptr, 0);  // arg -> size_t

  for (size_t i = 0; i < 10; i++) {
    printf("i++ is %zu\n", i);
  }
  printf("\n");

  for (size_t i = 0; i < 10; ++i) {
    printf("++i is %zu\n", i);
  }
  printf("\n");

  if (number <= 0) {
    printf("The value of Fibonacci(%zu) is %d\n", number, 0);

    return EXIT_SUCCESS;
  }

  size_t result = fib(number);
  clock_t end = clock();

  printf("The value of Fibonacci(%zu) is %zu\n", number, result);
  printf("It took %ld microseconds", end - start);

  return EXIT_SUCCESS;
}
