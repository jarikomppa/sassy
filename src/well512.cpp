/*
	The Well Equidistributed Long-period Linear (WELL) is 
	a pseudorandom number generator developed in 2006 by 
	F. Panneton, P. L'Ecuyer, and M. Matsumoto [1] that is 
	based on linear recurrences modulo 2 over a finite binary field F2.

	c++ functions based on a c++ version of mersenne twister

	everything in public domain
*/
#include <stdlib.h>
//#include <stdio.h>

#include "well512.h"



WELL512::WELL512(unsigned long s) :
    index(0)
{
    init_genrand(s);
}


WELL512::WELL512(unsigned long init_key[], int key_length) :
	index(0)
{
    init_by_array(init_key, key_length);
}


WELL512::~WELL512()
{
}


/* initializes state[N] with a seed */
/* based on mersenne twister init */
void WELL512::init_genrand(unsigned long s)
{
    int j;
    state[0]= s & 0xffffffffUL;
    for (j=1; j<16; j++) {
        state[j] = (1812433253UL * (state[j-1] ^ (state[j-1] >> 30)) + j); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array state[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        state[j] &= 0xffffffffUL;  /* for >32 bit machines */
    }
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
/* based on mersenne twister init */
void WELL512::init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (16>key_length ? 16 : key_length);
    for (; k; k--) {
        state[i] = (state[i] ^ ((state[i-1] ^ (state[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=16) { state[0] = state[16-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=16-1; k; k--) {
        state[i] = (state[i] ^ ((state[i-1] ^ (state[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        state[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=16) { state[0] = state[16-1]; i=1; }
    }

    state[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long WELL512::genrand_int32(void)
{
  unsigned long a, b, c, d;
  a = state[index];
  c = state[(index+13)&15];
  b = a^c^(a<<16)^(c<<15);
  c = state[(index+9)&15];
  c ^= (c>>11);
  a = state[index] = b^c;
  d = a^((a<<5)&0xDA442D20UL);
  index = (index + 15)&15;
  a = state[index];
  state[index] = a^b^d^(a<<2)^(b<<18)^(c<<28);
  return state[index];
}

/* generates a random number on [0,0x7fffffff]-interval */
long WELL512::genrand_int31(void)
{
    unsigned long y;

    y = genrand_int32();

    return (long)(y>>1);
}

/* generates a random number on [0,1]-real-interval */
double WELL512::genrand_real1(void)
{
    unsigned long y;

    y = genrand_int32();

    return (double)y * (1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

/* generates a random number on [0,1)-real-interval */
double WELL512::genrand_real2(void)
{
    unsigned long y;

    y = genrand_int32();

    return (double)y * (1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double WELL512::genrand_real3(void)
{
    unsigned long y;

    y = genrand_int32();

    return ((double)y + 0.5) * (1.0/4294967296.0); 
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double WELL512::genrand_res53(void) 
{ 
    unsigned long a=genrand_int32()>>5, b=genrand_int32()>>6; 
    return(a*67108864.0+b)*(1.0/9007199254740992.0); 
} 
/* These real versions are due to Isaku Wada, 2002/01/09 added */


#if 0
int main(void)
{
    int i;
    unsigned long init[4]={0x123, 0x234, 0x345, 0x456}, length=4;
    init_by_array(init, length);
    /* This is an example of initializing by an array.       */
    /* You may use init_genrand(seed) with any 32bit integer */
    /* as a seed for a simpler initialization                */
    printf("1000 outputs of genrand_int32()\n");
    for (i=0; i<1000; i++) {
      printf("%10lu ", genrand_int32());
      if (i%5==4) printf("\n");
    }
    printf("\n1000 outputs of genrand_real2()\n");
    for (i=0; i<1000; i++) {
      printf("%10.8f ", genrand_real2());
      if (i%5==4) printf("\n");
    }

    return 0;
}
#endif
