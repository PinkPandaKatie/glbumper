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

// engine.cpp: The engine itself.
//

#include "common.h"
#include "engine.h"
#include "profile.h"
#include "tokenizer.h"

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <queue>


#define HIT_XY 1
#define HIT_FLOOR 2
#define HIT_CEIL 4
#define HIT_Z (HIT_FLOOR|HIT_CEIL)

int _attrpushed = 0;

// Definitions for globals in engine.h
int actorid=0;
int debug_draw = 0;
int debug_tweak = 0;
int draweverything = 0;
int drawmode = 0;
int nowalls = 0;
int debug_colors = 0;
int debug_revalidate = 0;
ActorFilter allactors;
NothingFilter noactors;
sptr<Bumpable> __dummy;
tracepath __dummy_tracepath;

string Texture::loaderclass = "texture";

#include objdef(Texture)
#include objdef(Appearance)
#include objdef(Communicator)


#define NODE_BLOCK_LEN (4096 / sizeof(rangenode))

GCList rangenode::pool;
rangenode* rangenode::alloc() {
    rangenode* nodes = new rangenode[NODE_BLOCK_LEN];

    rangenode* crawler = nodes;

    for (unsigned int i=0; i < NODE_BLOCK_LEN; i++, crawler++) {
	crawler->next=(crawler+1);
	crawler->prev=(crawler-1);
    }
    nodes->prev = nodes;
    nodes->next = nodes;
    nodes++; // we'll return the first one
    crawler--; // this is one past the end
    pool.next = nodes; // insert the rest into the pool
    nodes->prev = &pool;
    pool.prev = crawler;
    crawler->next = &pool;
    
    return (nodes-1);
}

Link::Link(Area* a, int flags) : a(a), flags(flags) { }

AreaTess::AreaTess(Area* area) :  name(0), area(area) { }

Wall::Wall() { }
ServerActor::ServerActor() 
    : angv(0),
      thinkinterval(0), nextthink(0),stepheight(0),comm(NULL) { }
Appearance::Appearance(Actor* act) : act(act) { }
Appearance::~Appearance() { }
Actor::Actor() 
    : area(NULL),  ang(0), rad(0),  height(0), solidnum(0),
      appearance(NULL), is_static(false), id(-1)
       { };

// A small value, 2 ** -20
string Wall::str() const throw() {
    ostringstream oss;
    if (next == NULL)
	oss << "Wall: " << v1 << "-<incomplete>";
    else
	oss << "Wall: " << v1 << "-" << next->v1;
    return oss.str();
}
string ServerActor::str() const throw() {
    ostringstream oss;
    oss << "Actor: " << loc << " " << vel;
    return oss.str();
}
string Plane::str() const throw() {
    ostringstream oss;
    oss << "Plane: " << cpt << " " << dir << " "<<slope;
    return oss.str();
}


// Implementation of class Wall

#include objdef(Wall)

void Wall::revalidate ()
{
    j = next->v1-v1;
    len = vlen(j);
    j /= len;

    // Cache the inverse slope of the wall. Used in Area::pointin.
    tslope = ((next->v1.x - v1.x) / (next->v1.y - v1.y));
}
bool Wall::clippoint (vect2d npt, double rad, vect2d* clip, vect2d* hitpt, double *oprio,
	      double surfaceepsilon)
{
    double distm=-1;
    double prio=0;
    
    // Transform the point into the coordinate space of the wall. (so
    // that the wall becomes <0, 0> to <len, 0>
    vect2d tp(j * (npt-v1), ~j * (npt-v1));
    vect2d ret;

    if (tp.x >= 0 && tp.x <= len && tp.y <= rad && tp.y >= -rad) {
	ret.x=tp.x;
	if (tp.y > 0) {
	    ret.y = rad;
	    distm = rad-tp.y;
	} else {
	    ret.y = -rad;
	    distm = rad+tp.y;
	}
	prio=rad*1000;
    } else {
	double ln2 = vlen2(tp);
	double r2=rad*rad;
	if (ln2 < r2) {
	    double ln = sqrt (ln2);
	    distm = rad-ln;
	    ret = tp * (rad / ln);
	} else {
	    tp.x-=len;
	    ln2=vlen2(tp);
	    if (ln2<r2) {
		double ln = sqrt (ln2);
		distm = rad-ln;
		ret = tp * (rad / ln);
		ret.x += len;
	    }
	}
    }
    
    // If the circle intersected, and clipx is not null, transform the
    // point back to normal coordinate space.
    if (distm>EPSILON) {
	if (clip) *clip = v1 + j*ret.x + ~j*ret.y;
	if (ret.x < 0) ret.x=0;
	else if (ret.x > len) ret.x=len;
	if (hitpt) *hitpt = v1 + j*ret.x;
	if (oprio) *oprio=distm+prio;
	return true;
    }
    return false;
}

bool Wall::intersect(vect2d ov1, vect2d ov2, vect2d& hit)
{

    // Transform both points into the wall's coordinate space.
    vect2d tv1 (j * (ov1-v1), ~j * (ov1-v1));
    vect2d tv2 (j * (ov2-v1), ~j * (ov2-v1));

    double xintercept;

    // If both points are on the same side of the wall, the line does
    // not intersect.
    if ((tv1.y > 0) == (tv2.y > 0)) {
	return false;
    }

    xintercept = tv1.x - ((tv2.x - tv1.x) / (tv2.y - tv1.y)) * tv1.y;
    // If the x intercept is within the bounds of the wall, transform
    // the intersection point back to normal coordinate space and
    // return true.
    if (xintercept >= 0 && xintercept <= len) {
	hit = j * xintercept + v1;
	return true;
    }
    return false;
}

// Returns true if two numbers are equal or very close.
inline bool __attribute__((const)) close (double x, double y)
{
    return fabs (x - y) < EPSILON;
}

// Implementation of class Plane.

// If we have a slope, then our transformation matrix is:
//
// [  1.0   0.0   0.0         0.0        ]
// [  0.0   1.0   0.0         0.0        ]
// [ ix*s  iy*s   1.0 -(x*ix*s + y*iy*s) ]
// [  0.0   0.0   0.0         1.0        ]
//
// where <x, y> is the control point, <ix, iy> is the direction,
// and s is the slope.
//
// Note that if slope is zero, the matrix is the identity
// matrix and is not needed.

