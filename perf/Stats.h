#include <inttypes.h>
#include <stdio.h>
#include <unordered_map>

struct Highest
{
	uint64_t compulsory_PC;
	uint64_t compulsory_hot;
	uint64_t capacity_PC;
	uint64_t capacity_hot;
	uint64_t coherence_PC;
	uint64_t coherence_hot;
	uint64_t conflict_PC;
	uint64_t conflict_hot;
};
struct Misses
{
	uint64_t compulsory;
	uint64_t capacity;
	uint64_t coherence;
	uint64_t conflict;
};
struct PCCountPair
{
	uint64_t PC;
	uint64_t Count;
};


std::unordered_map<uint64_t, Misses> constructHotPC(std::unordered_map<uint64_t, Misses> hotPC, int missType, uint64_t PC)
{
	Misses miss_insert;
	miss_insert.compulsory = 0;
        miss_insert.capacity = 0;
        miss_insert.coherence = 0;
        miss_insert.conflict = 0;
	
	if( (hotPC.empty()) || (hotPC.find(PC) == hotPC.end()) )
	{
	    hotPC[PC]  = miss_insert;
	}
	miss_insert = hotPC.at(PC);
	switch(missType)
	{
		case 1: miss_insert.compulsory++;
		break;
		case 2: miss_insert.capacity++;
		break;
		case 3: miss_insert.coherence++;
		break;
		case 4: miss_insert.conflict++;
		break;
		default: 
		break;
	}
	//Push updated values back into map
	hotPC[PC] = miss_insert;
	return hotPC;
}

void reportStats(std::unordered_map<uint64_t, Misses> hotPC, int k, uint64_t execTime)
{
    /*Highest report;
    report.compulsory_hot = 0;
    report.capacity_hot = 0;
    report.coherence_hot = 0;
    report.conflict_hot = 0;*/
    PCCountPair compulsoryArray[hotPC.size()];
    PCCountPair capacityArray[hotPC.size()];
    PCCountPair coherenceArray[hotPC.size()];
    PCCountPair conflictArray[hotPC.size()];
    int i = 0;
    for(auto it = hotPC.begin(); it != hotPC.end(); ++it)
    {
	PCCountPair pair;
	pair.PC = it->first;
	pair.Count = (it->second).compulsory;
	compulsoryArray[i] = pair;
	pair.Count = (it->second).capacity;
	capacityArray[i] = pair;
	pair.Count = (it->second).coherence;
	coherenceArray[i] = pair;
	pair.Count = (it->second).conflict;
	conflictArray[i] = pair;
	i++;
    }
    //Sort compulsory array
    PCCountPair temp;
    for(int j=0; j<i; j++)
    {
		for(int k=0; k<i-1; k++)
		{
			// If s[i].student_number is greater than s[i+1].student_number, swap the records
			if(compulsoryArray[k].Count < compulsoryArray[k+1].Count)
			{
				temp = compulsoryArray[k];
				compulsoryArray[k] = compulsoryArray[k+1];
				compulsoryArray[k+1] = temp;
			}
			if(capacityArray[k].Count < capacityArray[k+1].Count)
                        {
                                temp = capacityArray[k];
                                capacityArray[k] = capacityArray[k+1];
                                capacityArray[k+1] = temp;
                        }
			if(coherenceArray[k].Count < coherenceArray[k+1].Count)
                        {
                                temp = coherenceArray[k];
                                coherenceArray[k] = coherenceArray[k+1];
                                coherenceArray[k+1] = temp;
                        }
			if(conflictArray[k].Count < conflictArray[k+1].Count)
                        {
                                temp = conflictArray[k];
                                conflictArray[k] = conflictArray[k+1];
                                conflictArray[k+1] = temp;
                        }
		}
	}
	printf("***************************DETECTOR STATS******************************\n");
	printf("Top Compulsory Misses (PC, Abs count, Rate):\n");
	for(int i = 0; i < k; i++)
	{
	    printf("%lx, %lu, %.5f\n", compulsoryArray[i].PC, compulsoryArray[i].Count, (float)((float)compulsoryArray[i].Count/(float)execTime));
	}
	printf("------------------------------------------------------------------------\nTop Capacity Misses (PC, Abs count, Rate):\n");
        for(int i = 0; i < k; i++)
        {
            printf("%lx, %lu, %.5f\n", capacityArray[i].PC, capacityArray[i].Count, (float)((float)capacityArray[i].Count/(float)execTime));
        }       
	printf("------------------------------------------------------------------------\nTop Coherence Misses (PC, Abs count, Rate):\n");
        for(int i = 0; i < k; i++)
        {
            printf("%lx, %lu, %.5f\n", coherenceArray[i].PC, coherenceArray[i].Count, (float)((float)coherenceArray[i].Count/(float)execTime));
        }
	printf("------------------------------------------------------------------------\nTop Conflict Misses (PC, Abs count, Rate):\n");
        for(int i = 0; i < k; i++)
        {
            printf("%lx, %lu, %.5f\n", conflictArray[i].PC, conflictArray[i].Count, (float)((float)conflictArray[i].Count/(float)execTime));
        }
	printf("*************************************************************************\n");
        /*if( (it->second).compulsory > report.compulsory_hot )
	{
	    report.compulsory_hot = (it->second).compulsory;
	    report.compulsory_PC = it->first;
        }
	if( (it->second).capacity > report.capacity_hot )
        {
            report.capacity_hot = (it->second).capacity;
            report.capacity_PC = it->first;
        }
	if( (it->second).coherence > report.coherence_hot )
        {
            report.coherence_hot = (it->second).coherence;
            report.coherence_PC = it->first;
        }
	if( (it->second).conflict > report.conflict_hot )
        {
            report.conflict_hot = (it->second).conflict;
            report.conflict_PC = it->first;
        }*/
    /*printf("**********************DOMINANT PC, COUNT*******************\n");
    printf("Compulsory miss: %lx, %lu: \n", report.compulsory_PC, report.compulsory_hot);
    printf("Capacity miss: %lx, %lu: \n", report.capacity_PC, report.capacity_hot);
    printf("Coherence miss: %lx, %lu: \n", report.coherence_PC, report.coherence_hot);
    printf("Conflict miss: %lx, %lu: \n", report.conflict_PC, report.conflict_hot);
    printf("***********************************************************\n");*/
	       
}



