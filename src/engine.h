// -*- c++ -*-

// Real Programmers don't write comments. It was hard to write,
// it should be hard to read.

// Seriously, though, I'll try to comment whatever I can.
// See README for general details on the engine.

#ifndef _ENGINE_H_
#define _ENGINE_H_


#include <SDL.h>
#include <SDL_image.h>

#include <cstdlib>

#include <math.h>

#include <vector>
#include <set>
#include <map>
#include <iostream>

#include "common.h"
#include "object.h"

#include "resource.h"
#include "datastream.h"
#include "vector.h"

#define MASTER_SCALE (1.0/64.0)

class Area;
class Effect;
class ServerActor;
class Actor;
class World;
class Plane;
class Clipper;
class Communicator;

class DrawManager;
class Texture : public Resource {
public:
    int w, h;

    virtual void setup(DrawManager& dm) { }
    virtual ~Texture() { }

    static string loaderclass;

    DECLARE_OBJECT;
};

// @OBJDEF WallPart
// sptr().visit("(%s).tex"%obj, n)
// sptr().clear("(%s).tex"%obj, n)

class WallPart {
public:
    void draw(DrawManager& dm, double x1, double y1, double jx, double jy, double len);
    
    double fza, fzb, cza, czb;

    sptr<Texture> tex;
    int flags;
    vect2d pan, scale;
};

class Bumpable : virtual public Object {
public:

    virtual double takemomentum(vect3d norm) { return 0.0; }
    virtual double givemomentum(Bumpable* hit, Actor* a, vect3d norm, double momentum);

    DECLARE_OBJECT;
};

// A wall.

// Hey, you wanted comments, you get comments.

// @OBJDEF Link 
// sptr().visit("(%s).a"%obj, n)
// sptr().clear("(%s).a"%obj, n)


struct Link {
    sptr<Area> a;
    int flags;

    Link(Area* a = NULL, int flags = 0);
};

////// 

// An arbitrary, non-vertical plane.
// 
// Planes are defined by a control point (x,y,z), a unit vector
// (ix,iy) which gives the direction of the slope, and a slope value
// which determines the magnitude of the slope.


class Plane : public Bumpable {
public:
    Plane() {
	matrix = NULL;
	tex = NULL;
    }
    



    Plane(bool ceil, Texture* tex, int flags=0, vect2d pan=vect2d(), vect2d scale=vect2d(1.0,1.0), vect3d cpt=vect3d(), 
	  vect2d dir=vect2d(0,1), double slope=0) : cpt(cpt),dir(dir),slope(slope),tex(tex),flags(flags),pan(pan),
						    scale(scale), sfact(0.0), ceil(ceil) {
	matrix = NULL;
	revalidate();
    }
    ~Plane() {
	delete[] matrix;
	matrix = NULL;
    }

    double givemomentum(Bumpable* o, Actor* a, vect3d norm, double mom) { return -mom; }

    virtual string str() const throw(); 

    // Calculates internal values used by the plane. call after
    // changing the slope, unit vector, or control point x or y. This
    // does not have to be called when you only change control point
    // z, texture, flags, or panning.

    void revalidate();

    // Draws the plane using the area's tesselation.

    void draw(DrawManager& dm, Area* area);

    // get the minimum or maximum z value for which an actor's clipping
    // cylinder is on the correct side and does not intersect with this
    // plane. If this plane is a ceiling, returns the maximum z,
    // otherwise returns the minimum. If limit is not NULL, limits
    // testing to points inside the area.

    // For details on the algorithm, see the comments in the
    // implementation. 

    double getzbound(Area* limit, Actor* a, bool clipsolid = false);

    // Same, but explicitly specifying the clipping cylinder. Values
    // are the same as the corresponding field in the Actor class.
    double getzbound(Area* limit, vect2d pt, double rad, double height, bool clipsolid = false);

    // get the z value for <tx,ty>.
    inline double getz(double tx, double ty) {
	return getz(vect2d(tx,ty));
    };
    inline double getz(const vect2d& pt) {
	return cpt.z + ((pt-vect2d(cpt))*dir) * slope;
    }
    void setup_texture_matrix(Texture* ti);

    // The control point.
    vect3d cpt;

    // The unit vector giving the slope direction.
    vect2d dir;