#include objdef(Plane)

void Plane::revalidate()
{
    sfact = sqrt (1.0 + sqr(slope));
    if (slope!=0) {
	if (!matrix) {
	    matrix = new double[16];
	}

	matrix[0]  = 1.0;
	matrix[1]  = 0.0;
	matrix[2]  = dir.x*slope;
	matrix[3]  = 0.0;

	matrix[4]  = 0.0;
	matrix[5]  = 1.0;
	matrix[6]  = dir.y*slope;
	matrix[7]  = 0.0;

	matrix[8]  = 0.0;
	matrix[9]  = 0.0;
	matrix[10] = 1.0;
	matrix[11] = 0.0;

	matrix[12] = 0.0;
	matrix[13] = 0.0;
	matrix[14] = - (cpt.x*matrix[2] + cpt.y*matrix[6]);
	matrix[15] = 1.0;
    } else {
	delete[] matrix;
	matrix = NULL;
    }
}

extern void __bkpt();

double Plane::getzbound(Area* limit, vect2d pt, double rad, double height, bool clipsolid)
{
    PROBEGIN(checkz_fast);
    PROBEGIN(checkz_slow);

    if (slope==0 && !clipsolid) {
	// Special case: slope is zero, so bound is z [+-] height.
	return cpt.z+(ceil?-height:height);
    }
    if (rad == 0) {
	return getz(pt)+(ceil?-height:height);
	
    }

    // Calculate the highest (or lowest) point where the cylinder
    // intersects with the plane. This is always <px, py> [+-] <ix, iy>*rad

    vect2d tpt = pt;

    if ((slope>0) == ceil) {
	tpt -= dir*rad;
    } else {
	tpt += dir*rad;
    }

    // If the limit area is not specified, or <tpx, tpy> is in the
    // area, simply return the value of the plane at that point [+-] height.

    if ((limit == NULL) || limit->pointin(tpt)) {
	PROEND(checkz_fast);
        double ret = getz(tpt);

        if (ceil) {
            ret -= height;
        } else {
            ret += height;
        }

        return ret;
    }

    // Ok, so the point is not in the area. So, we have to find some
    // other point to be the (max|min)imum. This point will either be
    // 1. a vertex of the area, or
    // 2. a point along one of the walls.
    //
    // We start out with the value at [+-]infinity, and then check all
    // possible points. We loop through the walls, checking the vertex
    // if the vertex is inside the circle, then check the points where
    // the circle intersects the wall.

    double cz = (ceil?INFINITY:-INFINITY);

#define ckz(pt) {                                       \
	double tz=getz(pt) + (ceil?-height:height);     \
	if (ceil == (tz<cz)) cz = tz;                   \
    }
    double rad2 = sqr(rad);
    FOREACHW(w, limit) {
	double a, ta;

	// Check the vertex.
	if (vlen2(w->v1-pt) < rad2) {
	    ckz(w->v1);
	}

	// Transform the point to wall coordinate space.
	double doti = ~w->j*(pt-w->v1);
	double dotj = w->j*(pt-w->v1);

	// Circle does not intersect.
	if (fabs(doti)>rad+EPSILON) continue;

	// Check intersection points.
	a = sqrt(rad2-sqr(doti));
	if (isnan(a)) a = 0;
	ta = dotj+a;
	if (ta >= 0 && ta <= w->len) {
	    ckz(w->v1+w->j*ta);
	}

	ta = dotj-a;
	if (ta >= 0 && ta <= w->len) {
	    ckz(w->v1+w->j*ta);
	}
    }
    PROEND(checkz_slow);
    return cz;

#undef ckz    
}

double Plane::getzbound(Area* limit, Actor* a, bool clipsolid) 
{
    return getzbound(limit, a->loc, a->rad, a->height, clipsolid);
}

#include objdef(Area)

Area::Area(void) { 
    tess = NULL;
    invert = false;
}

Area::~Area(void) { 
    delete tess;
}


bool Area::pointin(double x, double y) {
    return pointin(vect2d(x, y));
}
bool Area::pointin(vect2d pt)
{
    PROBEGINF;
    int cnt = 0;
    FOREACHW(w, this) {
	if ((w->v1.y < pt.y) == (w->next->v1.y < pt.y))
	    continue;
	double xi = (w->v1.x - w->tslope * (w->v1.y - pt.y));
	if (w->v1.y > w->next->v1.y) {
	    if (xi < pt.x)
		cnt++;
	} else {
	    if (xi <= pt.x)
		cnt++;
	}
    }
    PROENDF;
    return cnt & 1;
}


#include objdef(World)

sptr<Area> World::findarea(vect3d v1, Area* start, vect3d v2) {
    unsigned char wseen[wall_bit_size];
    memset (wseen, 0, wall_bit_size);

    sptr<Area> ca = start;
    vect2d norm(vnormal(vect2d(v2)-vect2d(v1)));
    while (1) {
	sptr<Wall> hitw = NULL;
	vect3d hitpt;

	double mindist = INFINITY;
	FOREACHW(w, ca) {
	    //if (ISSET(wseen, w->id)) continue;
	    /*if (lwall != NULL && (lwall->v1 == w->next->v1 && lwall->next->v1 == w->v1)) {
		SETBIT(wseen, w->id);
		continue;
                }*/
	    if (norm*~w->j <= 0) continue;
	    vect2d hit;
	    if (w->intersect (v1, v2, hit)) {
		double tdist = vlen2(hit-vect2d(v1));
		if (tdist<mindist) {
		    mindist=tdist;
		    hitw=&*w;
		    hitpt=hit;
		    
		}
	    }
	}
	if (hitw == NULL) return ca;
	if (fabs(v2.x-v1.x)<fabs(v2.y-v1.y)) {
	    hitpt.z=(v2.z-v1.z)*(hitpt.y-v1.y)/(v2.y-v1.y)+v1.z;
	} else {
	    hitpt.z=(v2.z-v1.z)*(hitpt.x-v1.x)/(v2.x-v1.x)+v1.z;
	}
	sptr<Area> next=NULL;
	FOREACH(lnk, hitw->links) {
	    
	    double fz = lnk->a->floor->getz(hitpt);
	    double cz = lnk->a->ceil->getz(hitpt);
	    if (hitpt.z >= fz-EPSILON && hitpt.z <= cz+EPSILON) {
		next=lnk->a;
		break;
	    }
	    
	}

	if (next == NULL) {
            return start;
        }

	v1 = hitpt;
	SETBIT(wseen, hitw->id);

	ca = next;
    }

}

