// -*- c++ -*-

#ifndef _CLIENT_H
#define _CLIENT_H

#include "common.h"
#include "object.h"
#include "engine.h"
#include "resource.h"
#include "sound.h"
#include "smdp.h"
#include "text.h"
#include "ui.h"

class ClientComponent;
class ActorParams;
class ActorProxy;

enum ConnectState {
    CONN_DISCONNECTED,
    CONN_SENT_HELLO,
    CONN_SENT_JOIN,
    CONN_CONNECTED
};

// @OBJDEF ActorParams
// sptr().visit("(%s).area"%obj, n)
// sptr().clear("(%s).area"%obj, n)

#define NUMPARAMS 32

class ActorParams {
public:
    vect3d loc;
    sptr<Area> area;
    double ang;

    bool lerp;
    
    double rad;

    Parameter params[NUMPARAMS];
    
    ActorParams() : loc(0,0,0), area(NULL), ang(0), rad(0) { 
	for (int i = 0; i < NUMPARAMS; i++) {
	    params[i].type = PT_UNUSED;
	    params[i].flags = 0;
	}
    }

    ActorParams& operator=(const ActorParams& o) {
	loc = o.loc;
	area = o.area;
	ang = o.ang;
	lerp = o.lerp;
	rad = o.rad;
	for (int i = 0; i < NUMPARAMS; i++) {
	    Parameter& p(params[i]);
	    const Parameter& op(o.params[i]);
	    if (op.type != PT_UNUSED && op.flags&1) {
		p.type = op.type;
		p.flags = op.flags;
		if (op.type == PT_STRING) {
		    p.sval = op.sval;
		} else {
		    p.ival = op.ival;
		    p.dval = op.dval;
		}
	    }
	}
	return *this;
    }
    
};


class ActorProxy : public Actor {
public: DECLARE_OBJECT;

    ActorParams readold, readnew;
    ActorParams commit;
    ActorParams write;

    

    string cclass;
    int flagsset;
    
#ifdef DEBUG_VEL
    double lastx;
    double lastxv;
    double lastxa;
    double lastxda;
#endif

    ActorProxy(int _id) {
	id = _id;
	flagsset=0;
    }


    void readfrom(Message* s, int flags);
    void init();
    void update(double frac);

};

class ClientConnectionManager : public SMDPConnectionManager {
    sptr<ClientComponent> ccomp;
public: DECLARE_OBJECT;

    ClientConnectionManager(ClientComponent* ccomp);
    
    sptr<MessageHandler> create_handler();
};

class ClientMessageHandler : public MessageHandler {
    sptr<ClientComponent> ccomp;

public: DECLARE_OBJECT;
    bool dropped_packet;
    ClientMessageHandler(ClientComponent*);
    bool handle(Message* msg);
    void disconnected();
    void dropped(int num) {
	dropped_packet = true;
    }
    
};

#define SMOOTHCNT 12
class ClientComponent : public Component {
    sptr<SMDPUDPSocket> sock;
    sptr<SocketGroup> sg;
    sptr<SMDPConnection> conn;
    sptr<ClientConnectionManager> mgr;
    sptr<ClientMessageHandler> cmh;

    ConnectState cstate;

    double smoothtimes[SMOOTHCNT];

    int showinfo;
    sptr<Label> info;

    bool suspended;
    int lerpcount;
    ostringstream packetinf;

    double interp_begin;
    double interp_end;
    bool got_endframe;
    double last_endframe_time;

    double lsendtime;
    int framems;
    double frametime;
    int eyeactorid;
    int neweyeactorid;
    
    bool dropwarn;

    int ovf, ovs, ovz, ovr;
    int vf, vf_h, vf_l;
    int vz, vz_h, vz_l;
    int vr, vr_h, vr_l;
    int vs, vs_h, vs_l;

    int vvd, vvd_h, vvd_l;
    int vvh, vvh_h, vvh_l;

    int vd2, vd2_h, vd2_l;

    double vdist;
    double vheight;

    bool profile;

    vect3d viewpt;
    sptr<Area> viewarea;

    sptr<World> world;
    bool interp;

    friend class ClientMessageHandler;
    bool handle_message(Message* msg);

public: DECLARE_OBJECT;

    ClientComponent();
    ~ClientComponent();

    bool event(SDL_Event& evt);
    void tick(double time);
    void draw(int x, int y);

    void connect(string to);
    void connect_loopback(SMDPLoopbackConnection* loop);
    void disconnect();
    void quit();

    void suspend();
    void resume();

    void sendspecial(int cmd);
};





#endif
