#include "profile.h"
#ifdef ENABLE_PROFILE
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cstdio>

profile_info_t profile_info[128];
unsigned int n_profiles=0;
double cpuhz;

double calccpuhz() {
    long long begin,end;
    struct timeval tvb,tve;
    gettimeofday(&tvb,NULL);
    begin=_rdtsc();
    sleep(2);
    end=_rdtsc();
    gettimeofday(&tve,NULL);
    return (double)((end-begin)/((tve.tv_sec-tvb.tv_sec)*1000000 + (tve.tv_usec-tvb.tv_usec)))*1000000.0;
}

profile_info_t* _getinfo(const char* name) {
    unsigned int i;
    for (i=0;i<n_profiles;i++) {
	if (!strcmp(name,profile_info[i].name)) return &profile_info[i];
    }
    profile_info[n_profiles].name = name;
    profile_info[n_profiles].time = 0;
    profile_info[n_profiles].tottime = 0;
    profile_info[n_profiles].count = 0;
    profile_info[n_profiles].totcount = 0;
    return &profile_info[n_profiles++];
}

void profile_init() {
    printf("\033[H\033[2J");
    fflush(stdout);
    cpuhz = calccpuhz();
}

void profile_show() {
    int i;

    printf("\033[HFRAME TIMES:\033[K\n");
    for (i = 0; (unsigned)i < n_profiles; i++) {
	profile_info_t* prof = &profile_info[i];
	
	double secs=(prof->time/cpuhz);
	double avg=secs/prof->count;
	printf("%25s: %20.15fs %10dc %20.15fs avg\033[K\n",prof->name,secs,prof->count,avg);
	secs=(prof->tottime/cpuhz);
	avg=secs/prof->totcount;
    }
    
    printf("TOTALS:\033[K\n");
    for (i = 0; (unsigned)i < n_profiles; i++) {
	profile_info_t* prof = &profile_info[i];
	
	double secs=(prof->time/cpuhz);
	double avg=secs/prof->count;
	secs=(prof->tottime/cpuhz);
	avg=secs/prof->totcount;
	printf("%25s: %20.15fs %10dc %20.15fs avg\033[K\n",prof->name,secs,prof->totcount,avg);
	prof->tottime += prof->time;
	prof->time = 0;
	prof->totcount += prof->count;
	prof->count = 0;
    }
    fflush(stdout);
}

#endif