enum {
    ACTOR_PRIO=0, 
    WALL_PRIO=1, 
    PLANE_PRIO=2, 
};
    
#define clipd 0

struct Clipper {
public:
    ServerActor* a;
    vect3d onpt, npt;
    vect3d ofs;
    vect3d clip_;
    int prio;
    double distm;
    Bumpable* hit;
    ServerActor* hitactor;
    ActorFilter* cfilt;
    unsigned char* aseen;
    map<pair< Bumpable*, ServerActor* >, vect3d> bumps;
    bool zonly;

    Clipper(ServerActor* a, ActorFilter* cfilt) : a(&*a), prio(-1), distm(0), hit(NULL), 
						  cfilt(cfilt) {
	aseen = new unsigned char[a->world->area_bit_size];
    }
    void reset(vect2d _npt) {
    }
    virtual ~Clipper() {
	delete[] aseen;
    }
    inline void checkpt(vect3d pt, int tprio, double tdistm, Bumpable* h, ServerActor* ha = NULL) {
	if (tprio>prio || (tprio==prio && tdistm >= distm)) {
	    clip_ = pt + ofs;
	    prio=tprio;
	    distm=tdistm;
	    hit=h;
	    hitactor = ha;
	}
    }

    inline void checkpt(vect3d pt, int tprio, Bumpable* h, ServerActor* ha = NULL) {
	checkpt(pt, tprio, 0, h, ha);
    }

    void checkplane(Area* area, Plane* pl, ServerActor* solidactor) {
	vect3d tnpt(npt - ofs);
	double zb = pl->getzbound(area, tnpt, a->rad, a->height, true);
	if (pl->ceil) {
	    if (tnpt.z>zb && tnpt.z<zb+a->height*2)
		checkpt(vect3d(tnpt.x, tnpt.y, zb - EPSILON), PLANE_PRIO, &*pl, solidactor);
	} else {
	    if (tnpt.z<zb && tnpt.z>zb-a->height*2) {
		checkpt(vect3d(tnpt.x, tnpt.y, zb + EPSILON), PLANE_PRIO, &*pl, solidactor);
	    }
	}
    }
    void clipsolid(Area* area, ServerActor* solidactor) {

	vect3d taloc(a->loc - ofs);
	vect3d tnpt(npt - ofs);

	bool clipplanes = false;
	vect2d tclip, hitpt;
	double td;

	FOREACHW(w, area) {
	    if ((vect2d(taloc) - w->v1) * ~w->j > 0) continue;
	    if (w->clippoint(tnpt, a->rad, &tclip, &hitpt, &td, EPSILON)) {
		bool cw = false;
		cw = false;
		double fz = area->floor->getz(hitpt);
		double cz = area->ceil->getz(hitpt);
		if (taloc.z + a->height > fz + EPSILON && taloc.z - a->height < cz - EPSILON) cw = true;
		if (cw && !zonly) {
		    checkpt(tclip+vect3d(0, 0, tnpt.z), WALL_PRIO, td, &*w, solidactor);
		} else {
		    clipplanes = true;
		}
	    }
	}
	
	if (clipplanes || area->pointin(tnpt)) {
	    checkplane(area, &*area->floor, solidactor);
	    checkplane(area, &*area->ceil, solidactor);
	}

    }
    void clip(Area* area) {
	PROBEGINF;
	SETBIT(aseen, area->id);
	FOREACH(itr, area->actors) {
	    ServerActor* oa = &*itr->cast<ServerActor>();
	    if (oa == a || oa->height == 0) continue;
	    vect2d diff(npt-vect2d(oa->loc));
	    double d2 = vlen2(diff);
	    double combrad = a->rad+oa->rad;
	    if ((!cfilt || cfilt->filter(oa)) && d2 < sqr(combrad) && a->clippedby(oa)) {
		if (oa->solidnum) {
		    if (npt.z-a->height <= oa->loc.z+oa->height && npt.z+a->height >= oa->loc.z-oa->height) {
			if (((unsigned)oa->solidnum-1) >= oa->world->extragroups.size())
			    continue;
			vector< sptr<Area> >& ag(a->world->extragroups.at(oa->solidnum-1));
			
			ofs = oa->loc;
			
			FOREACH(ar, ag){
			    clipsolid(&**ar, oa);
			}
			ofs = vect3d();
		    }
		} else {
		    if (a->loc.z - a->height >= oa->loc.z + oa->height && 
			npt.z - a->height <= oa->loc.z + oa->height) {
			
			checkpt(vect3d(npt.x, npt.y, 
				       oa->loc.z + oa->height + a->height + EPSILON), ACTOR_PRIO, oa, oa);

		    } else if (a->loc.z + a->height <= oa->loc.z - oa->height && 
			       npt.z + a->height >= oa->loc.z - oa->height) {
			checkpt(vect3d(npt.x, npt.y, 
				       oa->loc.z - oa->height - a->height + EPSILON), ACTOR_PRIO, oa, oa);
		    } else {
			if (vlen2(vect2d(a->loc) - vect2d(oa->loc)) >= sqr(combrad - EPSILON)) 
			    if (npt.z - a->height <= oa->loc.z + oa->height && 
				npt.z + a->height >= oa->loc.z - oa->height) {
				diff /= sqrt(d2);
				checkpt(vect2d(oa->loc) + diff*(combrad+EPSILON) + vect3d(0, 0, npt.z), 
					ACTOR_PRIO, oa, oa);
			    }
		    }
		}
	    }
	}
	
	checkplane(area, &*area->floor, NULL);
	checkplane(area, &*area->ceil, NULL);

	vect2d tclip, hitpt;
	double td;

	FOREACHW(w, area) {
	    if ((vect2d(a->loc) - w->v1) * ~w->j > 0) continue;
	    if (w->clippoint(npt, a->rad, &tclip, &hitpt, &td, EPSILON)) {
		bool cw = true; // clip to wall?
		FOREACH(lnk, w->links) {
		    if (lnk->flags&2) continue;
                    
		    double stepz=npt.z+a->stepheight;
			
		    bool clipthis = true;
		    
		    Area* other = &*lnk->a;

		    double minfz = other->floor->getz(hitpt) + a->height - EPSILON;
		    double maxcz = other->ceil->getz(hitpt) - a->height + EPSILON;

		    //if (minfz < stepz && stepz < maxcz)
		    //stepval--; 
		    if (minfz < npt.z && npt.z <= maxcz) {
			clipthis = false;
		    } else {
			if (minfz < stepz && stepz < maxcz) {
			    double zval = other->floor->getzbound(other, npt, a->rad, a->height);
			    checkpt(vect3d(npt.x, npt.y, zval+EPSILON), PLANE_PRIO, &*other->floor);
			    clipthis = false;
			} else clipthis = true;
		    }

		    if ((zonly || !clipthis) && !ISSET(aseen, other->id))
			clip(other);
		    
		    cw &= clipthis;
		}
		if (cw && !zonly) {
		    checkpt(tclip+vect3d(0, 0, npt.z), WALL_PRIO, td, &*w);
		}
	    }
	}
	
	PROENDFNT;
    }
    bool run (vect3d onpt)
    {
	PROBEGINF;
	vect3d opt(a->loc);
	npt = onpt;
	vect3d vel = onpt - opt;
	zonly = (vel.x == 0 && vel.y == 0); //zonly;
	int sanecnt = 10;
	while (--sanecnt) {
	    hit=NULL;
	    prio=-1;
	    distm=-1;
	    memset(aseen, 0, a->world->area_bit_size);
	    clip(&*a->area);
	    if (hit == NULL) break;
	    bumps[pair< Bumpable*, ServerActor* >(hit, hitactor)] = vnormal(npt - clip_);
	    npt = clip_;
	}
	
	double dot = (vect2d(npt)-vect2d(a->loc))*vect2d(vel);
	
	if ((zonly || dot>=-1e-4) && sanecnt>0) {
	    a->loc = npt;
	} else {
	    return false;
	}
	
	a->area = a->world->findarea(opt, a->area, a->loc);
	PROENDF;
	return true;
	
    }
    void runbumps() {
	FOREACH(itr, bumps) {
	    vect3d norm=itr->second;
	    Bumpable* o = itr->first.first;
	    ServerActor* oa = itr->first.second;
	    if (isnan(norm.x)||isnan(norm.y)||isnan(norm.z)) continue;
	    
	    double mom1 = (oa != NULL ? oa->takemomentum(-norm) : 0.0);
	    double mom2 = (a->takemomentum(norm));
	    int safecnt = 4;
	    while (safecnt-- && (mom1 || mom2)) {
		double ret1 = 0, ret2 = 0;
		if (mom1) ret1 = a->givemomentum(o, oa, norm, -mom1);
		if (mom2) {
		    if (oa != NULL) ret2 = oa->givemomentum(&*a, a, -norm, -mom2);
		    else ret2 = o->givemomentum(&*a, a, -norm, -mom2);
		    }
		mom1 = ret2;
		mom2 = ret1;
	    }
	    
	}
    }

