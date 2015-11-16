#include "stdio.h"
#include "stdlib.h"
#include "xmmintrin.h"

#define prefetch(addr) _mm_prefetch(((char*)(addr)),_MM_HINT_NTA)

unsigned long probe(char *adrs) {
	volatile unsigned long time;
	asm __volatile__ (
			"  mfence				\n"
			"  lfence				\n"
			"  rdtsc				\n"
			"  lfence				\n"
			"  movl %%eax, %%esi	\n"
			"  movl (%1), %%eax		\n"
			"  lfence							\n"
			"  rdtsc							\n"
			"  subl %%esi, %%eax	\n"
			"  clflush 0(%1)			\n"
			: "=a" (time)
			: "c" (adrs)
			:  "%esi", "%edx");
	return time;
}


int main() {
	void *printfunc = printf;
	//printf("Hello World!\n");
	//printf("printf() is at 0x%x\n",printfunc);
	printf("loading printf function\n"); // instruction is now cached
	int offset = *(int*)(printfunc+2); // two bytes past jmp instruction in plt
																		 // first two bytes are FF 25, opcode for jmp
																		 // next four are address of __printf function
	//printf("offset = 0x%x\n", offset);
	void *printf_addr_ref = (void*)(printfunc+offset+6); // 6 is number of bytes of
																											 // jmp opcode
	printf("addr_ref = 0x%x\n", printf_addr_ref);
	void *printf_addr = *(void**)printf_addr_ref;
  printf("printf() is actually at 0x%llx\n",printf_addr);
	
	////////////////
	// Experiment //
	////////////////
	
	unsigned long cached,uncached,prefetched;
	cached = probe(printf_addr); // get timing for cached value
	uncached = probe(printf_addr); // get timing for uncached value

	for(int i=0; i<1000000; i++) {} // spin wait for a bit
	prefetch(printf_addr);
	for(int i=0; i<1000000; i++) {} // spin wait for 10x longer

	prefetched = probe(printf_addr); // get timing for cached value

	printf("%llu %llu %llu\n",cached, uncached, prefetched);

	//unsigned long *trials = malloc(sizeof(unsigned long) * 10);
	//while (1) {
	//	for(int i=0; i<10; i++) {
	//		//trials[i] = probe((char*)printf_addr+9); // arbitrary line of code within printf
	//		trials[i] = probe((char*)printf_addr); // arbitrary line of code within printf
	//		for(int j=0; j<100000000; j++) {}
	//	}
	//	printf("times: ");
	//	for(int i=0; i<10; i++) {
	//		printf("%u ", trials[i]);
	//	}
	//	printf("\n");
	//}

	return 0;
}
