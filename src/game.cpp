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
 *
 */


#include "common.h"
#include "engine.h"
#include "tokenizer.h"
#include "profile.h"
#include "game.h"
#include "path.h"

#include <iostream>
#include <sstream>
#include <list>

#define FWVEL (200.0)
#define TURNVEL ((M_PI/56.0)*40.0)
#define abs fabs

#include objdef(BaseActor)
bool _game_debug = false;

class BaseActor;
typedef sptr<BaseActor> (*CreateFunc)();

static map<string, CreateFunc> classes;

static int register_create_func(const char* s, CreateFunc cf) {
    classes.insert(pair<string, CreateFunc>(string(s), cf));
    return 0;
}

#define REGISTER_CLASS(name, clas)                                              \
static sptr<BaseActor> Create##clas() {                                         \
    return GC::track(new clas());                                               \
}                                                                               \
static int __dummy_create_##clas = register_create_func(#name, Create##clas)

void __bkpt() {
}


/*
 *
 * Water
 *
 */

class Water : public BaseActor {
public: DECLARE_OBJECT;

    string texn;

    Water() {
	is_static = true;
	height=0;
	rad=0;
	stepheight=0;
    }

    static double getlevel(Area* area) {
	FOREACH (itr, area->staticactors) {
	    Water* w = itr->cast<Water>();
	    if (w != NULL && w->area == area) return w->loc.z;
	}
	return -INFINITY;
    }
    bool clippedby(ServerActor* o) {
	return false;
    }
    
    ~Water() {
    }
    CLASSN(Water);
};
REGISTER_CLASS(water, Water);
#include objdef(Water)


bool tracefloor(World* w, vect3d start, Area* starta, vect3d end, Area* enda) {

    unsigned char wseen[w->wall_bit_size];
    memset (wseen, 0, w->wall_bit_size);

    sptr<Area> area = starta;
    sptr<Wall> lwall = NULL;
    vect2d v1 = start;
    vect2d v2 = end;
    vect2d norm(vnormal(v2-v1));
    
    while (1) {
	sptr<Wall> hitw = NULL;
	vect2d hitpt;
	double mindist = INFINITY;
	FOREACHW(w, area) {
	    if (ISSET(wseen, w->id)) continue;
	    if (lwall != NULL && (lwall->v1 == w->next->v1 && lwall->next->v1 == w->v1)) {
		SETBIT(wseen, w->id);
		continue;
	    }
	    if (norm*~w->j <= 0) continue;
	    vect2d hit;
	    if (w->intersect (v1, v2, hit)) {
		double tdist = vlen2(hit-v1);
		if (tdist<mindist) {
		    mindist=tdist;
		    hitw=w;
		    hitpt=hit;
		}
	    }
	}
	if (hitw == NULL) {
	    break;
	}
	double hz = area->floor->getz(hitpt)+0.265625;

	double maxz = -INFINITY;

	sptr<Area> nextarea=NULL;

	FOREACH(itr, hitw->links) {
	    if (itr->flags&1) continue;
	    sptr<Area> narea = itr->a;
	    
	    double oz = narea->floor->getz(hitpt);

	    double wlevel = Water::getlevel(narea);
	    if (wlevel > oz) oz = wlevel;

	    double cz = narea->ceil->getz(hitpt);
	    if (oz >= maxz && oz <= hz && (hz+0.546845) <= cz) {
		maxz = oz;
		nextarea = narea;
	    }
	}


	if (nextarea == NULL) return false;

	v1 = hitpt;
	SETBIT(wseen, hitw->id);

	area = nextarea;
    }

    return area == enda;
}

/*
 *
 * DynamicActor
 *
 */


#define PLAYERACCEL 15.625
#define GRAVITY (-1200) // units/sec^2
struct Substance {
    double density;
    double friction;
    double zfrict;
    double statfrict;
};

Substance airsubst = { 0, 0.48, 0.8, 0.02 };
Substance watersubst = { 471.8592, 0.0839, 0.02 };

