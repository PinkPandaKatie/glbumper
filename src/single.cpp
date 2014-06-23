/*
 *  Copyright (C) 2005 Jared Stafford
 *
 *  GLBumper: A clone of Micro$oft "Hover!"
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * game.cpp: a simple demonstration game
 *
 * This code is a mess.
 */

#include "common.h"
#include "graphics.h"
#include "sound.h"
#include "engine.h"
#include "game.h"
#include "tokenizer.h"
#include "profile.h"
#include "model.h"
#include "text.h"
#include "ui.h"

// for path debugging
#include "path.h"

#include "tokenizer.h"


#include <sys/time.h>
#include <stdlib.h>

#include <math.h>

#include <iostream>
#include <sstream>
#include <list>

static int debug_drawpath = 0;

void usage(char* argv0) {
    printf("usage: %s [-f] [-g wxh] [-fps limit] [world]\n"
	   "\n"
	   "-f        Use fullscreen mode\n"
	   "-g        set the window size or fullscreen resolution\n"
	   "-fps      Limit frames per second. Default is 80\n"
	   "world     Specify world to load, minus the .world extension.\n\n"
	   ,
	   argv0);
    exit(0);
}

bool tracefloor(World* w, vect3d start, Area* starta, vect3d end, Area* enda);


class ClipsFilter : public ActorFilter {
public:
    sptr<ServerActor> a;
    ClipsFilter(ServerActor* a) : a(a) { }
    virtual bool filter(Actor* t) {
	if (t==a) return false;
	if (a->clippedby(static_cast<ServerActor*>(t))) return true;
	return false;
    }
};



class SingleCommunicator : public Communicator {
public:
    SingleCommunicator(ServerActor* a) : Communicator(a) { }


    void playsound(const char* snd, double vol) { 
	sptr<Sound> s = Resource::get<Sound>(snd);
	if (s == NULL) return;
	s->play(vol);
    }
    void playsound(const char* snd, vect3d loc, double vol) { 
	sptr<Sound> s = Resource::get<Sound>(snd);
	if (s == NULL) return;
	s->play(loc, vol);
    }
    void set_client_class(string clas) { 
	set_appearance_class(a, clas);
    }
    void setparameter(int num, string sval) {
	if (a->appearance == NULL) return;
	Parameter& p(a->appearance->params[num]);
	p.type = PT_STRING;
	p.sval = sval;
	a->appearance->paramchanged(num);
    }
    void setparameter(int num, ParamType type, int ival) {
	if (a->appearance == NULL) return;
	Parameter& p(a->appearance->params[num]);
	p.type = type;
	p.ival = ival;
	p.dval = (double)ival;
	a->appearance->paramchanged(num);
	
    }
    void setparameter(int num, ParamType type, double dval) {
	if (a->appearance == NULL) return;
	Parameter& p(a->appearance->params[num]);
	p.type = type;
	p.ival = (int)dval;
	p.dval = dval;
	a->appearance->paramchanged(num);
    }

