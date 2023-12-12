#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 1000  // Size of the circular buffer
static uintptr_t buffer[BUFFER_SIZE];
static int in = 0;
static uintptr_t last_random_number;

volatile uint64_t prngState[2];
static inline uintptr_t getRandomPre(int rollback)
{
	// https://en.wikipedia.org/wiki/Xorshift#xorshift.2B
	uint64_t x = prngState[0];
	const uint64_t y = prngState[1];
	prngState[0] = prngState[0] * !rollback + rollback * y;
	x ^= x << 23;
	const uint64_t z = x ^ y ^ (x >> 17) ^ (y >> 26);
	prngState[1] = prngState[1] * !rollback + rollback * z;
	return (uintptr_t)(z + y);
}
void precalculate_random_numbers() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = getRandomPre(1);
    }
}
volatile uint64_t prngStateOG[2];
static inline uintptr_t getRandom()
{
    // https://en.wikipedia.org/wiki/Xorshift#xorshift.2B
    uint64_t x = prngStateOG[0];
    const uint64_t y = prngStateOG[1];
    prngStateOG[0] = y;
    x ^= x << 23;
    const uint64_t z = x ^ y ^ (x >> 17) ^ (y >> 26);
    prngStateOG[1] = z;
    return (uintptr_t)(z + y);
}
static inline uintptr_t getRandomRollback(uintptr_t rollback) {
    uintptr_t num = buffer[in];
    last_random_number = num;  // Store the last random number
    uintptr_t new_num = getRandomPre(rollback);
    buffer[in] = (new_num & -rollback) | (num & ~-rollback);  // Generate a new random number only if rollback is not zero
    in = ((in + 1) & -rollback) | (in & ~-rollback);  // Roll back if rollback is zero
    return num;
}


int main() {
    // Initialize the PRNG state
    prngState[0] = time(NULL);
    prngState[1] = 0xDEADBEEF;

    prngStateOG[0] = time(NULL);
    prngStateOG[1] = 0xDEADBEEF;

    // Precalculate random numbers
    precalculate_random_numbers();

    // Get a random number without rollback
    uintptr_t num1 = getRandomRollback(1);
    printf("Random number without rollback: %lu\n", num1);

    uintptr_t numOG = getRandom();
    printf("Random number OG: %lu\n", numOG);

    // Get a random number with rollback
    uintptr_t num2 = getRandomRollback(0);
    printf("Random number with rollback: %lu\n", num2);

    // Check if the random number with rollback is the same as the previous number
    if (num2 == num1) {
        printf("Rollback works correctly\n");
    } else {
        printf("Rollback does not work correctly\n");
    }

    // Get another random number without rollback
    uintptr_t num3 = getRandomRollback(1);
    printf("Another random number without rollback: %lu\n", num3);
    uintptr_t numOG2 = getRandom();
    printf("Random number OG2: %lu\n", numOG2);

    // Check if the new random number is different from the previous numbers
    if (num3 != num1 && num3 != num2) {
        printf("Random number generation works correctly\n");
    } else {
        printf("Random number generation does not work correctly\n");
    }

    return 0;
}