class DynamicActor : public BaseActor {
public: DECLARE_OBJECT;
    bool falls;
    bool canjump;
    bool onground;
    bool wasonground;

    double waterxyspeed;
    double waterzspeed;

    double mass;
    double volume;
    double density;

    double floatheight;
    double floatdens;

    double sparkwait;

    weakptr<ServerActor> onsolid;

    DynamicActor() : falls(true), canjump(false), waterxyspeed(7), waterzspeed(4.5), 
			     floatheight(-INFINITY), sparkwait(0), onsolid(NULL) { }
    

    void setmass(double mas) {
	mass = mas;
	
	double volume = (2 * height) * (M_PI * rad*rad);

	density = mass / volume;
	
    }
    /*    
     *
     *            Gravity and buoyancy calculation:
     *
     *                          -->
     *                        ______
     *                       /______\
     *                      //      \\         Equal volume of substance
     *                     //        \\
     *                    ||          ||
     *                    ||          || |    /
     *                  ^ | \        / | V   /
     *                  | |  \______/  |    /
     *    This object   | |            |   /
     *             \      |            |  /
     *              \     |          +++++   Gravity (constant)
     *               \    |          +++++   | | | |
     *                \   |          +++++   V V V V
     *                 \  |   ^
     *                  +++++ |
     *                  +++++  \
     *                  +++++   \
     *                           \
     *                            Force of gravity on equal volume
     *                            cancels out force of gravity on object
     *
     */

    void buoy(double& force, double level, double dens, double* frac=NULL) {
	double bot = loc.z - height;

	double heightinsubst = level - bot;
	if (heightinsubst<0) heightinsubst=0;
	if (heightinsubst>(height*2)) heightinsubst=(height*2);
	    
	double volinsubst = heightinsubst * M_PI * rad*rad;
	double smass = dens * volinsubst;
	double sforce = GRAVITY * smass;
	    
	force -= sforce;

	if (frac) 
	    (*frac) = (heightinsubst/(height*2));
    }
    virtual void runphys(double time) {

	canjump=false;
	double zbound=-INFINITY;
	double waterlevel = Water::getlevel(area);

	double fh = -INFINITY;

	sptr<ServerActor> tsolid;

	FOREACH(itr, areas) {
	    double zb = (*itr)->floor->getzbound(*itr, this) - height;
	    if (zb > zbound) {
		zbound = zb;
		tsolid = NULL;
	    }
	    FOREACH(aitr, (*itr)->actors) {
		unsigned int sn = (*aitr)->solidnum;
		if (sn) {
		    if (sn - 1 >= world->extragroups.size())
			continue;
		    FOREACH(gitr, world->extragroups.at(sn-1)) {
			vect3d tloc(loc - (*aitr)->loc);
			zb = (*gitr)->ceil->getzbound(*gitr, tloc, rad, height, true);
                        //cout << "zb = " << zb << endl;
                        zb -= height; 
                        zb += (*aitr)->loc.z;
			//cout << zb <<endl;
			if (loc.z >= zb && zb > zbound) {
			    zbound = zb;
			    tsolid = aitr->cast<ServerActor>();
			}
		    }
		}
	    }
	}

	if (floatheight) {
	    
	    fh = floatheight+zbound;
	    if (zbound < waterlevel && loc.z>waterlevel) fh=floatheight+waterlevel;
	}


	double frac = 0;
	double frac2 = 0;


	double force = GRAVITY * mass;

	buoy(force, waterlevel, watersubst.density, &frac);
	buoy(force, fh, floatdens, &frac2);
	
	if (loc.z-height<=(zbound+floatheight)) {
	    canjump = true;
	}
	double accel = force / mass;

	if (falls) {
	    vel.z += accel*time / 64.0;

	}

	double frict = (watersubst.friction-airsubst.friction)*frac+airsubst.friction;
	if (loc.z-height <= zbound+EPSILON) {
	    frict = 0.006;
	    onground = true;
	} else {
	    onground = false;
	}

	if (!frac2 && !onground) tsolid = NULL;
	if (tsolid != NULL) {
            //cout << "foo!" << endl;
	    frict *= 2.0/3.0;
	    vel -= tsolid->vel;
	}


	double fmul = pow(frict, time);
	vect2d vel2(vel);
	vel2 *= fmul;
	if (vlen(vel2) < 1.0 / 32.0) {
	    vel2 = vect2d();
	}
	vel.x = vel2.x;
	vel.y = vel2.y;

	double zfrict = (watersubst.zfrict-airsubst.zfrict)*frac+airsubst.zfrict;
	if (frac2) zfrict = 0.12;

	vel.z *= pow(zfrict, time);
	if (tsolid != NULL) {
            vect3d tvel = tsolid->vel;
	    vel += tvel;
	}
        sptr<ServerActor> oldsolid = onsolid.getref();
	if (tsolid != oldsolid)
	    onsolid.setref(tsolid);
    }
    virtual void tick(double time) {
	if (sparkwait>0) sparkwait-=time;
	BaseActor::tick(time);
    }

