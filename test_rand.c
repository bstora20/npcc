#include <stdio.h>
#include <stdint.h>

volatile uint64_t prngState[2];
volatile uint64_t prngStateOG[2];
uintptr_t getRandom(int check){
    uint64_t x = prngState[0];
    const uint64_t y = prngState[1];
    prngState[0] = check*prngState[0]+ !check*y;
    x ^= x << 23;
    const uint64_t z = x ^ y ^ (x >> 17) ^ (y >> 26); 
    prngState[1] = check*prngState[1]+ !check*z;
    
    return (uintptr_t)(z+y);
}

static inline uintptr_t OGRandom()
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


int main(){
    uintptr_t diff;
    for (int i = 0;i<10000;i++){
        diff = getRandom(1)-OGRandom();
        printf("%d ",diff);
    }
    return 0;
}
