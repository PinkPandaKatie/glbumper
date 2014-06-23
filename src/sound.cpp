
#include "common.h"
#include "profile.h"
#include "sound.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>

vector< weakptr<AmbientSound> > ambient_sounds;

string Sound::loaderclass = "sound";

bool Sound::enable = true;

class SoundChannel {
public:
    sptr<Sound> snd;
    double mvol;
    double lvol, rvol;
    bool usepos, started;
    vect3d pos;
};

SoundChannel snd_channels[SND_CHANNELS];

Sound::~Sound() {
    if (snd)
	Mix_FreeChunk(snd);
}

#define ATTEN_DIST 40.0
vect3d lpos;

void Sound::play(bool usepos, vect3d pos, double vol) {
    if (!enable) return;

    double dist = vlen(pos - lpos);
    if (usepos && dist > ATTEN_DIST) {
	return;
    }
    SDL_LockAudio();

    int chan = Mix_GroupAvailable(0);
    if (chan == -1) {
	chan = Mix_GroupOldest(0);
	SDL_UnlockAudio();
	Mix_HaltChannel(chan);
	SDL_LockAudio();
    }

    if (snd_channels[chan].snd != NULL) {
	//cout << "WTF? can't happen"<<endl;
    }
    snd_channels[chan].mvol = vol;
    snd_channels[chan].snd = this;
    snd_channels[chan].usepos = usepos;
    snd_channels[chan].pos = pos;
    snd_channels[chan].started = false;
    
    SDL_UnlockAudio();
}

void Sound::play(double vol) {
    play(false, vect3d(), vol);
}

void Sound::play(vect3d pos, double vol) {
    play(true, pos, vol);
}

static void channel_finished(int channel) {
    if (snd_channels[channel].snd != NULL) {
	snd_channels[channel].snd = NULL;
    }
}

void calc_vol(double dist, double fdist, double xdist, double& lvol, double& rvol) {

    double atten = 1.0 - (dist / ATTEN_DIST);
    if (atten < 0) {
	lvol = rvol = 0.0;
	return;
    }
    double bal = xdist / (ATTEN_DIST/2);
    fdist /= (ATTEN_DIST);

    if (fdist > 1.0)
	fdist = 1.0;
    if (fdist < -1.0)
	fdist = -1.0;

    double bal_mul = (dist / (ATTEN_DIST/8)) + 0.125;

    if (bal_mul > 1.0)
	bal_mul = 1.0;

    fdist = (fdist * bal_mul);
    bal = (bal * bal_mul);

    if (bal > 1.0)
	bal = 1.0;
    if (bal < -1.0)
	bal = -1.0;

    lvol = (1.0 - (bal * 0.75)) + fdist;
    rvol = (1.0 - (-bal * 0.75)) + fdist;

    lvol *= atten;
    rvol *= atten;

    //cout << rvol << ' ' << lvol << ' ' << bal << endl;
    if (lvol > 1.0) lvol = 1.0;
    if (lvol < 0.0) lvol = 0.0;
    if (rvol > 1.0) rvol = 1.0;
    if (rvol < 0.0) rvol = 0.0;

}

AmbientSound::AmbientSound(Sound* s, double vol, vect3d pos) 
    : snd(s), pos(pos), vol(vol), playing(false) { }
AmbientSound::~AmbientSound() { }

#include objdef(Sound)
#include objdef(AmbientSound)

void AmbientSound::start() {
    if (playing)
	return;

    ambient_sounds.push_back(this);
}
void AmbientSound::stop() {
    if (!playing) 
	return;

    FOREACH(itr, ambient_sounds) {
	sptr<AmbientSound> snd = itr->getref();
	if (snd == this) {
	    ambient_sounds.erase(itr);
	    return;
	}
    }
}