    bool inwater(double tz) {
	FOREACH(s, areas) {
	    if (loc.z-height < Water::getlevel(*s)) return true;
	}
	return false;
    }
    bool inwater() {
	return inwater(loc.z-height);
    }
    virtual double bounce(Bumpable* o, ServerActor* oa, vect3d force) {
	vel += force;

	return 0.0;
    }
    virtual void hitceil(double ozv, double f) { vel.z=ozv-0.2; }
    virtual void hitfloor(double ozv, double f);
    virtual double takemomentum(vect3d norm) {
	double ret = norm * vel;
	//cout << vel<<" Take "<<ret<<" ("<<(ret*norm)<<")"<<endl;
	//cout << "Take "<<vel<<endl; //<<" Take "<<ret<<" ("<<(ret*norm)<<")"<<endl;
	vel -= ret * norm;
	return ret;
    }
    virtual double givemomentum(Bumpable* o, Actor* oa, vect3d norm, double mom) {
	Plane* p = dynamic_cast<Plane*>(o);
	vect3d force = norm * mom;
	//cout <<force<<' '<<p<<endl;
	//cout << "Give" << vel << ' ' << (vel + force) << ' ' << (vel - force) << endl; // <<" Give "<<mom<<" ("<<(force)<<")"<<endl;
	if (p != NULL) {
	    if (force.z>0) canjump=true;
	    vel -= force;
	    double oz = 0.0;
	    if (oa != NULL) oz = static_cast<ServerActor*>(oa)->vel.z;
	    if (vel.z >= oz && (p->ceil)) hitceil(oz, abs(force.z));
	    if (vel.z <= oz && (!p->ceil)) hitfloor(oz, abs(force.z));
		
	    return 0.0;
	}
	//cout << vel << ' ' << force;
	double ret = bounce(o, static_cast<ServerActor*>(oa), force);
	//cout << vel << ' ' << endl;
	return ret*mom;
    }

};

#include objdef(DynamicActor)

#define randflt() ((double)rand()/(double)(RAND_MAX+1.0))

/*
 *
 * Spark 
 *
 */

class Spark : public DynamicActor {
public: DECLARE_OBJECT;
    double ttl;
    Spark(vect3d _loc, Area* _area) {
	loc = _loc;
	area = _area;
	rad = .46875;
	height = .15625 * 0.5;
	stepheight = 0;
	mass=10.0;
	floatheight=0;
	

	//dr=new SpriteDrawable(this, w->texm, "spark.png", 0, randflt()*0.25+0.1, vect2d(16, 32), 1, 0.5);

	ttl = randflt()*2.0+0.5;

	vel.x = randflt()*2.0-1.0;
	vel.y = randflt()*2.0-1.0;
	vel.z = randflt()/3.0;
	vel *= 10.0;
    }
    