    virtual bool move(vect3d pt)=0;
};

class NormalClipper : public Clipper {
public:
    NormalClipper(ServerActor* sa, ActorFilter* cfilt) : Clipper(sa, cfilt) { }
    bool move(vect3d pt) {
	vect3d tvel = pt - a->loc;
	if (tvel == vect3d()) {
	    return true;
	}
	if (a->height == 0) {
	    a->area = a->world->findarea(a->loc, a->area, pt);
	    a->loc = pt;
	    return true;
	}

	map<pair< Bumpable*, ServerActor* >, vect3d> bumps;

	vect3d dv(a->loc+tvel);

	double v=vlen(vect2d(tvel));
	double zv = fabs(tvel.z);

	int prec=(int)(v/a->rad*2.01)+1;
	int zprec=(int)(zv/a->height*2.0)+1;

	if (zprec>prec) prec=zprec;

	vect3d odv(dv);
	vect3d oloc(a->loc);
    
	bool stuck = false;
	//double iprec = 1.0 / prec;
    
	vect3d tdvel(tvel/prec);

	vect3d lpt = a->loc;
	for (int i = 0; i < prec; i++) {
	    vect3d npt(a->loc+tdvel);
	    vect3d npta(npt);
	    npta.z = a->loc.z;

	    bool runa=true, runb=true;
	    if (npta != a->loc)
		runa=run(npta);

	    vect3d nptb(a->loc);
	    nptb.z = npta.z+tdvel.z;
	    runb = run(nptb);
	    if (!runa || !runb) {
		stuck = true;
	    }
	    if (a->loc == lpt) {
		//cout << "stopped after " << i << "/" << prec << " iterations"<<endl;
		break;
	    }
	    //cout << i << ' ' << (a->loc.z - oloc.z) << endl;
	    lpt=a->loc;
	}

	return !stuck;
    }
};