    // The slope magnitude.
    double slope;
    
    // The texture
    sptr<Texture> tex;

    // Flags for drawing this plane
    int flags;

    // Texture panning and scaling
    vect2d pan, scale;

    // The slope scale factor. At some point in the future, this will be
    // used for correcting the stretching caused by sloping. Currently
    // it is unused. Calculated automatically by revalidate()
    double sfact;

    // True if this plane is a ceiling, false if it is a floor.
    bool ceil;

    // A transformation matrix for drawing this plane. Calculated
    // automatically by revalidate().
    double *matrix;

    DECLARE_OBJECT;
};

class Wall : public Bumpable {
public:
    Wall();
    

    // Calculates the wall's length, unit vector, and slope.
    // call whenever you move the wall or it's nextwall.
    void revalidate();

    // clip circle <tx,ty,rad> to the wall. If the circle intersects the wall,
    // returns true and sets <clipx, clipy> (if not null) to the closest center
    // point for which the circle does not intersect this wall (although it may touch 
    // the wall). If priority is not null, it is set to a value that can be used
    // to prioritize clipping. If surfaceepsilon is specified, the circle is pushed
    // a small amount further away from the wall so that it does not touch.
    //
    // If the circle does not intersect, returns false.

    bool clippoint (vect2d pt, double rad, vect2d *clip=NULL, vect2d* hitpt=NULL, double *priority=NULL,
		   double surfaceepsilon=0.0);

    // Check if a line intersects the wall. If the line <x1,y1> - <x2,y2> intersects, 
    // returns true and sets <hx,hy> to the point of intersection.

    bool intersect (vect2d v1, vect2d v2, vect2d& hit);

    void draw(DrawManager& dm);

    virtual string str() const throw();

    // This wall's point.
    // The wall goes from <v1.x,v1.y> to <next->v1.x,next->v1.y>
    vect2d v1;

    // The next wall in the contour.
    sptr<Wall> next;

    // The beginning of the next contour if this is the last wall in
    // a contour. Otherwise, it should be set to the next wall in the
    // current contour. If this is the last wall in the last contour,
    // it should be NULL.

    sptr<Wall> nextcontour;

    // Length from pt to next->pt
    // Cache a normalized vector from point 1 to point 2.
    // Calculated automatically by revalidate().
    vect2d j;
    double len;


    // Texture stretching and panning.

    // The inverse slope of the wall, (y2-y1) / (x2-x1).
    // Calculated automatically by revalidate().
    double tslope;

    // The area that this wall belongs to.
    sptr<Area> area;

    // The links to areas on the other side of this wall
    vector<Link> links;

    vector<WallPart> parts;

    // Wall number; used for bit field testing.
    int id;

    DECLARE_OBJECT;
};

#define FOREACHW(w,area) for (Wall* w = area->firstcontour; w != NULL; w = w->nextcontour) 

class AreaTess : public GLResource {
    int name;
    Area* area;

public:
    AreaTess(Area* area);
    void call();
    void release();
    void revalidate();
};

struct range {
    double s, e;
    range() : s(0), e(0) { };
    range(double s, double e) : s(s), e(e) { }
};

class rangenode : public GCList {
    static rangenode* alloc();

public:
    range r;

    rangenode() { }
    rangenode(range r) : r(r) { }
    rangenode(double s, double e) : r(s, e) { }

    static GCList pool;
    static inline rangenode* get(range r = range()) {
	rangenode* ret;
	if (pool.empty()) 
	    ret = alloc();
	else
	    ret = static_cast<rangenode*>(pool.next);
	ret->r = r;
	return ret;
    }

    inline void movetopool() {
	moveto(&pool);
    }

};

class rangeiterator : public iterator<bidirectional_iterator_tag, rangenode> {
    const GCList* l;
public:
    rangeiterator(const GCList* l) : l(l) { }
    const range& operator*() {
	return static_cast<const rangenode*>(l)->r;
    }

    const range* operator->() {
	return &static_cast<const rangenode*>(l)->r;
    }

    rangeiterator& operator++() {
	l = l->next;
	return *this;
    }

    rangeiterator operator++(int) {
	rangeiterator tmp = *this;
	l = l->next;
	return tmp;
    }

