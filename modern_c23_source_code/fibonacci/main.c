#include <stdio.h>
#include <stdlib.h>

#ifndef nullptr
# define nullptr NULL
#endif

size_t fib(size_t n) {
  if (n < 3)
    return 1;
  else
    return fib(n-1) + fib(n-2);
}


int main(int argc, char *argv[]) {
  div_t result = div(5, 2);

  printf("Result is %d / %d\n", result.quot, result.rem);

  for (int i = 1; i < argc; ++i) {             // Processes args
    size_t const n = strtoull(argv[i], nullptr, 0);  // arg -> size_t
    printf("fib(%zu) is %zu\n",
           n, fib(n));
  }
  return EXIT_SUCCESS;
}