class SolidClipper : public Clipper {
public:
    bool bump;
    SolidClipper(ServerActor* sa, bool bump) :Clipper(sa, NULL), bump(bump) { }
    bool move(vect3d pt) {
	vect3d tvel = pt - a->loc;
	if (tvel == vect3d()) {
	    return true;
	}

	vect3d oloc(a->loc);

	FOREACH(itr, a->areas) {
	    Area* ca = &**itr;

	    FOREACH(aitr, ca->actors) {
		//_dbg = true;
		ServerActor* act = &*aitr->cast<ServerActor>();
		if (act->solidnum) continue;
		if (!act->clippedby(a)) continue;

		vect3d aoloc(act->loc);
		//loc = oloc;
		bool res = true;
		ExcludeOneFilter af(a);
		
		NormalClipper ncr (act, NULL);

		if (act->blocks(a)) {
		    double v=vlen(vect2d(tvel));
		    double zv = fabs(tvel.z);
		    
		    int prec=(int)(v/act->rad*2.01)+1;
		    int zprec=(int)(zv/act->height*2.0)+1;
		    
		    if (zprec>prec) prec=zprec;
		    
		    vect3d tdvel(tvel/prec);
		    a->loc = pt;
		    res = act->moveto(aoloc + tvel, ncr);
		    for (int i = 0; res && i < prec; i++) {
			vect3d npt(oloc+(tdvel*i));
			a->loc = npt;
			res = act->moveto(aoloc + tvel, ncr);
		    }
		    if (!res) {
			FOREACH(itr, ncr.bumps) {
			    if (itr->first.second == a) {

				a->blockedby(act);
				a->loc = oloc - EPSILON * (pt - oloc);
				act->loc = aoloc;
				return false;
			    }
			}
		    }
		}
		ncr.bumps.clear();
		a->loc = pt;
		act->loc = aoloc;
		ncr.cfilt = &af;
		act->moveto(aoloc + tvel, ncr);
		ncr.cfilt = NULL;
		ncr.bumps.clear();
		res = act->moveto(aoloc, ncr);
		if (bump) ncr.runbumps();
		//_dbg = false;
	    }
	}
	a->area = a->world->findarea(oloc, a->area, pt);
	a->checkareas();
	a->loc = pt;
	return true;
    }
};

#include objdef(Bumpable)

double Bumpable::givemomentum(Bumpable* hit, Actor* a, vect3d norm, double momentum) { 
    return fabs(momentum);
};


static void _populate_recurs(Actor* a, Area* area, unsigned char* aseen) 
{
    
    SETBIT(aseen, area->id);
	
    a->areas.push_back(area);
    area->actors.insert(a);

    if (a->rad == 0) return;

    vect2d clip, hitpt;
    FOREACHW(w, area) {
	if (w->clippoint(a->loc, a->rad, &clip, &hitpt)) {
	    FOREACH(lnk, w->links) {
		Area* ar = lnk->a;
		if (!ISSET(aseen, ar->id)) {
		    double fz = ar->floor->getz(hitpt);
		    double cz = ar->ceil->getz(hitpt);
		    if (a->loc.z >= fz && a->loc.z <= cz) {
			_populate_recurs(a, ar, aseen);
		    }
		}
	    }
	}
    }
	
}

#include objdef(Actor)
#include objdef(ServerActor)

ServerActor::~ServerActor() {
}
void ServerActor::init() { 
    if (comm != NULL) comm->init(); 
}

void ServerActor::onremove() { 
    comm = NULL;
}
Actor::~Actor() {
}

void Actor::remove() {
    if (world == NULL) return;
    //cout << "remove actor "<<id<<endl;
    onremove();
    world = NULL;
    checkareas();

    appearance = NULL;
}


void Actor::checkareas() {
    if (is_static) 
	return;

    FOREACH(itr, areas) {
	Area* ar = *itr;
	set< sptr<Actor> >::iterator mitr = ar->actors.find(this);
	if (mitr != ar->actors.end())
	    ar->actors.erase(mitr);
    }
    areas.clear();

    if (world != NULL && area != NULL) {
	unsigned char aseen[world->area_bit_size];
	memset (aseen, 0, world->area_bit_size);
	_populate_recurs(this, area, aseen);
    }
    
}

void ServerActor::move(double time, bool bump) 
{
    ang = normalize_angle(ang+angv*time);
    if (vel == vect3d()) return;
    if (solidnum) {
	SolidClipper cr(this, bump);
	moveto(loc + vel * time, cr);
    } else {
	NormalClipper cr(this, NULL);
	bool res = moveto(loc + vel * time, cr);
	if (bump) {
	    if (!res) {
		vect3d norm = vnormal(vel);
		double mm = takemomentum(norm);
		givemomentum(this, this, norm, -mm);
	    } else
		cr.runbumps();
	}
    }
}
bool ServerActor::moveto(vect3d pt, Clipper& cr) 
{
    if (world == NULL) return true;
    
    return cr.move(pt);
}


static inline bool abetween(double s, double e, double q) {
    if (e<s)
        return q >= s || q <= e;
    return q >= s && q <= e;
}


static void view_trace_nonrecurs (vect2d pt, Area* sa, double sla, double sra, unsigned char *areaseen) {

    queue< Area* > q;

    double centang = (sla + sra) / 2;

    sa->need += range(sla, sra);
    sa->queued = true;
    q.push(sa);

    int callcnt = 0;
    int arccnt = 0;

    while (!q.empty()) {
	Area* area = q.front();
	q.pop();
	area->queued = false;
	SETBIT (areaseen, area->id);

	area->need -= area->traced;
	area->traced += area->need;
	callcnt++;

	FOREACH(rng, area->need) { 
	    double la = rng->s;
	    double ra = rng->e;

	    vect2d lv(cos(la), sin(la));
	    vect2d rv(cos(ra), sin(ra));

	    arccnt++;

	    FOREACHW(w, area) {
		/*if (!w->links.empty())
		  i++;*/
		bool keep_left = true;
		bool keep_right = true;

		if (w->links.empty()) continue;

		vect2d ttv (pt - w->v1);
	
		vect2d tp(w->j * ttv, w->j.x*ttv.y + -w->j.y * ttv.x);

		if (tp.y > 0.001) continue;

		vect2d tlv (w->j * lv, ~w->j * lv);
		vect2d trv (w->j * rv, ~w->j * rv);

		if (tlv.y <= 0 && trv.y <= 0)
		    continue;


		if (tlv.y > 0) {
		    double xint = tp.x - (tlv.x / tlv.y) * tp.y;
		    if (xint < 0)
			continue;
		    if (xint >= w->len)
			keep_left = false;

		} else
		    keep_left = false;

		if (trv.y > 0) {
		    double xint = tp.x - (trv.x / trv.y) * tp.y;
		    if (xint > w->len)
			continue;
		    if (xint <= 0)
			keep_right = false;

		} else
		    keep_right = false;

		double nla = la;
		double nra = ra;

		if (!keep_left)
		    nla = normalize_angle(atan2(w->next->v1 - pt), centang);

		if (!keep_right)
		    nra = normalize_angle(atan2(w->v1 - pt), centang);

		range trng(nla, nra);

		FOREACH(lnk, w->links) {
		    lnk->a->need += trng;
		    if (!lnk->a->queued) {
			lnk->a->queued = true;
			q.push(lnk->a);
		    }
		}
	    }
	}
	area->need.clear();
    }

}