    rangeiterator& operator--() {
	l = l->prev;
	return *this;
    }

    rangeiterator operator--(int) {
	rangeiterator tmp = *this;
	l = l->prev;
	return tmp;
    }
    
    bool operator==(rangeiterator o) {
	return l == o.l;
    }
    bool operator!=(rangeiterator o) {
	return l != o.l;
    }
};

class arcset {

public:
    GCList ranges;

    arcset() { }
    arcset(const arcset& o) {
	*this = o;
    }
    arcset& operator=(const arcset& o) {
	clear();
	//cerr <<"warning: arcset copy"<<endl;
	FOREACH(r, o) {
	    rangenode::get(*r)->moveto(&ranges);
	}
        return *this;
    }

    void insert(range r) {
	if (r.e <= r.s) 
	    return;

	GCList *itr;

	itr = ranges.next;

	while (itr != &ranges) {
	    range& tr(static_cast<rangenode*>(itr)->r);

	    if (r.s < tr.s) {
		break;
	    }
	    if (r.s <= tr.e) {
		r.s = tr.s;
		if (r.e <= tr.e)
		    return;
		break;
	    }

	    itr = itr->next;
	}

	while (itr != &ranges) {
	    range& tr(static_cast<rangenode*>(itr)->r);
	    GCList* next = itr->next;

	    if (r.e < tr.s) {
		break;
	    }
	    if (r.e <= tr.e) {
		r.e = tr.e;
		itr->moveto(&rangenode::pool);
		itr = next;
		break;
	    }

	    itr->moveto(&rangenode::pool);
	    itr = next;
	}
	rangenode::get(r)->moveto(itr);
    }
    
    void remove(range r) {
	if (r.e <= r.s) 
	    return;

	GCList *itr;

	itr = ranges.next;

	while (itr != &ranges) {
	    range& tr(static_cast<rangenode*>(itr)->r);

	    if (r.s < tr.s) {
		if (r.e <= tr.s)
		    return;
		break;
	    }
	    if (r.s <= tr.e) {
		rangenode::get(range(tr.s, r.s))->moveto(itr);
		break;
	    }

	    itr = itr->next;
	}


	while (itr != &ranges) {
	    range& tr(static_cast<rangenode*>(itr)->r);
	    GCList* next = itr->next;

	    if (r.e < tr.s) {
		break;
	    }
	    if (r.e < tr.e) {
		tr.s = r.e;
		break;
	    }

	    itr->moveto(&rangenode::pool);
	    itr = next;
	}
    }

    arcset& operator+=(range r) {
	insert(r);
	return *this;
    }
    arcset& operator+=(const arcset& o) {
	FOREACH(r, o) {
	    insert(*r);
	}
	return *this;
    }

    arcset& operator-=(range r) {
	remove(r);
	return *this;
    }
    
    arcset& operator-=(const arcset& o) {
	FOREACH(r, o) {
	    remove(*r);
	}
	return *this;
    }
    arcset operator+(const arcset& o) const {
	return (arcset(*this) += o);
    }
    arcset operator-(const arcset& o) const {
	return (arcset(*this) -= o);
    }
    arcset operator+(range r) const {
	return (arcset(*this) += r);
    }
    arcset operator-(range r) const {
	return (arcset(*this) -= r);
    }

    rangeiterator begin() const {
	return rangeiterator(ranges.next);
    }
    rangeiterator end() const {
	return rangeiterator(&ranges);
    }
    void clear() {
	rangenode::pool.merge(&ranges);
    }
};

class Area : public Object {
public:

    Area();
    
    ~Area();

    // Returns true if the given point is in the area.

    bool pointin(double x, double y);
    bool pointin(vect2d v);

    void draw(DrawManager& dm, bool walls = true);

    // The first wall in the first contour
    sptr<Wall> firstcontour;

    int nwalls;

    // The tesselation display list. set this to an OpenGL display
    // list by calling glGenLists(1) and then call Tesselate().

    AreaTess* tess;

    // The floor and ceiling of the area. ceil.ceil must be true and
    // floor.ceil must be false or funny things will happen.
    sptr<Plane> floor, ceil;

    // The area number, used for bit fields and identifying the area 
    // over the network.
    int id;

    // The area name, from the world file
    string name;

    bool invert;