    void init() {
	DynamicActor::init();
	ostringstream oss;

	comm->set_client_class("sprite");
	comm->setparameter(0, "spark.png");
	comm->setparameter(1, PT_INT, 0);
	comm->setparameter(2, PT_FLOAT, ((randflt() * (1.0/256.0) + (0.1/64.0))));
	comm->setparameter(3, PT_INT, 16);
	comm->setparameter(4, PT_INT, 32);
    }
    void hitfloor(double ozv, double f) {
	vel.z = fabs(vel.z);
    }
    bool blocks(ServerActor* o) {
	return false;
    }
    void tick(double time) {
	ttl -= time;
	if (vlen(vel)<0.15625 || ttl<=0) {
	    remove();
	    return;
	}
	runphys(time);
    }

    CLASSN(Spark);
    
};

#include objdef(Spark)

void DynamicActor::hitfloor(double ozv, double f) { 
    //cout << id<<" Hit floor at velocity of "<<vel.z<<endl;
    if (vel.z < 0) vel.z=0;
    //if (ozv > 0 && vel.z < ozv) vel.z = ozv;
    //if (vel.z > ozv) vel.z=ozv;
    if (sparkwait <= 0) {
	for (int i = rand()%2; i >=0; i--) {
	    sptr<Spark> s = GC::track(new Spark(loc - vect3d(0, 0, height), area));
	    world->add(s);
	}
	sparkwait=randflt()*0.5+0.6;
    }
    
}


/*
 *
 * SolidActor 
 *
 */

class SolidActor : public BaseActor {
public: DECLARE_OBJECT;
    SolidActor() : blocked(false) {
    }
    

    virtual void readargs(DataInput& di) {
	solidnum = di.readu16();
	rad = di.readf();
	height = di.readf();

	stepheight = 0;
    }
    virtual void init() {
	ostringstream oss;
	comm->set_client_class("areagroup");
	comm->setparameter(0, PT_ID, solidnum);
    }
    virtual double takemomentum(vect3d norm) {
	return norm * vel;
    }
    virtual double givemomentum(Bumpable* o, Actor* oa, vect3d norm, double mom) {
	if (vect2d(norm) == vect2d()) return -mom;
	return abs(mom);
    }

    virtual bool clips(ServerActor* other) {
	return !other->solidnum;
    }
    virtual void tick(double time) {
	blocked = false;
    }
    virtual void blockedby(ServerActor* a) {
	//cout << "blocked by " << ((sptr<BaseActor>)a)->getname()<<endl<<endl;
	blocked = true;
	vel = vect3d();
    }
    bool blocked;
    CLASSN(SolidActor);
};

#include objdef(SolidActor)


/*
 *
 * DebugActor
 *
 */

class DebugActor : public SolidActor {
public: DECLARE_OBJECT;
    DebugActor() {
	tdir = vect3d(0, 0, 1).normalize();
	//tdir = vect3d();
	dir = 1;
    }
 
    virtual void readargs(DataInput& di) {
	SolidActor::readargs(di);
	
	origloc = loc;
    }

    void tick(double time) {
	double tv = vlen(origloc-loc);
	double nv = 3-tv;
	if (nv < 0.01 && (origloc-loc)*tdir*dir < 0) {
	    dir = -dir;
	    //tv = -tv;
	}
	if (!blocked && !_game_debug)
	    vel = sqrt(abs(nv)) * dir * 5 * tdir;
	else
	    vel = vect3d();
	//if (isnan(vel.x)) cout << nv << "  foo!!!\007\n";
	SolidActor::tick(time);
    }
    vect3d tdir;
    vect3d origloc;
    int dir;
    CLASSN(DebugActor);
};
#include objdef(DebugActor)
REGISTER_CLASS(debug, DebugActor);


/*
 *
 * Platform
 *
 */

class Platform : public SolidActor {
public: DECLARE_OBJECT;
    Platform() {
	cpos = 0;
	delayrem = 0;
    }