void Sound::update_pos(vect3d pos, double ang) {
    if (!enable) return;
    SDL_LockAudio();

    lpos = pos;

    double c = cos(ang);
    double s = sin(ang);
    vect2d j(c, s);
    for (int i = 0; i < SND_AMBIENT; i++) {
	SoundChannel& cc(snd_channels[i]);
	if (cc.snd == NULL) continue;
	if (cc.usepos) {
	    double dist = vlen(cc.pos - pos);
	    double fdist = j*(cc.pos - pos);
	    double xdist = ~j*(cc.pos - pos);
	    calc_vol(dist, fdist, xdist, cc.lvol, cc.rvol);

	} else {
	    cc.lvol = cc.rvol = 1.0;
	}
    }

    // We don't want to lose the sounds as we're playing them, so
    // create a vector of smart pointers from the weak pointers.
    vector< sptr<AmbientSound> > asounds;

    FOREACH(itr, ambient_sounds) {
	sptr<AmbientSound> a = itr->getref();
	if (a == NULL) {
	    ambient_sounds.erase(itr--);
	    continue;
	}

	asounds.push_back(a);

	a->dist = vlen(a->pos - pos);
	a->fdist = j*(a->pos - pos);
	a->xdist = ~j*(a->pos - pos);
    }
    sort(asounds.begin(), asounds.end(), AmbientSound::compare);

    int achan_used = 0;
    for (int chan = SND_AMBIENT; chan < SND_CHANNELS; chan++) {
	snd_channels[chan].lvol = 0;
	snd_channels[chan].rvol = 0;
	snd_channels[chan].mvol = 1.0;
    }
    
    FOREACH(itr, asounds) {
	AmbientSound* a = *itr;

	double lvol, rvol;
	int chan;
	calc_vol(a->dist, a->fdist, a->xdist, lvol, rvol);
	if (!lvol && !rvol) continue;
	lvol *= a->vol;
	rvol *= a->vol;
	for (chan = SND_AMBIENT; chan < SND_CHANNELS; chan++) {
	    if (snd_channels[chan].snd == a->snd) break;
	}
	if (chan == SND_CHANNELS) {
	    chan = Mix_GroupAvailable(1);
	    if (chan == -1) continue;
	    snd_channels[chan].snd = a->snd;
	    snd_channels[chan].started = false;
	}
       
	snd_channels[chan].lvol += lvol;
	snd_channels[chan].rvol += rvol;
	achan_used |= (1<<chan);
    }
    SDL_UnlockAudio();
    for (int chan = SND_AMBIENT; chan < SND_CHANNELS; chan++) {
	if (!(achan_used<<chan)) {
	    Mix_HaltChannel(chan);
	}
    }
    for (int i = 0; i < SND_CHANNELS; i++) {
	SoundChannel& cc(snd_channels[i]);
	if (cc.snd != NULL && !cc.started) {
	    cc.started = true;
	    Mix_PlayChannel(i, cc.snd->snd, (i >= SND_AMBIENT ? -1 : 0));
	}
	int rvol = (int)((cc.lvol*cc.mvol) * 255);
	int lvol = (int)((cc.rvol*cc.mvol) * 255);
	if (lvol > 255) lvol = 255;
	if (rvol > 255) rvol = 255;
	
	Mix_SetPanning(i, lvol, rvol);
    }
    
}

sptr<Resource> SoundLoader::load_resource_from_file(string filename) {
    Mix_Chunk* mc = Mix_LoadWAV(filename.c_str());
    if (!mc) return NULL;
    sptr<Sound> s = GC::track(new Sound());
    s->snd = mc;
    return s;
}

void Sound::init() {
    if (!enable) return;
    Mix_OpenAudio(44100,MIX_DEFAULT_FORMAT,2,512);
    
    Mix_AllocateChannels(SND_CHANNELS);

    for (int i = 0; i < SND_AMBIENT; i++) {
	Mix_GroupChannel(i, 0);
    }
    for (int i = SND_AMBIENT; i < SND_CHANNELS; i++) {
	Mix_GroupChannel(i, 1);
    }

    Mix_ChannelFinished(channel_finished);

}
