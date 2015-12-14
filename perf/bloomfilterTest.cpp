#include "compulsory.h"
#include <stdio.h>
#include <string.h>
int main()
{
Compulsory compulsory;
bloom_filter filter = compulsory.instantiateBloomFilter();
filter.insert(10);
//compulsory.insertFilter(filter, 10);
//bool flag = false;
//flag = compulsory.checkFilter(filter, 10);
if(filter.contains(10))
	printf("Contains\n");
char* name = "MEM";
if (strcmp(name, "MEM") == 0)
	printf("Works!");
return 0;
}