    // The set of actors completely or partially in this area.
    set< sptr<Actor> > actors;

    // The set of static actors in this area.
    set< sptr<Actor> > staticactors;

    string str() const throw() { return string("Area ") + name; }


    arcset traced;
    arcset need;
    bool queued;

    DECLARE_OBJECT;
};

typedef enum { PT_UNUSED=0, PT_ID, PT_INT, PT_FLOAT_LERP, PT_FLOAT_NOLERP, PT_STRING } ParamType;
#define PT_FLOAT PT_FLOAT_LERP

class Parameter {
public:
    ParamType type;
    int ival;
    double dval;
    string sval;

    Uint32 flags;
    Uint32 timestamp;

    Parameter() : type(PT_ID), ival(0), dval(0), sval(""), flags(0), timestamp(0) { }
    
    
};


class Appearance : public Object {
public:
    Appearance(Actor* act);
    virtual ~Appearance();
    virtual void runsound() { }
    virtual void draw(DrawManager& dm) { }

    virtual void paramchanged(int num) { }

    map<int,Parameter> params;

    // True if this Drawable needs to be sorted from back to front.
    // (i.e. if it uses transparency effects)
    bool needsort;

    // The distance from the view plane to this object, set by the
    // drawing code for sorting.
    double sortz;

    bool willdraw;

    sptr<Actor> act;

    DECLARE_OBJECT;
};
// An actor is an object that can be clipped to the world and other actors.



// Used for World::trace to determine which actors should be checked.
class ActorFilter {
public:
    virtual bool filter(Actor* t) {
	return true;
    }
    virtual ~ActorFilter() { }
};
class NothingFilter : public ActorFilter {
public:
    bool filter(Actor* t) {
	return false;
    }
};
class ExcludeOneFilter : public ActorFilter {
    sptr<Actor> act;
public:
    ExcludeOneFilter(Actor* act) : act(act) { }
    bool filter(Actor* a) {
	return a != act;
    }
};


// An ActorFilter that matches all actors.
extern ActorFilter allactors;
extern NothingFilter noactors;
extern sptr<Bumpable> __dummy;

class Actor : public Bumpable {
public:
    Actor();

    // move the actor by its x, y, and z velocity, clipping to the
    // world and other actors.
    void checkareas();
    void remove();
    virtual void onremove() { };

    virtual ~Actor();    
    // This actor's current <x,y,z> location and area.
    vect3d loc;
    sptr<Area> area;
    double ang;

    // The clipping cylinder.
    double rad, height;

    int solidnum;

    // The world that this actor is associated with.
    sptr<World> world;


    sptr<Appearance> appearance;

    // A list of all the areas occupied by this actor. More
    // specifically, a list of all areas for which this actor is in
    // the set area->things.
    vector< sptr<Area> > areas;

    bool is_static;
    

    int id;


    DECLARE_OBJECT;
};

class ServerActor : public Actor {
public:

    ServerActor();

    virtual ~ServerActor();
    virtual void init();

    virtual string str() const throw();
    virtual bool moveto(vect3d npt, Clipper& cr);
    virtual void move(double time, bool bump);
    virtual void blockedby(ServerActor* o) { };
    virtual bool blocks(ServerActor* o) { return true; };

    virtual void onremove();

    // Called every frame.
    virtual void tick(double time) { };

    virtual void think() { };

    // Return true if the other actor should be clipped to this
    // actor. Note that this can be overridden by the other actor by
    // overriding clippedby(). Default is false.
    virtual bool clips(ServerActor* t) { return false; }

    // Return true if this actor should be clipped by the other
    // actor. Default is to ask the other actor if we should be
    // clipped by it.
    virtual bool clippedby(ServerActor* t) { return t->clips(this); }

    vect3d vel;
    double angv;

    double thinkinterval;
    double nextthink;

    // The step height, the maximum stair-step height. This should
    // probably be between (height/2) and height. Experiment with
    // different values to find one that works.
    double stepheight;

    sptr<Communicator> comm;

    // This actor's Drawable. If not null, the Drawable will be drawn
    // in this actor's location when it is visible.


    DECLARE_OBJECT;
};

class Communicator : public Object {
public:
    sptr<ServerActor> a;
    Communicator (ServerActor* a) : a(a) { }
    virtual ~Communicator () { }
    
