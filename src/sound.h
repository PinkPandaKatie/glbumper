/* -*- c++ -*- */

#ifndef _SOUND_H
#define _SOUND_H

#include "common.h"
#include "object.h"

#include "vector.h"
#include "resource.h"

#include <SDL/SDL_mixer.h>

#define SND_AMBIENT 6
#define SND_CHANNELS 12

class Sound : public Resource {
    friend class SoundLoader;
    friend class AmbientSound;

    Mix_Chunk* snd;

    Sound() : snd(NULL) { }

    void play(bool usepos, vect3d pos, double vol);

public: DECLARE_OBJECT;
    
    static void update_pos(vect3d pos, double ang);
    static bool enable;
    static void init();

    ~Sound();

    void play(double vol = 1.0);
    void play(vect3d pos, double vol = 1.0);

    static string loaderclass;
};

class SoundLoader : public FileResourceLoader {
public:
    SoundLoader(string dirname) : FileResourceLoader(dirname, ".wav") { }
    sptr<Resource> load_resource_from_file(string filename);
};

class AmbientSound : public Object {
    friend class Sound;
    sptr<Sound> snd;
    vect3d pos;
    double vol;
    double dist, fdist, xdist;

    bool playing;

    static inline bool compare (const AmbientSound* a, const AmbientSound* b) {
	return a->dist < b->dist;
    }

public: DECLARE_OBJECT;
    AmbientSound(Sound* snd, double vol=1.0, vect3d pos=vect3d());
    ~AmbientSound();

    void start();
    void stop();

    void setpos(vect3d p) { pos = p; }
    void setvol(double v) { vol = v; }



};


#endif
