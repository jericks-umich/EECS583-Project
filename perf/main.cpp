#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sched.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "Perf.hpp"
#include "Stats.h"
#include "Helpers.h"

using namespace std;

#define BUF_BITS 9
//#define SAMPLE_PERIOD 4000 
#define SAMPLE_PERIOD 10

#define RESERVATION_SIZE    32768
#define CACHE_CAPACITY 20000000
#define NUMBER_TOP_PC 5

struct RawPerfCounter
{
	const char* name;
	const char* event_name; 
};


static const RawPerfCounter COUNTERS[] = 
{
	{ "MEM_LOAD_UOPS_RETIRED_L3_MISS", "0x20D1" }, /* Haswell */
};

#define NUM_COUNTERS (sizeof(COUNTERS) / sizeof(COUNTERS[0]))

//#define LLC_LOAD_MISS (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_READ << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16))
//#define LLC_STORE_MISS (PERF_COUNT_HW_CACHE_LL | (PERF_COUNT_HW_CACHE_OP_WRITE << 8) | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16))

void abort(const char* msg, ...) { va_list ap; va_start(ap, msg); vprintf(msg, ap); exit(-1); }
void myassert(bool cond, const char* msg, ...) { if(!cond) { va_list ap; va_start(ap, msg); vprintf(msg, ap); exit(-1); } }

class SimpleGate
{
	public:
		SimpleGate()
		{
			_evt = eventfd(0, EFD_CLOEXEC);
			myassert(_evt >= 0, "Event creation error!\n");
		}

		~SimpleGate() { close(_evt); }

		uint64_t wait()
		{
			uint64_t tmp = 0;
			read(_evt, &tmp, sizeof(tmp));
			return tmp;
		}

		void open(uint64_t value)
		{
			write(_evt, &value, sizeof(value));
		}

		int get_fd() const { return _evt; }

	private:
		int _evt;
};

enum { INIT_OK = 1, INIT_FAIL = 2 };
static SimpleGate child_done;
static uint64_t   child_done_time = 0;

void sigchild_handler(int sig)
{
	child_done_time = PerfCounted::cur_time();
	child_done.open(INIT_OK);
}



