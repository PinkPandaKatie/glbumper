// -*- c++ -*-

#ifndef _GAME_H_
#define _GAME_H_

#include "common.h"
#include "object.h"

#include "engine.h"

extern bool _game_debug;

class BaseActor : public ServerActor {
public: DECLARE_OBJECT;
    BaseActor() { }



    virtual void tick(double time) { }

    virtual void readargs(DataInput& ts) { }
    virtual ~BaseActor() {
    }

};
#define CLASSN(c)

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
    virtual ~HovercraftCtl() { }
};

/*
class ClientInterface : virtual public Object {
public:

    virtual void text(string txt)=0;

    DECLARE_OBJECT;
};

class GameInterface : virtual public Object {
    sptr<World> world;
    sptr<ClientInterface> iface;

    GameInterface(World* w, ClientInterface* iface) : world(w), iface(iface);

public:

    sptr<GameInterface> create(World* w, ClientInterface* iface);

    virtual void control(int vf, int vs, int vz, int vr)=0;
    virtual bool command(string cmd)=0;

    DECLARE_OBJECT;
};
*/



class GameActorLoader : public ActorLoader {
public:
    vector< sptr<Actor> >& startpts;
    GameActorLoader(vector< sptr<Actor> >& startpts) : startpts(startpts) { }
    sptr<Actor> load(string& clas, vect3d pt, Area* area, double ang, DataInput& di);
    static sptr<Actor> create_hovercraft(HovercraftCtl* ctl);
};

#endif