    virtual void init() { }
    virtual void playsound(const char* snd, double vol = 1.0) { };
    virtual void playsound(const char* snd, vect3d loc, double vol = 1.0) { };
    virtual void set_client_class(string clas)=0;
    virtual void setparameter(int num, ParamType type, int val)=0;
    virtual void setparameter(int num, ParamType  type, double val)=0;
    virtual void setparameter(int num, string val)=0;

    DECLARE_OBJECT;
};



class ActorLoader {
public:
    virtual sptr<Actor> load(string& clas, vect3d loc, Area* area, double ang, DataInput& ds)=0;
    virtual ~ActorLoader() { }
};


// A World is a collection of all items neccesary for drawing and
// clipping to a world.

typedef vector< pair<int,vect2d> > tracepath;
extern tracepath __dummy_tracepath;



class World : public Object {
public:
    World() { };
    World(DataInput& tk, ActorLoader& al);
    World(FILE* in, ActorLoader& al);
    World(const char* filename, ActorLoader& al);

    vector < sptr<Area> > areas;
    vector < vector< sptr<Area> > > extragroups;

    vector < sptr<Actor> > pendingactors;
    vector < sptr<Actor> > actors;
    vector < sptr<Actor> > staticactors;

    map <int, sptr<Actor> > actorByID;
    map <int, sptr<Actor> > staticactorByID;

    

    void add(Actor* a);

    void remove_stale_actors();
    void moveactors(double time);
    void add_pending_actors();
    sptr<Actor> findactor(int id);
    
    // draw the view, from <x,y,z> in the specified area, looking in
    // the direction "ang". vofs is the vertical slope. e.g. vofs = 1
    // means look up 45 degrees. xfov and yfov specify the horizontal
    // and vertical field of view, respectively. An xfov of 1 means a
    // horizontal field of view of 90 degrees, likewise for yfov.
    void drawview (vect3d loc, Area* area, double ang, double vofs,
		   double xfov, double yfov, ActorFilter& filt=allactors);

    // Given a starting area and location <ox,oy>, finds the area
    // containing <nx,ny> by direct line of sight. If the area
    // cannot be found, returns the original area.
    sptr<Area> findarea(vect3d o, Area* start, vect3d n);

    //bool TracePath(vect2d o, int sn, vect2d n, tracepath& path);
    // trace from <x1,y1,z1,s1> to <x2,y2,z2>, checking walls, ceilings,
    // floors, bridges, and optionally actors. Returns the fraction
    // along the line to the hit point (like Quake) and stores the hit
    // point in <x2,y2,z2>. If nothing is hit, returns 1.0 and leaves
    // <x2,y2,z2> unchanged. If an actor is hit, stores a pointer to the actor in
    // act. If filt is specified, the filter is used to determine
    // which actors to check.
    //
    
    vect3d trace(vect3d a, Area* startarea, vect3d b, ActorFilter& filt = allactors, sptr<Bumpable>& hit = __dummy);

    void trace(vect2d loc, Area* startarea, set<int>& areaset);
    void trace(vect2d loc, Area* startarea, double ang, double fov, Uint8* areaseen);

    sptr<Area> getarea(const string&);

    ~World();

    // The minimum size of a bit field for areas.
    // equal to (areas.size()+7)/8
    int area_bit_size;

    // The minimum size of a bit field for walls.
    // equal to (<maxumim wall id>+7)/8
    int wall_bit_size;


private:
    map<string, sptr<Area> > areaByName;
    void load(DataInput& tk, ActorLoader& al);

    DECLARE_OBJECT;
};


#define DFLAG_FAILSAFE 1

// Various values used for debugging.
extern int debug_draw;
extern int debug_tweak;

enum {
    DRAWMODE_TEXTURE = 0,
    DRAWMODE_SOLID_OUTLINE_WALLS,
    DRAWMODE_SOLID_OUTLINE_AREABOUNDS,
    DRAWMODE_SOLID_OUTLINE_AREATESS,
    DRAWMODE_WIREFRAME,
    NUM_DRAWMODES
};

extern bool hasgraph;

extern int draweverything;
extern int drawmode;
extern int nowalls;
extern int debug_colors;
extern int debug_revalidate;

void init_engine (void);


#endif