    DECLARE_OBJECT;
};
#define VEL(lk,hk,var)							\
		case SDLK_##lk:						\
		    var##_l=(evt.type==SDL_KEYDOWN?1:0);		\
		    var=((var##_h?1:0)+(var##_l?-1:0)); return true;	\
		case SDLK_##hk:						\
		    var##_h=(evt.type==SDL_KEYDOWN?1:0);		\
		    var=((var##_h?1:0)+(var##_l?-1:0)); return true;

#define VEL2(lk,alk,hk,ahk,var)						\
		case SDLK_##lk:						\
		case SDLK_##alk:					\
		    var##_l=(evt.type==SDL_KEYDOWN?1:0);		\
		    var=((var##_h?1:0)+(var##_l?-1:0)); return true;	\
		case SDLK_##hk:						\
		case SDLK_##ahk:					\
		    var##_h=(evt.type==SDL_KEYDOWN?1:0);		\
		    var=((var##_h?1:0)+(var##_l?-1:0)); return true;





class SingleGameComponent : public Component, public HovercraftCtl {
public:
    SingleGameComponent(string worldname);
    ~SingleGameComponent();

    bool event(SDL_Event& evt);
    void tick(double time);
    void draw(int x, int y);

    // HovercraftCtl implementation
    int forward_accel() { return vf; }
    int strafe_accel() { return vs; }
    int vert_accel() { return vz; }
    int rotvel() { return vr; }
    bool horn() { if (horn_) { horn_ = false; return true; } return false; }
    bool _dbgEnableBounce() { return bounce && !ghost; }
    bool _dbgEnableGrav() { return falls && !ghost; }
    bool _dbgNoclip() { return ghost; }
    int _dbgShiftMul() { return shiftmul; }

    int showinfo;
    sptr<Label> info;

    int vf, vf_h, vf_l;
    int vz, vz_h, vz_l;
    int vr, vr_h, vr_l;
    int vs, vs_h, vs_l;

    int vvd, vvd_h, vvd_l;
    int vvh, vvh_h, vvh_l;

    int vd2, vd2_h, vd2_l;

    double vdist;
    double vheight;

    bool bounce,falls,ghost,horn_;

    bool profile;

    vect3d viewpt;
    double vofs;
    sptr<Area> viewarea;

    int camera;
    vect3d camerapt;

    vect3d ghostpt;
    sptr<Area> cameraarea;
    double camvofs;
    double camang;

    int framecnt;

    int dostep;
    int step;
    int rearview;
    double speedmul;

    int shiftmul;
    int shift;
    int superspeed;

    vect3d tstartpt;
    sptr<Area> tstartarea;
    vect3d tendpt;
    sptr<Area> tendarea;

    int rwf;

    sptr<World> world;

    sptr<Actor> player;

    DECLARE_OBJECT;
};

SingleGameComponent::SingleGameComponent(string worldname) {
    showinfo = false;
#ifdef ENABLE_PROFILE
    profile = false;
    profile_init();
#endif

    vf = 0; vf_h = 0; vf_l = 0;
    vz = 0; vz_h = 0; vz_l = 0;
    vr = 0; vr_h = 0; vr_l = 0;
    vs = 0; vs_h = 0; vs_l = 0;

    vvd = 0; vvd_h = 0; vvd_l = 0;
    vvh = 0; vvh_h = 0; vvh_l = 0;
    vd2 = 0; vd2_h = 0; vd2_l = 0;

    vdist = 0; vheight = 0;

    vofs = 0;

    viewarea = NULL;

    camera = 0; camvofs = 0; camang = 0;
    cameraarea = NULL;

    dostep = 0; step = 0;

    rearview = 0;

    shift = 2;

    superspeed = 0;
    speedmul = 1.0;

    framecnt = 0;

    tstartarea = NULL;
    tendarea = NULL;

    rwf = 0;

    horn_ = false;
    bounce = true;
    falls = true;
    ghost = false;

    world = NULL;

    info = GC::track(new Label(color::Clear));


    vector< sptr<Actor> > startpts;

    GameActorLoader loader(startpts);

    DecorationLoader dloader(loader);

    printf("Loading world... "); fflush(stdout);

    Uint32 begin = SDL_GetTicks ();

    try {
	world = GC::track(new World (join_path(FileResourceLoader::get_data_dir(), 
				    "worlds", worldname + ".world.gz").c_str(), dloader));
    } catch (std::exception& wtf) {
	world = GC::track(new World (join_path(FileResourceLoader::get_data_dir(), 
				    "worlds", worldname + ".world").c_str(), dloader));
    }
    GC::collect();

    Uint32 end = SDL_GetTicks ();
    printf(" %d ms\n",end-begin); fflush(stdout);

    player = loader.create_hovercraft(this);

    if (startpts.empty()) {
	player->loc = vect3d();
	player->area = world->areas[0];
	player->ang = 0;
	ghost = true;

    } else {
	Actor* startpt = startpts[0];
	
	player->loc = startpt->loc;
	player->area = startpt->area;
	player->ang = startpt->ang;
    }

    startpts.clear();
    world->add(player);


}
SingleGameComponent::~SingleGameComponent() {
}
bool SingleGameComponent::event(SDL_Event& evt) {
    switch (evt.type) {
    case SDL_KEYDOWN:
	switch (evt.key.keysym.sym) {
	case SDLK_w:
	    drawmode++;
	    if (drawmode > 4)
		drawmode = 0;
	    return true;
	case SDLK_r:
	    rwf++;
	    if (rwf > 4)
		rwf = 0;
	    return true;
	case SDLK_t:
	    superspeed ^= 1;
	    return true;
	case SDLK_z:
	    rearview ^= 1;
	    return true;
	case SDLK_c:
	    nowalls ^= 1;
	    return true;
	case SDLK_v:
	    nowalls ^= 2;
	    return true;
	case SDLK_x:

	    step ^= 1;

	    return true;
	case SDLK_RETURN:
	    if (evt.key.keysym.mod & KMOD_ALT)
		setmode(screenw, screenh, !screen_fs);
	    break;
	case SDLK_SPACE:
	    horn_ = true;
	    cout << "------- "<<player->loc.x << ' ' << player->loc.y << ' ' << player->loc.z << ' ' << player->area->name<<" --------------"<<endl;
	    dostep = 1;
#ifdef TRACEREFS
	    _dbg_object = NULL;
#endif
	    return true;

	case SDLK_b:
		    
	    bounce ^= 1;
		    
	    return true;
	case SDLK_k:
	    camera += 1;
	    if (camera>2) camera=0;
	    if (cameraarea == NULL) {
	    case SDLK_i:
		camerapt=viewpt;
		cameraarea=viewarea;
		camang = player->ang;
		camvofs=vofs;
	    }
	    return true;
	case SDLK_n:
	    debug_drawpath ^= 1;
	    return true;
	case SDLK_o:
	    if (speedmul == 1.0) speedmul=10.0;
	    else speedmul = 1.0;
	    return true;

	case SDLK_TAB:
	    showinfo++;
	    if (showinfo == 3) showinfo = 0;
	    if (showinfo) {
		add(info, 0, 0);
	    } else {
		remove(info);
	    }
	    return true;
	case SDLK_1:
	    tstartpt = player->loc;
	    tstartarea = player->area;
	    cout <<"start pos: "<< tstartpt << ':' << tstartarea->str() <<endl;
	    return true;
	case SDLK_2:
	    tendpt = player->loc;
	    tendarea = player->area;
	    cout <<"End pos: "<< tendpt<<endl;
	    {

		sptr<Area> ar = world->findarea(tstartpt,tstartarea,tendpt);
                if (ar != NULL) 
                    cout <<"result area = "<<ar->str() << endl;
                else
                    cout <<"failed"<<endl;
                //"tracefloor "<<(tfret?"succeeded":"failed")<<endl;

			

	    }
	    return true;
	case SDLK_3:
	    cout << "Player is in:";
	    FOREACH (itr,player->areas) {
		cout << ' '<<(*itr)->name;
	    }
	    cout<<endl;
	    return true;
	case SDLK_7:
	    _game_debug ^= 1;
	    return true;
	case SDLK_8:
	    ghost ^= 1;
	    if (ghost) {
		ghostpt = player->loc;
	    } else {
		sptr<Area> newarea = NULL;
		FOREACH(itr, world->areas) {
		    sptr<Area> a = *itr;
		    if (a->pointin(player->loc)) {
			double fz = a->floor->getzbound(a,player);
			double cz = a->ceil->getzbound(a,player);
			if (player->loc.z >= fz && player->loc.z <= cz) {
			    newarea = a;
			    break;
			}
		    }
		}
		if (newarea != NULL) {
		    player->area = newarea;
		    player->checkareas();
		} else {
		    player->loc = ghostpt;
		}
	    }
	    return true;
	case SDLK_9:
	    draweverything ^= 1;
	    return true;
#ifdef ENABLE_PROFILE
	case SDLK_p:
	    profile^=1;
	    printf("\033[H\033[2J");
	    return true;
#endif
	case SDLK_g:
	    falls ^= 1;
	    return true;
	case SDLK_LEFTBRACKET:
	    if (!shift)
		debug_tweak--;
	    printf ("tweak is %d\n", debug_tweak);
	    return true;
	case SDLK_RIGHTBRACKET:
	    if (!shift)
		debug_tweak++;
	    printf ("tweak is %d\n", debug_tweak);
	    return true;

	case SDLK_RSHIFT:
	    shift ^= 2;
	    return true;


	case SDLK_ESCAPE:
	    if (parent != NULL)
		parent->remove(this);
	    return true;
	default: // get rid of "enumeration value not handled" warning
	    break;
	}
	// fallthrough
    case SDL_KEYUP:
	switch (evt.key.keysym.sym) {
	    VEL (LEFTBRACKET, RIGHTBRACKET, vd2);
	    VEL (d, e, vz);
	    VEL (f, s, vs);
	    VEL2 (RIGHT, KP6, LEFT, KP4, vr);
	    VEL2 (DOWN, KP2, UP, KP8, vf);
	    VEL (y, h, vvd);
	    VEL (j, u, vvh);
	case SDLK_LSHIFT:
		    
	    shift &= ~1;
	    shift |= evt.type == SDL_KEYDOWN;
	    return true;
	default:
	    return true;
	}
	return false;
    case SDL_QUIT:
	if (parent != NULL)
	    parent->remove(this);
	return true;
    }
    return false;
}

void SingleGameComponent::tick(double deltatime) {
    if (!step || dostep) {
	int shiftval = (shift ^ (shift>>1))&1;
	shiftmul = (shiftval ? 3 : 1)*(superspeed?20:1);
	if (shiftval && vd2) {
	    debug_tweak += vd2;
	    printf ("tweak is %d\n", debug_tweak);
	}

	world->remove_stale_actors();
	PROBEGIN(move_actors);
	world->moveactors(deltatime*speedmul);
	PROEND(move_actors);

	FOREACH(a, world->pendingactors) {
	    sptr<ServerActor> sa = a->cast<ServerActor>();
	    if (sa != NULL)
		sa->comm = GC::track(new SingleCommunicator(sa));
	}
	world->add_pending_actors();
	FOREACH(itr,world->actors) {
	    if ((*itr)->appearance != NULL)
		(*itr)->appearance->runsound();
	}

	Sound::update_pos(player->loc,player->ang);
	vdist += vvd*shiftmul*deltatime;
	vheight += vvh*shiftmul*deltatime;

	viewpt = player->loc;
	viewpt.z += 8 * MASTER_SCALE;
	double s = sin (player->ang);
	double c = cos (player->ang);

	vofs=0;
	
	viewarea=player->area;
	if (vdist>0) {
	    vect3d ovpt(viewpt);
	    vect3d visofs(-c*vdist,-s*vdist,vheight);

	    vect3d tv2(viewpt+visofs);

	    ClipsFilter filter(player.statcast<ServerActor>());
	    if (ghost) {
		viewpt = tv2;
	    } else {
		viewpt = world->trace(viewpt, player->area, tv2, filter);
		viewpt -= player->rad*vnormal(visofs);
		if ((viewpt-ovpt)*visofs<0) viewpt=ovpt;
		viewarea=world->findarea(ovpt, player->area,viewpt);
	    }
	    
	    vofs=-vheight/vdist;
	}
    }

    if (showinfo) {


	info->layout().clear();
	info->layout().shadow(color::Black)
	    .font(Resource::get<Font>("default16"))
	    << player->loc.x << ' ' << htab(100) 
	    << player->loc.y << ' ' << htab(200)
	    << player->loc.z << htab(300) << '\n';
	FOREACH (itr,player->areas) {
	    info->layout() << (*itr)->name << ' ';
	}
	info->layout() 
	    << '\n' 
	    << color::LtBlue << "Real FPS: " 
	    << color::White << (1.0 / deltatime) << '\n';

	if (showinfo > 1)
	    GC::printstats(info->layout().stream());

    }
    dostep = 0;
#ifdef ENABLE_PROFILE
    if (profile) {
	profile_show();
    }
#endif
}

void SingleGameComponent::draw(int x, int y) {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDepthMask(1);
    glStencilMask(0xff);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0);

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable (GL_POLYGON_SMOOTH);
    glDisable (GL_BLEND);
    glDisable (GL_ALPHA_TEST);
    glEnable (GL_DEPTH_TEST);
    glEnable (GL_TEXTURE_2D);
    glEnable (GL_CULL_FACE);
    glFrontFace (GL_CCW);
    glDepthFunc (GL_LEQUAL);
    glDisable (GL_LINE_SMOOTH);

    glDisable (GL_FOG);
    CHECKERROR();

    vect3d tviewpt(viewpt);
    sptr<Area> tviewarea(viewarea);
    double tvofs(vofs);
    double tvang(player->ang);
    Actor* eyeactor=player;
    Actor* rveyeactor=player;
    if (vdist>0)
	eyeactor=NULL;
    if (camera==1) {
	tviewpt=camerapt;
	tviewarea=cameraarea;
	tvofs=camvofs;
	tvang=camang;
	eyeactor=NULL;
    }
    if (camera && vdist>0)
	rveyeactor=NULL;

    if (ghost) {
	draweverything |= 2;
	eyeactor=player;
    } else {
	draweverything &= ~2;
    }


    if (vdist < 0) vdist = 0;

    ExcludeOneFilter drawfilter(eyeactor);

    double yfov = 0.75;
    double xfov=yfov*((double)screenw)/screenh;


    PROBEGIN(draw_view);
    world->drawview (tviewpt, ghost ? NULL : tviewarea, tvang, tvofs, xfov, yfov, drawfilter);
    PROEND(draw_view);

    if (rearview && !ghost) {
	int rvw=screenw/3;
	int rvh=rvw/2;
	int rvx=(screenw-rvw)/2;
	int rvy=(screenh/40);


	glViewport (rvx, screenh - rvh - rvy, rvw, rvh);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0, 1, 1, 0);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	
	glDisable(GL_DEPTH_TEST);

	glColor3f(0,0,0);
	glBegin(GL_QUADS);
	glVertex2d(0,0); glVertex2d(0,1);
	glVertex2d(1,1); glVertex2d(1,0);
	glEnd();

	glEnable(GL_DEPTH_TEST);

	glDepthMask(1);
	glClear (GL_DEPTH_BUFFER_BIT);

	double xfov=0.6;
	double yfov=xfov*((double)rvh)/rvw;

	int twf=drawmode;
	drawmode=rwf;

	ExcludeOneFilter rvdrawfilter(rveyeactor);
	PROBEGIN(draw_rearview);
	if (camera)
	    world->drawview (viewpt, viewarea, player->ang, vofs, xfov, yfov, rvdrawfilter);
	else
	    world->drawview (viewpt, viewarea, player->ang+M_PI, -vofs, -xfov, yfov, rvdrawfilter);
	PROEND(draw_rearview);

	drawmode=twf;

	glViewport (0,0, screenw, screenh);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0, screenw, screenh, 0);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);
	glColor3f(1,1,1);
	glBegin(GL_LINE_LOOP);

	glVertex2d(rvx,rvy);
	glVertex2d(rvx+rvw,rvy);
	glVertex2d(rvx+rvw,rvy+rvh);
	glVertex2d(rvx,rvy+rvh);

	glEnd();
    }

    glPopAttrib();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

#include objdef(SingleGameComponent)
#include objdef(SingleCommunicator)

int main (int argc, char **argv) {
    int fullscreen = 0;
    int w = 640, h = 480;
    int targc = argc - 1;
    char **targv = argv + 1;
    char *worldname = "demo";
    
    int maxfps = 80;

    const char* datadir = getenv("GLBUMPER_DATA");
    if (!datadir) {
	datadir = ".";
    }


    srand(time(NULL));

    while (targc) {
	if (**targv == '-') {
	    if (!strcmp (*targv + 1, "f")) {
		fullscreen = 1;
	    } else if (!strcmp (*targv + 1, "nosound")) {
		Sound::enable = false;
	    } else if (!strcmp (*targv + 1, "D")) {
		if (!targc--) usage(argv[0]);
		datadir = *++targv;
	    } else if (!strcmp (*targv + 1, "g")) {
		int ww, hh;
		if (!targc--) usage(argv[0]);
		if (sscanf (*++targv, "%dx%d", &ww, &hh) == 2) {
		    w = ww;
		    h = hh;
		}
	    } else if (!strcmp (*targv + 1, "fps")) {
		if (!targc--) usage(argv[0]);
		maxfps=atoi(*++targv);
	    } else {
		usage(argv[0]);
	    }
	} else {
	    worldname = *targv;
	}
	targc--;
	targv++;
    }

    int minframetime = 1000/maxfps;
    unsigned int mintime=0;

    int retval = 0;
    double ctime = 0.0, ltime;
    
    SDL_Event evt;

    SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);

    setmode(w, h, fullscreen);

    init_engine();
    Sound::init();

    FileResourceLoader::set_data_dir(datadir);

    ResourceLoader::registerloader("texture", new TextureLoader("textures"));
    ResourceLoader::registerloader("model", new ModelLoader("models"));
    ResourceLoader::registerloader("font", new FontLoader("fonts"));
    if (Sound::enable)
	ResourceLoader::registerloader("sound", new SoundLoader("sounds"));

    Font::default_font = Resource::get<Font>("default16");

    Uint32 tticks;

    sptr<Component> gamecomp = GC::track(new SingleGameComponent(worldname));
    //gamecomp->MakeStatic();
    //gctmp->ref();

    screenw = w;
    screenh = h;

    sptr<RootComponent> root = GC::track(new RootComponent());
    root->add(gamecomp);
    //root->MakeStatic();
    gamecomp->grabfocus();

    try {
	while (!root->children.empty()) {
	    while ((tticks=SDL_GetTicks())<mintime) {
		SDL_Delay(mintime-tticks);
	    }
            SDL_Delay(0);
	    mintime = tticks + minframetime;
	    ltime=ctime;
	    ctime=gettime();
	    double deltatime = ctime - ltime;
	    if (deltatime > 0.1) deltatime = 0.1;
	
	    while (SDL_PollEvent (&evt)) {
		root->event(evt);
	    }
	    root->tick(deltatime);
	
	    glDepthMask(1);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	    glDepthMask(0);
	
	    glMatrixMode(GL_MODELVIEW);
	    glLoadIdentity();
	
	    glMatrixMode(GL_PROJECTION);
	    glLoadIdentity();
	    gluOrtho2D (0, screenw, screenh, 0);
	
	    root->render(0, 0);
	    CHECKERROR();
	    SDL_GL_SwapBuffers();
	    
	    // Create and destroy an Object to bump the garbage
	    // collector's counter.
	    sptr<Object> tmpobject = GC::track(new Object());
	}
    } catch (std::exception& te) {
	cout << "\nUnexpected error: " << te.what() << "\n";
    }

    gamecomp = NULL;
    root = NULL;    

    cout << "\nBefore collection:\n";
    GC::printstats();
    GC::collect();
    cout << "After collection:\n";
    GC::printstats();
    GC::printobjects();

    SDL_Quit ();
    return retval;
}