void World::trace (vect2d pt, Area* area, double ang, double fov, Uint8* aseen)
{
    PROBEGINF;
    memset (aseen, 0, area_bit_size);
    if (fov == 0) {
	int ta = 0;
	for (ta = 0; ta < 3; ta++, ang += (M_PI*2/3)) {
	    view_trace_nonrecurs (pt, area, ang - (M_PI / 3), ang + (M_PI / 3), aseen);
	    FOREACH(s, areas) {
		(*s)->traced.clear();
		(*s)->need.clear();
	    }
	}
    } else {
	view_trace_nonrecurs (pt, area, ang - fov, ang + fov, aseen);
	FOREACH(s, areas) {
	    (*s)->traced.clear();
	    (*s)->need.clear();
	}

    }

    PROENDF;
}

void World::add(Actor* a) {
    pendingactors.push_back(a);
    map<int, sptr<Actor> >& byID(a->is_static ? staticactorByID : actorByID);
    map<int, sptr<Actor> >::iterator prev;
    if (a->id == -1) {
	a->id = 0;
	while ((prev = byID.find(a->id)) != byID.end()) a->id++;
    }
    byID[a->id] = a;
	
}

sptr<Actor> World::findactor(int id) {
    map<int, sptr<Actor> >::iterator itr = actorByID.find(id);
    if (itr == actorByID.end())
	return NULL;
    return itr->second;
    
}
void World::remove_stale_actors() {
    FOREACH(itr, actors) {
	Actor* a = *itr;
	if (a->world == NULL) {
	    map<int, sptr<Actor> >::iterator mitr = actorByID.find(a->id);
	    if (mitr != actorByID.end())
		actorByID.erase(mitr);

	    actors.erase(itr--);
	}
    }
}

void World::moveactors(double time) {
    FOREACH(itr, actors) {
	sptr<Actor> a = *itr;
	if (a->world == NULL) continue;
	sptr<ServerActor> sa = a.cast<ServerActor>();
	if (sa != NULL && sa->solidnum) {
	    vect3d oloc = sa->loc;
	    sa->move(time, true);
	    if (oloc != sa->loc)
		sa->checkareas();
	}
    }
    FOREACH(itr, actors) {
	sptr<Actor> a = *itr;
	if (a->world == NULL) continue;
	sptr<ServerActor> sa = a.cast<ServerActor>();
	if (sa != NULL && !sa->solidnum) {
	    vect3d oloc = sa->loc;
	    sa->move(time, true);
	    if (oloc != sa->loc)
		sa->checkareas();
	}
    }
    FOREACH(itr, actors) {
	sptr<Actor> a = *itr;
	if (a->world == NULL) continue;
	sptr<ServerActor> sa = a.cast<ServerActor>();
	if (sa != NULL)
	    sa->tick(time);
    }
}

void World::add_pending_actors() {
    FOREACH(itr, pendingactors) {
	sptr<Actor> a = *itr;

	a->world = this;
	if (a->is_static)
	    staticactors.push_back(a);
	else
	    actors.push_back(a);
	a->checkareas();
    }
    FOREACH(itr, pendingactors) {
	Actor* a = *itr;
	if (a->is_static) a->area->staticactors.insert(a);
	ServerActor* sa = dynamic_cast<ServerActor*>(a);
	if (sa != NULL) {
	    sa->init();
	}
    }
    pendingactors.clear();
}

World::~World() {
    /*
    FOREACH(itr, pendingactors) {
	(*itr)->remove();
    }
    FOREACH(itr, actors) {
	(*itr)->remove();
    }
    FOREACH(itr, staticactors) {
	(*itr)->remove();
    }
    */
}

static bool intersect_plane(Area* limit, Plane* pl, vect3d v1, vect3d v2, vect3d& nv) 
{

    v1.z -= pl->getz(v1);
    v2.z -= pl->getz(v2);

    if (v1.z>0 == v2.z>0) return false;
    
    nv.x = v1.x - v1.z*(v2.x-v1.x)/(v2.z-v1.z);
    nv.y = v1.y - v1.z*(v2.y-v1.y)/(v2.z-v1.z);
    nv.z = pl->getz(nv);
    return limit->pointin(nv);
}

static bool intersect_flat_plane(double z, vect2d pt, double r, 
				 vect3d v1, vect3d v2, vect3d& nv)
{

    v1.z -= z;
    v2.z -= z;

    if (v1.z>0 == v2.z>0) return false;
    
    nv.x = v1.x - v1.z*(v2.x-v1.x)/(v2.z-v1.z);
    nv.y = v1.y - v1.z*(v2.y-v1.y)/(v2.z-v1.z);
    nv.z = z;

    if (vlen2(vect2d(nv)-pt) > sqr(r)) return false;
    return true;
}



static int intersect_actor(Actor* a, vect3d v1, vect3d v2, 
			   vect3d* pts)
{

    int npts=0;
#define addpt(pt) ({ npts++; *pts++ = (pt); })

    vect3d nv;
    
    if (a->height == 0) return 0;

    if (intersect_flat_plane(a->loc.z + a->height, a->loc, a->rad, v1, v2, nv))
	addpt(nv);

    if (intersect_flat_plane(a->loc.z - a->height, a->loc, a->rad, v1, v2, nv))
	addpt(nv);

    if (vect2d(v1) == vect2d(v2)) return npts;

    vect3d dv(v2-v1);

    double l = vlen(vect2d(dv));
    dv /= l;

    double doti = fabs(~dv*(v1 - a->loc));
    double dotj = vect2d(a->loc-v1)*vect2d(dv);

    if (doti > a->rad) return npts;

    double ia = sqrt(sqr(a->rad)-sqr(doti));
    double ta = dotj + ia;
    if (ta >= 0 && ta <= l) {
	nv = dv*ta+v1;
	if (nv.z >= a->loc.z-a->height && nv.z <= a->loc.z+a->height) addpt(nv);
    }
    ta = dotj - ia;
    if (ta >= 0 && ta <= l) {
	nv = dv*ta+v1;
	if (nv.z >= a->loc.z-a->height && nv.z <= a->loc.z+a->height) addpt(nv);
    }

    return npts;
    
}

