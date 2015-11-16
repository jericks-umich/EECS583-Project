#include "stdio.h"
#include "stdlib.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
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
	int fd;
	void *random_map;
	struct stat statbuf;
	fd = open("random.dump", O_RDONLY);
	fstat(fd,&statbuf); // get filesize
	random_map = mmap(NULL,statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0); // mmap random.dump into memory
	if ((long long)random_map == -1) { // if error
		printf("Error: %d\n",errno);
		return -1;
	}

	int *addr = (int*)random_map + 1024;
	// probe it to make sure it actually loads into memory from disk, this will also flush it from the cache
	probe((char*)addr); 

	for(int i=0; i<1000000; i++) {} // spin wait for a bit

	char a;
	for(int i=0; i<100000000; i++) {
		//prefetch(addr);
		__builtin_prefetch(addr,0,0);
		for(int j=0; j<50; j++){}
		//a = *(char*)addr;
	}

	//unsigned long p = probe((char*)addr);
	//printf("%llu\n",p);
	
	return 0;
}