    virtual void readargs(DataInput& di) {
	SolidActor::readargs(di);
	
	while (!di.isEOF()) {
	    double tx = di.readf();
	    double ty = di.readf();
	    double tz = di.readf();
	    double td = di.readf();
	    pos.push_back(pair<vect3d, double>(vect3d(tx, ty, tz), td));
	}
	origpos = loc;
	
    }
    void tick(double time) {
	if (blocked) {
	    vel = vect3d();
	    SolidActor::tick(time);
	    return;
	}
	if (pos.empty()) { vel = vect3d(); return; }
	if (delayrem>0) {
	    delayrem -= time;
	    if (delayrem < 0) delayrem = 0;
	    vel = vect3d();
	    return;
	}
	vect3d dpos = pos[cpos].first;
	vect3d tdir = loc - dpos;
	vect3d odir = origpos - dpos;

	vect3d nloc = loc + vel*time;
	vect3d ntdir = nloc - dpos;

	if (odir * ntdir <= EPSILON) {
	    origpos = dpos;
	    delayrem = pos[cpos].second-time;
	    cpos++;
	    if (((unsigned)cpos) >= pos.size()) cpos = 0;
	    vel = (dpos - loc)/(time);
	    return;
	}
	tdir.normalize();
	vel = -tdir*6;

	SolidActor::tick(time);
    }
    vect3d origpos;
    int cpos;
    vector< pair<vect3d, double> > pos;
    double delayrem;
    CLASSN(Platform);
};
#include objdef(Platform)
REGISTER_CLASS(platform, Platform);

/*
 *
 * PathNode
 *
 */
#include objdef(PathNode)

REGISTER_CLASS(node, PathNode);


PathNode::GoalPair PathNode::getnextnode(PathNode* goal) {
    if (tgoal==goal) return GoalPair(d2g, nextn);

    PROBEGIN(GetNextNode_fast);
    PROBEGIN(GetNextNode_slow);
    
    GoalCache::iterator gitr = gcache.find(goal);
    if (gitr != gcache.end()) {
	tgoal=goal;
	d2g = gitr->second.first;
	nextn = gitr->second.second;
	PROEND(GetNextNode_fast);
	return gitr->second;
    }

    FOREACH(itr, world->actors) {
	PathNode* p = itr->cast<PathNode>();
	if (p == NULL) continue;
	p->nextn = NULL;
    }
    
    tgoal = goal;
    goal->nextn=goal;
    goal->d2g=0;

    set<PathNode* > nodes;
    nodes.insert(goal);

    while(!nodes.empty()) {
	PROBEGIN(GetNextNode_loop);
	set<PathNode* >::iterator beg=nodes.begin();
	PathNode* cn = *beg;
	nodes.erase(*beg);
	FOREACH(itr, cn->onodes) {
	    PathNode* o = itr->n;
	    double d = cn->d2g+itr->d;
	    if (o->nextn == NULL || o->d2g>d) {
		o->nextn = cn;
		o->d2g = d;
		nodes.insert(o);
	    }
	}
	PROENDNT(GetNextNode_loop);
    }
    FOREACH(itr, world->actors) {
	PathNode* p = itr->cast<PathNode>();
	if (p == NULL) continue;
	if (p->nextn != NULL)
	    p->gcache.insert(GoalCache::value_type(goal, GoalPair(p->d2g, p->nextn)));
    }

    PROEND(GetNextNode_slow);
    return GoalPair (d2g, nextn);
    
}
void PathNode::fillpathto(PathNode* goal) {
    FOREACH(ca, world->actors) {
	PathNode* cn = ca->cast<PathNode>();
	if (cn == NULL) continue;
	cn->debug_onpath=false;
    }
    PathNode* c = this;
    while (c != NULL && c!=goal) {
	c->debug_onpath=true;
	c->nextn = c->getnextnode(goal).second;
	c = c->nextn;
    }
    goal->debug_onpath = true;
}



