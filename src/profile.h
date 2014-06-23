#ifndef _PROFILE_H
#define _PROFILE_H

#ifdef ENABLE_PROFILE

typedef struct {
    const char* name;
    int count;
    int totcount;
    long long time;
    long long tottime;
} profile_info_t;

extern profile_info_t profile_info[];
extern unsigned int n_profiles;

static inline long long _rdtsc() {
    long long ret;
    asm __volatile__ ("rdtsc" : "=A" (ret));
    return ret;
}

profile_info_t* _getinfo(const char* name);

extern double calccpuhz();
extern void profile_init();
extern void profile_show();

#define PROBEGIN(name) static profile_info_t* __attribute__((unused)) __profile_info_##name = _getinfo(#name); \
                       long long __attribute__((unused)) __profile_begin_##name = _rdtsc()
#define PROEND(name) ({ 						\
    __profile_info_##name->time += _rdtsc()-__profile_begin_##name;	\
    __profile_info_##name->count++;					\
})
#define PROBEGINF static profile_info_t* __attribute__((unused)) __profile_info_function_ = _getinfo(__FUNCTION__);\
                  long long __attribute__((unused)) __profile_begin_function_ = _rdtsc()
#define PROENDF PROEND(function_)

// use this to get a count only (where recursiveness would cause confusing timings)
#define PROENDNT(name) ({ 						\
    __profile_info_##name->count++;					\
})
#define PROENDFNT PROENDNT(function_)
#else
#define PROBEGIN(name)
#define PROEND(name)
#define PROBEGINF
#define PROENDF
#define PROENDNT(name)
#define PROENDFNT
#endif

#endif