int main(int argc, char **argv)
{
	int k = NUMBER_TOP_PC;
	cpu_set_t cpu_set; 
	CPU_ZERO(&cpu_set); 
	uint64_t miss_count = 0, compulsory_count = 0, capacity_count = 0, coherence_count = 0, conflict_count = 0, hitm_count = 0;
	uint64_t startTime, endTime, execTime;
	uint64_t prev_record = 0, curr_record = 0, diff_record = 0;
	int fifo_misses;

	myassert(argc >= 2,  "Syntax: %s <cmd> [args...]\n", argv[0]);

	SimpleGate gate;

	int pid = fork();
	myassert(pid >= 0, "Fork error!\n");
	startTime = GetTimeUs();

	if(pid == 0)
	{
		printf("Entering child\n");
		//Migrate child to second socket
		CPU_SET(3, &cpu_set);
		/*CPU_SET(8, &cpu_set);
		CPU_SET(9, &cpu_set);
		CPU_SET(10, &cpu_set);
		CPU_SET(11, &cpu_set);
		CPU_SET(12, &cpu_set);
		CPU_SET(13, &cpu_set);
		CPU_SET(14, &cpu_set);
		CPU_SET(15, &cpu_set);
		CPU_SET(24, &cpu_set);
		CPU_SET(25, &cpu_set);
		CPU_SET(26, &cpu_set);
		CPU_SET(27, &cpu_set);
		CPU_SET(28, &cpu_set);
		CPU_SET(29, &cpu_set);
		CPU_SET(30, &cpu_set);
		CPU_SET(31, &cpu_set);*/

		sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

		/*uint64_t tmp = gate.wait();
		if(tmp == INIT_FAIL)
		{
			printf("Aborted.\n");
			return -1;
		}*/

		// ready
		for(int i = 1; i < argc; ++ i)
			argv[i - 1] = argv[i];
		argv[argc - 1] = NULL;
		printf("Running child...\n");
		execvp(argv[0], argv);
		myassert(false, "Exec failed!: %i: %s\n", errno, strerror(errno)); 
		printf("Child\n");
	}
	else
	{  
		//Detector on the 1st socket
		/*CPU_SET(0, &cpu_set);
		CPU_SET(1, &cpu_set);*/
		CPU_SET(2, &cpu_set);
		/*CPU_SET(3, &cpu_set);
		CPU_SET(4, &cpu_set);
		CPU_SET(5, &cpu_set);
		CPU_SET(6, &cpu_set);
		CPU_SET(7, &cpu_set);
		CPU_SET(16, &cpu_set);
		CPU_SET(17, &cpu_set);
		CPU_SET(18, &cpu_set);
		CPU_SET(19, &cpu_set);
		CPU_SET(20, &cpu_set);
		CPU_SET(21, &cpu_set);
		CPU_SET(22, &cpu_set);
		CPU_SET(23, &cpu_set);*/
		sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);


		char fifo1[] = "/tmp/fifo1";
		char msg[500], buf[500];

		bool flag = false;
		printf("Before open \n");
		fifo_misses = open(fifo1, O_WRONLY);
		printf("Opened fifo 1\n");

		signal(SIGCHLD, sigchild_handler);

		PerfCounted llc_miss(0x20D1);
		if(!llc_miss.open(pid))
		{
			printf("Failed opening LLC miss counter!\n");
			gate.open(INIT_FAIL);
			return -1;
		}

		/*PerfSampled* perfs[NUM_COUNTERS];
		for(size_t i = 0; i < NUM_COUNTERS; ++ i)
		{
			perfs[i] = new PerfSampled(BUF_BITS, SAMPLE_PERIOD, COUNTERS[i].event_name);
			//if(perfs[i] == NULL || !perfs[i]->open(pid, false  enable ))
			if(perfs[i] == NULL || !perfs[i]->open(pid))
			{
				printf("Failed opening perf counter for %s: %i: %s.\n", COUNTERS[i].name, errno, strerror(errno));
				gate.open(INIT_FAIL);
				return -1;
			}
		}*/

		//gate.open(INIT_OK); 
		printf("Program started, waiting for termination, collecting %lu events.\n", NUM_COUNTERS);
		int status = 0;
		struct timeval tv;
		fd_set rfds;
		FD_ZERO(&rfds);

		CountedRecord cr;

		int xx = 0;
		printf("event,time,pc,data-addr\n");
		ofstream f;
		f.open("misses.txt");
		for(;;)
		{
			++ xx;
			tv.tv_sec = 0;
			tv.tv_usec = 10;
			FD_SET(child_done.get_fd(), &rfds);
			if(select(child_done.get_fd() + 1, &rfds, NULL, NULL, &tv) != 0)
			{
				waitpid(pid, &status, 0);
				break;
			}
			if(!llc_miss.get_value(&cr, false))
				exit(1);
			printf("%lu\n", cr.value);
				curr_record = cr.value;
				diff_record = curr_record - prev_record;
				//printf("%lu, %lu, %lu\n", curr_record, prev_record, diff_record);
				prev_record = curr_record;
				

				//Write misses value to the fifo
				sprintf(buf, "%lu", diff_record);
				strcat(msg,buf);
				strcat(msg, "\0");
				
				write(fifo_misses, &diff_record, sizeof(diff_record));
				//printf("%lu", diff_record);	

				f << diff_record;
				f << "\n";

			/*for(size_t i = 0; i < NUM_COUNTERS; ++ i)
			  {
			  PerfSampled* pf = perfs[i];
			  if(!pf->start_iterate())
			  abort("Failed iterating performance counter!\n");
			  SampledRecord dr;
			  DataAccess da;
			  if(strcmp(COUNTERS[i].event_name, "0x20D1") == 0)
			  {
			  while (pf->next(&dr)){
			  miss_count++;
			  printf("We have an event\n");
			//What type is this particular miss - initialize flag
			printf("%s,%lu,%p,%p,%lu\n", COUNTERS[i].name, dr.time, dr.ip, dr.data, cr.value);
			}
			}

			}*/
		}
		f.close();

		//Delete fifo
		//unlink(fifo1);



		/*for(size_t i = 0; i < NUM_COUNTERS; ++ i)
		{
			PerfSampled* pf = perfs[i];
			delete pf;
		}*/

		printf("Program done with status: %i.\n", status);
		endTime = GetTimeUs();
		execTime = endTime - startTime;
	}


	return 0;
}