static int intersect_wall(Wall* w, vect3d v1, vect3d v2, vect3d& nv, Area*& other, bool solidactor = false)
{
    if (vect2d(v1) == vect2d(v2)) return 0;
    
    vect2d iv;
    if (!w->intersect(v1, v2, iv)) return 0;
    nv = iv;
    if (fabs(v2.x-v1.x)<fabs(v2.y-v1.y)) {
	nv.z=(v2.z-v1.z)*(nv.y-v1.y)/(v2.y-v1.y)+v1.z;
    } else {
	nv.z=(v2.z-v1.z)*(nv.x-v1.x)/(v2.x-v1.x)+v1.z;
    }
    if (solidactor) {
	double fz = w->area->floor->getz(nv);
	double cz = w->area->ceil->getz(nv);
	if (nv.z >= fz && nv.z <= cz) {
	    return 1;
	}
	return 0;

    } else {
	FOREACH(itr, w->links) {
	    Area* tother = itr->a;
	    double fz = tother->floor->getz(nv);
	    double cz = tother->ceil->getz(nv);
	    
	    if (nv.z<=fz || nv.z>=cz) continue;
	    
	    other = tother;
	    return 2;
	}
    }

    return 1;
}

#define ckpt(npt, bm) ({				\
	double pdot=(npt-v1)*dv;			\
	if (pdot <= cmdot) {				\
	    cmdot = pdot;				\
	    tv = (npt);					\
	    bump = (dynamic_cast<Bumpable*>(&*bm));	\
	}						\
    })

static void intersect_area_solid(Area* area, vect3d v1, vect3d v2, vect3d& tv, 
				 sptr<Bumpable>& bump, ActorFilter& filt, unsigned char* acheck) {
    
    vect3d dv(v2-v1);
    double l2 = vlen2(dv);
    double l = sqrt(l2);

    dv /= l;

    double cmdot=l;
    vect3d nv;

    tv=v2;

    if (intersect_plane(area, area->floor, v1, v2, nv)) {
	ckpt(nv, area->floor);
    }
    if (intersect_plane(area, area->ceil, v1, v2, nv)) {
	ckpt(nv, area->ceil);
    }
    FOREACHW(w, area) {
	double tdot=~w->j * vect2d(dv);
	    
	if (tdot>0) {
	    Area* other = NULL;
	    int res= intersect_wall(&*w, v1, v2, nv, other, true);
	    if (res==1) {
		ckpt(nv, &*w);
	    }
	}
    }
}
static void intersect_area(Area* area, vect3d v1, vect3d v2, vect3d& tv, 
			     sptr<Bumpable>& bump, ActorFilter& filt, unsigned char* acheck) {

    SETBIT(acheck, area->id);
    vect3d dv(v2-v1);
    double l2 = vlen2(dv);
    double l = sqrt(l2);
	
    dv /= l;

    tv=v2;

    double cmdot=l;
    vect3d nv;

    int i;


    if (&filt != &noactors) {
	FOREACH(itr, area->actors) {
	    Actor* a = *itr;
	    if (!filt.filter(a)) continue;

	    if (a->solidnum) {
		if (((unsigned)a->solidnum-1) >= a->world->extragroups.size())
		    continue;
		vector< sptr<Area> >& cg(a->world->extragroups.at(a->solidnum-1));
		FOREACH(ar, cg) {
		    vect3d tnv;
		    intersect_area_solid(*ar, v1 - a->loc, v2 - a->loc, tnv, bump, filt, acheck);
		    tnv += a->loc;
		    ckpt(tnv, a);
		}
	    } else {

		vect3d pts[4];
		
		int npts = intersect_actor(a, v1, v2, pts);
		vect3d *tpts=pts;
		for (i = 0; i < npts; i++, tpts++) {
		    ckpt(*tpts, a);
		}
	    }
	}
    }
    FOREACHW(w, area) {
	double tdot=~w->j * vect2d(dv);
	    
	if (tdot>0) {
	    Area* other = NULL;
	    int res= intersect_wall(&*w, v1, v2, nv, other);
	    if (res==1) {
		ckpt(nv, &*w);
	    } else if (res==2) {
		if (!ISSET(acheck, other->id)) {
		    sptr<Bumpable> a = NULL;
		    vect3d tnv;
		    intersect_area(other, nv, v2, tnv, a, filt, acheck);
		    ckpt(tnv, a);
		}
	    }
	}
    }
    if (intersect_plane(area, area->floor, v1, v2, nv)) {
	ckpt(nv, area->floor);
    }
    if (intersect_plane(area, area->ceil, v1, v2, nv)) {
	ckpt(nv, area->ceil);
    }
    CLEARBIT(acheck, area->id);
    
}
void World::trace(vect2d loc, Area* start, set<int>& areaset) {


    unsigned char aseen[area_bit_size];
    trace(loc, start, 0, 0, aseen);
    FOREACH(a, areas) {
	if (ISSET(aseen, (*a)->id))
	    areaset.insert((*a)->id);
    }
}

vect3d World::trace(vect3d v1, Area* start, vect3d v2, ActorFilter& filt, sptr<Bumpable>& bump) 
{
    vect3d ov1(v1);

    vect3d tv;
    
    bump = NULL;

    unsigned char acheck[area_bit_size];
    memset (acheck, 0, area_bit_size);

    intersect_area(start, v1, v2, tv, bump, filt, acheck);

    return tv;
}

struct ActorTemp {
    string clas;
    double x, y, z, ang;
    sptr<Area> area;