PathNode::PathNode() {
    rad = 0.25;
    height = 0.5;
    debug_onpath = false;
    is_static = true;
}
void PathNode::init() {
    if (zone.empty()) {
    }
    FOREACH(itr, world->actors) {
	PathNode* o = itr->cast<PathNode>();
	if (o == NULL || o == this) continue;
	if (can_see_from(o->loc, o->area)) {
	    double d = vlen(loc - o->loc);
	    onodes.push_back(NodeLink(d, o));
	}
    }
    sort(onodes.begin(), onodes.end());
}
bool PathNode::can_see_from(Area* area) {
    //if (zone.find(area->id)==zone.end()) return false;
    return true;
}
bool PathNode::can_see_from(vect3d pt) {
    vect3d tr = world->trace(loc, area, pt, noactors);
    if (tr == pt) {
	return true;
    }
    //cout<<"can't see from loc: "<<tr<<' '<<pt<<endl;
    return false;
}
bool PathNode::can_see_from(vect3d pt, Area* oarea, bool flooronly) {
    PROBEGINF;
    bool ret=/*tracefloor(world, pt, oarea, loc, area) &&*/ can_see_from(oarea) && (flooronly || can_see_from(pt));
    PROENDF;
    return ret;
}
void PathNode::readargs(DataInput& ts) {
}


/*
class HovercraftCtl {
public:
    virtual int forward_accel()=0;
    virtual int strafe_accel()=0;
    virtual int vert_accel()=0;
    virtual int rotvel()=0;
    virtual bool horn()=0;
    virtual bool _dbgEnableBounce() { return true; }
    virtual bool _dbgEnableGrav() { return true ; }
    virtual bool _dbgNoclip() { return false; }
    virtual int _dbgShiftMul() { return 1; }
};
*/

/*
class GameInterfaceImpl : public GameInterface {
    friend class Hovercraft;
    sptr<Hovercraft> player;

    int vf, vz, vr, vs;
    bool bounce, falls, ghost;


public: DECLARE_OBJECT;
    
    GameInterfaceImpl(World* w, ClientInterface* iface);

    bool command(string cmd);
    void control(int vf, int vs, int vz, int vr) {

	this->vf = vf;
	this->vs = vs;
	this->vz = vz;
	this->vr = vr;

    }

};
*/


/*
 *
 * HoverCraft
 *
 */

class Hovercraft;

#define PLAYERFH (0.5)

class Hovercraft : public DynamicActor {
    //friend class GameInterfaceImpl;

    bool jumplock;
    bool wasinwater;
    bool sndlock;
    int lastengine;
    double landsndwait;
    int shiftmul;

    double angvel;

    HovercraftCtl* ctl;
    //sptr<GameInterfaceImpl> gi;

public: DECLARE_OBJECT;

    Hovercraft(HovercraftCtl* ctl) : ctl(ctl){ 
	sndlock = false;
	rad = 0.525;
	height = 0.15625;
	stepheight = 0.0625;

	floatheight = PLAYERFH;
	floatdens = 2569.0112;
        //floatdens = 800;
        //floatdens = 3000;
	setmass(205);
	jumplock=false;
	waterzspeed=3;
	wasinwater=false;

	angvel = 0;
    }
    
