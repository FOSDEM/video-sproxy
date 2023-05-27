#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main() 
{
	struct timeval tv;
	struct timespec tsp;

	gettimeofday(&tv, NULL);
	tsp.tv_sec = 0;
	tsp.tv_nsec=1000000000 - (tv.tv_usec*1000);
	nanosleep(&tsp, NULL);
	return 0;
}
