#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 1000  // Size of the circular buffer
static uintptr_t buffer[BUFFER_SIZE];
static int in = 0;
static uintptr_t last_random_number;

static uintptr_t pre_buffer[BUFFER_SIZE];
static int pre_in = 0;

volatile uint64_t prngState[2];

static inline uintptr_t getRandomPre()
{
	// https://en.wikipedia.org/wiki/Xorshift#xorshift.2B
	uint64_t x = prngState[0];
	const uint64_t y = prngState[1];
	prngState[0] = y;
	x ^= x << 23;
	const uint64_t z = x ^ y ^ (x >> 17) ^ (y >> 26);
	prngState[1] = z;

    pre_buffer[pre_in] = num;
    pre_in = (pre_in + 1) % BUFFER_SIZE;

    uintptr_t num = (uintptr_t)(z + y);


	return num;
}
void precalculate_random_numbers() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = getRandomPre();
    }
}
static inline uintptr_t getRandom() {
    uintptr_t num = buffer[in];
    last_random_number = num;  // Store the last random number
	buffer[in] = getRandomPre();  // Generate a new random number and add it to the buffer
    in = (in + 1) % BUFFER_SIZE;  // Wrap around to 0 when index reaches BUFFER_SIZE
    return num;
}
static inline uintptr_t getRandomRollback(uintptr_t rollback) {
    uintptr_t num = buffer[in];
    buffer[in] = pre_buffer[pre_in];  // Use the current random number from pre_buffer
    in = ((in + 1) & -rollback) | (in & ~-rollback);  // Roll back if rollback is zero
    return num;
}


int main() {
    // Initialize the PRNG state
    prngState[0] = time(NULL);
    prngState[1] = 0xDEADBEEF;

    // Precalculate random numbers
    precalculate_random_numbers();

    for (int i = 0; i < 10000; ++i) {
        uintptr_t num = getRandom();
        uintptr_t num1 = getRandomRollback(0);
        if (num2 == num1) {
            printf("%lu = %luy\n", num, num1);
            } 
        else {
            printf("Rollback does not work correctly\n");
            }
    }
    return 0;
}