    void init() {
	DynamicActor::init();
#if 1
	comm->set_client_class("model");
	comm->setparameter(0, "player.mdl");
	comm->setparameter(1, PT_FLOAT, 0);
	comm->setparameter(2, PT_FLOAT, -0.415);
	comm->setparameter(3, PT_FLOAT, -0.15);
	comm->setparameter(4, PT_FLOAT, -(M_PI/2));
	comm->setparameter(5, PT_FLOAT, 0.3125);
#else
	comm->set_client_class("cylinder");
	comm->setparameter(0, PT_FLOAT, rad);
	comm->setparameter(1, PT_FLOAT, height);
	comm->setparameter(2, PT_FLOAT, 0.5);
	comm->setparameter(3, PT_FLOAT, 0.5);
	comm->setparameter(4, PT_FLOAT, 0.5);
#endif
	comm->setparameter(31, PT_FLOAT, 0.5);
	comm->setparameter(30, "eng_idle");
	lastengine = 1;
    }
    bool clips(ServerActor* a) {
	if (dynamic_cast<Hovercraft*>(a) != NULL) return true;
	return false;
    }
    virtual bool moveto(vect3d pt, Clipper& cr) {
	if (ctl->_dbgNoclip()) {
	    loc = pt;
	    return true;
	} else
	    return ServerActor::moveto(pt, cr);
    }
    virtual void tick(double time) {
	if (landsndwait>0) {
	    landsndwait -= time;
	    if (landsndwait<0) landsndwait = 0;
	}
	falls = ctl->_dbgEnableGrav();

	double dangvel = TURNVEL * ctl->rotvel();

	double tangvel, tdangvel;
	if (angvel > 0) {
	    tangvel = angvel; tdangvel = dangvel;
	} else {
	    tangvel = -angvel; tdangvel = -dangvel;
	}

	if (tangvel < tdangvel) {
	    tangvel += 10 * time;
	    if (tangvel > tdangvel) tangvel = tdangvel;
	} else {
	    tangvel -= 15 * time;
	    if (tangvel < tdangvel) tangvel = tdangvel;
	}

	if (angvel > 0) {
	    angvel = tangvel;
	} else {
	    angvel = -tangvel;
	}

	ang += angvel * time;

	if (ctl->horn()) {
	    comm->playsound("horn", loc, 1.0);
	}
	double s = sin (ang);
	double c = cos (ang);

	bool nowinwater=inwater();
	if (falls) {
	    
	    floatheight=(ctl->vert_accel()==-1)?0:PLAYERFH;

	    if (nowinwater) {
		vel.z += PLAYERACCEL*ctl->vert_accel()*time;

		if (!wasinwater && !ctl->_dbgNoclip()) {
		    comm->playsound("splash", loc, 1.0);
		}
		wasinwater=true;
	    } else {
		if (ctl->vert_accel()>0) {
		    if (canjump && !jumplock) {
			vel.z=750  * MASTER_SCALE;
			comm->playsound("jump", loc, 1.0);
			jumplock=true;
		    }
		} else jumplock=false;
		if (wasinwater && !ctl->_dbgNoclip()) {
		    
		    comm->playsound("unsplash", loc, 1.0);
		    if (ctl->vert_accel()) jumplock=true;
		}
		wasinwater=false;
	    }
	} else {
	    vel.z=(ctl->vert_accel() * FWVEL * 1) * ctl->_dbgShiftMul() * MASTER_SCALE;
	}

	
	if (ctl->_dbgEnableBounce()) {
	    vel.x += (c*ctl->forward_accel() - s*ctl->strafe_accel()) * PLAYERACCEL* time;
	    vel.y += (s*ctl->forward_accel() + c*ctl->strafe_accel()) * PLAYERACCEL*time;
	} else {
	    vel.x = ((c * FWVEL * ctl->forward_accel()) - (s * FWVEL * ctl->strafe_accel())) * ctl->_dbgShiftMul() * MASTER_SCALE;
	    vel.y = ((s * FWVEL * ctl->forward_accel()) + (c * FWVEL * ctl->strafe_accel())) * ctl->_dbgShiftMul() * MASTER_SCALE;
	}

	int engine = 1;
	if (ctl->vert_accel() > -1) {
	    if (ctl->forward_accel() || ctl->strafe_accel()) {
		engine = 2;
	    } else {
		engine = 1;
	    }
	    
	}
	if (ctl->_dbgNoclip())
	    engine = 0;
	if (engine != lastengine) {
	    lastengine = engine;
	    if (engine == 0) {
		comm->setparameter(30, "");
	    } else if (engine == 1) {
		comm->setparameter(30, "eng_idle");
		comm->setparameter(31, PT_FLOAT_NOLERP, 0.3);
	    } else {
		comm->setparameter(30, "eng_norm");
		comm->setparameter(31, PT_FLOAT_NOLERP, 0.5);
	    }
	}

	DynamicActor::runphys(time);
	if (onground && !wasonground) {
	    if (!landsndwait && !ctl->_dbgNoclip()) {
		comm->playsound("land", loc, 0.8);
		landsndwait=0.5;
	    }
	}
	wasonground = onground;
	DynamicActor::tick(time);
    }
    virtual double bounce(Bumpable* o, ServerActor* oa, vect3d force) {
	if (!ctl->_dbgEnableBounce()) return 1.0;
	if (isnan(force.x) || isnan(force.y) || isnan(force.z))
	    cout << "bounce "<<force<<"\007\n";

	if (dynamic_cast<Wall*>(o) != NULL) {
	    double vol = vlen(force)*0.25;
	    if (vol > 1.0) vol = 1.0;
	    if (vol > 0.15)
		comm->playsound("hit_wall", loc, vol);
	}

	Hovercraft* oh = dynamic_cast<Hovercraft*>(o);
	if (oh != NULL && oa != this && vect2d(force) != vect2d()) {
	    if (sndlock) {
		sndlock = false;
	    } else {
		oh->sndlock = true;
		comm->playsound("craft_bump", (loc+oh->loc)/2, 1.0);
	    }
	}
	vel += force;
	return 0.0;
    }
    virtual void readargs(DataInput& di) {
	if (!di.isEOF()) {
	    rad=di.readf();
	    height=di.readf();
	    stepheight=di.readf();
	}
    }
    virtual void hitceil(double ozv, double f) { 
	jumplock=true;
	vel.z=ozv-0.5;
	double vol = f*0.1;
	if (vol > 0.1) {
	    if (vol > 1.0) vol = 1.0;
	    comm->playsound("land", loc, vol);
	}
    }
    virtual void hitfloor(double ozv, double f) { 
	DynamicActor::hitfloor(ozv, f);
    }
    CLASSN(Hovercraft);
};

