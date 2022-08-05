#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>

#define MEGA 1024*1024
#define PACKET 1278

static char buffer[MEGA];

bool check_ll2(long long a) {
  volatile double b = (double) a;
  const double d_longLong_max_plus_1 = (LLONG_MAX/2 + 1)*2.0;
  #if LLONG_MIN == -LLONG_MAX
    const double d_longLong_min_minus_1 = (LLONG_MIN/2 - 1)*2.0;;
    if (b <= d_longLong_min_minus_1 || b >= d_longLong_max_plus_1) {
      return false;
    }
  #else
    if (b >= d_longLong_max_plus_1) {
      return false;
    }
  #endif
  return (long long) b == a;
}

int main(int argc, const char** argv) {
    // buffer = malloc(MEGA);
    assert(buffer);

    FILE* f = fopen(argv[1], "r");
    if (!f)
        return 1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    printf("%d\n", check_ll2(MEGA));

    register long n_bufs = ceil((double)size / (MEGA));
    
    clock_t begin = clock();

    for (register long i = 0; i < n_bufs; i++)
        fread(buffer, 1, MEGA, f);

    clock_t end = clock();

    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
 
    printf("Read file '%s' of size '%lld' in %lf seconds\n", argv[1], size, time_spent);

    fclose(f);
}