    Uint8* data;
    int datalen;
};


static sptr<Plane> readplane(DataInput& di) {
    sptr<Plane> cp = GC::track(new Plane());

    cp->cpt.x = di.readf();
    cp->cpt.y = di.readf();
    cp->cpt.z = di.readf();

    cp->dir.x = di.readf();
    cp->dir.y = di.readf();

    cp->slope = di.readf();
    cp->pan.x = di.readf();
    cp->pan.y = di.readf();
    cp->scale.x = di.readf();
    cp->scale.y = di.readf();

    cp->flags = di.readu16();

    cp->tex = Resource::get<Texture>(di.readstring()+".tex");

    return cp;
}
/*
sptr<Plane> getplane(vector<sptr<Area>>& cav, DataInput& di) {
    int fc = di.readu8();
    if (fc == 2) {
	return readplane(di);
    } else {
	int areanum = di.readu16();
	sptr<Area> a = cav.at(areanum);
	if (fc == 1) {
	    return a->floor;
	} else {
	    return a->ceil;
	}
    }
}
*/
sptr<Wall> readwall(vector<sptr<Area> >& cav, DataInput& di) {
    sptr<Wall> ret = GC::track(new Wall());

    ret->v1.x = di.readf();
    ret->v1.y = di.readf();
    
    int nlinks = di.readu8();

    ret->links.resize(nlinks);
    
    for (int i = 0; i < nlinks; i++) {
	ret->links[i].a = cav.at(di.readu16());
	ret->links[i].flags = di.readu8();
    }

    int nparts = di.readu8();

    ret->parts.resize(nparts);
    
    for (int i = 0; i < nparts; i++) {
	WallPart* p = &ret->parts[i];
	
	p->tex = Resource::get<Texture>(di.readstring()+".tex");

	p->flags = di.readu8();

	p->pan.x = di.readf();
	p->pan.y = di.readf();
	p->scale.x = di.readf();
	p->scale.y = di.readf();

	p->fza = di.readf();
	p->fzb = di.readf();
	p->cza = di.readf();
	p->czb = di.readf();

    }
    return ret;
}

void World::load(DataInput& di, ActorLoader& al) {
    int numgroups = di.readu16();
    int totareas= 0;
    int totalwalls = 0;

    extragroups.resize(numgroups-1);

    for (int cg = 0; cg < numgroups; cg++) {
	vector<sptr<Area> >& cav(cg == 0 ? areas : extragroups[cg - 1]);

	int numareas = di.readu16();
	
	cav.resize(numareas);
	
	vector<ActorTemp> tactors;
	// Initialize areas (so we can refer to them by pointer)


	for (int i = 0; i < numareas; i++) {
	    cav[i] = GC::track(new Area());
	}

	for (int i = 0; i < numareas; i++) {
	    sptr<Area> ca = cav[i];
	    
	    ca->id = totareas++;
	    
	    ca->name = di.readstring();
	    
	    ca->queued = false;

	    ca->ceil = readplane(di);
	    ca->floor = readplane(di);
	    
	    ca->ceil->ceil = cg == 0;
	    ca->floor->ceil = cg != 0;

	    ca->ceil->revalidate();
	    ca->floor->revalidate();
	    
	    int ncont = di.readu8();
	    
	    ca->firstcontour = NULL;
	    sptr<Wall> lastw = NULL;
	    sptr<Wall> cbegin = NULL;
	    
	    ca->nwalls = 0;
	    
	    for (int cc = 0; cc < ncont; cc++) {
		int nwalls = di.readu8();
		
		cbegin = NULL;
		
		for (int cw = 0; cw < nwalls; cw++) {
		    sptr<Wall> w = readwall(cav, di);
		    
		    ca->nwalls++;
		    w->id = totalwalls++;
		
		    w->area = ca;
		    w->next = NULL;
		    w->nextcontour = NULL;
		    
		    if (cbegin == NULL) cbegin = w;
		    
		    if (lastw != NULL) {
			if (lastw->next == NULL) {
			    lastw->next = w;
			    lastw->revalidate();
			}
			
			lastw->nextcontour = w;
		    }
		    lastw = w;
		}
		if (ca->firstcontour == NULL) {
		    ca->firstcontour=cbegin;
		}
		if (cbegin != NULL && lastw != NULL) {
		    lastw->next = cbegin;
		    lastw->revalidate();
		}
	    }


	    int nact = di.readu16();
	    for (int cc = 0; cc < nact; cc++) {
		ActorTemp a;
		
		a.clas = di.readstring();
		a.x = di.readf();
		a.y = di.readf();
		a.z = di.readf();
		a.ang = di.readf();
		a.area = ca;
		
		a.datalen = di.readu16();
		a.data = new Uint8[a.datalen];
		di.read(a.data, a.datalen);

		tactors.push_back(a);
	    }
	    
	    if (hasgraph) {
		ca->tess = new AreaTess(ca);
		ca->tess->revalidate();
	    }
	}
	FOREACH(srca, tactors) {
	    MemoryDataInput mdi(srca->data, srca->datalen);
	    sptr<Actor> act = al.load(srca->clas, vect3d(srca->x, srca->y, srca->z), 
				 srca->area, srca->ang, mdi);
	    if (act != NULL) {
		add(act);
	    }
	}
    }

    
    area_bit_size = (totareas+7)/8;
    wall_bit_size = (totalwalls+7)/8;
    
    

}


World::World(DataInput& di, ActorLoader& al) {
    load(di, al);
}
World::World(FILE* f, ActorLoader& al) {
    FileDataInput di(f, false);
    load(di, al);
}
World::World(const char* fname, ActorLoader& al) {
    gzFile f = gzopen(fname, "rb");
    if (!f) {
	throw ios_base::failure("cannot open file");
    }
    GzipFileDataInput di(f);
    load(di, al);
}

sptr<Area> World::getarea(const string& name) {
    map<string, sptr<Area> >::iterator it(areaByName.find(name));
    if (it == areaByName.end()) return NULL;
    return it->second;
}


void init_engine(void) {
}
void destructor_breakpoint() {
    cout << "destructor breakpoint!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
}