#include objdef(Hovercraft)

/*
 *
 * HovercraftStart
 *
 */
class HovercraftStart : public BaseActor {
public: DECLARE_OBJECT;
    HovercraftStart() { };
    bool clips(ServerActor* o) {
	return false;
    }
    
    CLASSN(HovercraftStart);

};
REGISTER_CLASS(player, HovercraftStart);
#include objdef(HovercraftStart)

#if 0

/*
 *
 * GameInterfaceImpl
 *
 */
#include objdef(GameInterfaceImpl)

GameInterfaceImpl::GameInterfaceImpl(World* w, ClientInterface* iface) 
    : GameInterface(w, iface) {
    
    player = GC::track(new Hovercraft());
}

/*
 *
 * GameInterface
 *
 */
#include objdef(GameInterface)
sptr<GameInterface> GameInterface::create(ClientInterface* iface) {
}
#endif

sptr<Actor> GameActorLoader::load(string& clas, vect3d pt, Area* area, double ang, DataInput& di) {
    sptr<BaseActor> ret;
    map<string, CreateFunc>::iterator itr = classes.find(clas);

    if (itr==classes.end()) {
	return NULL;
    } else
	ret = (itr->second)();
    
    if (ret != NULL) {
	ret->loc=pt;
	ret->ang = ang;
	ret->area = area;
    }
    
    ret->readargs(di);
    
    HovercraftStart* ps = ret.cast<HovercraftStart>();
    if (ps != NULL) {
	startpts.push_back(ps);
	return NULL;
    }
    
    return ret;
}

sptr<Actor> GameActorLoader::create_hovercraft(HovercraftCtl* ctl) {
    sptr<Hovercraft> ret = GC::track(new Hovercraft(ctl));
    return ret;
}
