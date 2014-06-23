// -*- c++ -*-

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_endian.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <math.h>


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
//necessary for gl.h

// grr... Even with WIN32_LEAN_AND_MEAN, windows.h still defines a lot
// of shit we don't need and interferes with our macros and types.
#define TokenType fooTokenType
#include <windows.h>

#undef TokenType
#undef registerclass
#undef sendmessage

#endif

// The Win32 headers define INFINITY as 1.0 / 0.0. This causes an ugly
// warning when cross-compiling under MinGW.

#if defined(WIN32) && defined(__GNUC__)
#undef INFINITY
static const double INFINITY = 0x1.0p32767L;
#endif

using namespace std;

#if SDL_BYTE_ORDER==SDL_LITTLE_ENDIAN
static inline Uint16 SwapBig16(Uint16 x) {
    return ((x>>8) | (x<<8));
}
static inline Uint32 SwapBig32(Uint32 x) {
    return
	((x >> 24) & 0x000000ff) |
	((x >>  8) & 0x0000ff00) |
	((x <<  8) & 0x00ff0000) |
	((x << 24) & 0xff000000);
}
static inline Uint64 SwapBig64(Uint64 x) {
    return
	((x >> 56) & 0x00000000000000ffULL) |
	((x >> 40) & 0x000000000000ff00ULL) |
	((x >> 24) & 0x0000000000ff0000ULL) |
	((x >>  8) & 0x00000000ff000000ULL) |
	((x <<  8) & 0x000000ff00000000ULL) |
	((x << 24) & 0x0000ff0000000000ULL) |
	((x << 40) & 0x00ff000000000000ULL) |
	((x << 56) & 0xff00000000000000ULL);

}

#else

#if SDL_BYTE_ORDER!=SDL_BIG_ENDIAN
#error "Your system isn't big or little endian. WTF is it?"
#endif

#define SwapBig16(X) (X)
#define SwapBig32(X) (X)
#define SwapBig64(X) (X)



#endif

#define ISSET(map,index) (((map)[(index)>>3]&(1<<((index)&7)))?1:0)
#define SETBIT(map,index) (map)[(index)>>3]|=(1<<((index)&7))
#define CLEARBIT(map,index) (map)[(index)>>3]&=~(1<<((index)&7))

#define FOREACH(itr,vect) for (typeof((vect).begin()) itr = (vect).begin(); itr != (vect).end(); ++itr)

#define FILTER(itr, coll, xcode, pred, xecode)					\
for (typeof((coll).begin()) itr = (coll).begin(); itr != (coll).end();) {	\
    typeof((coll).begin()) next_itr = itr;					\
    next_itr++;									\
    xcode;									\
    if(!(pred)) {								\
        xecode;									\
	(coll).erase(itr);							\
    }										\
    itr = next_itr;								\
}
#define IN_SET(set, val) ((set).find(val) != (set).end())

#ifdef unix

#include <time.h>
#include <sys/time.h>

static inline double gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

/*static void dsleep(double time) {
    struct timespec ts;
    ts.tv_sec = (int)(time);
    ts.tv_nsec = (int)((time - ts.tv_sec) * 1000000000.0);
    nanosleep(&ts, NULL);
    }*/
#else

static inline double gettime() {
    return SDL_GetTicks() / 1000.0;
}

#endif
static void __attribute__((used))  dsleep(double time) {
    SDL_Delay((int)(time*1000.0));
}

static inline double sqr(double x) { return x*x; }

static inline int powerof2 (int in) {
    int i = 0;
    in--;
    while (in) {
	in >>= 1;
	i++;
    }
    return 1 << i;
}

static inline double normalize_angle(double ang) {
    double t = ang/(M_PI*2);
    return (t-floor(t))*(M_PI*2);
}    
static inline double normalize_angle(double ang, double relto) {
    relto -= M_PI/2;
    double t = (ang-relto)/(M_PI*2);
    return (t-floor(t))*(M_PI*2)+relto;
}    

#endif
