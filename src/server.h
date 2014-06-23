// -*- c++ -*-

#ifndef _SERVER_H
#define _SERVER_H

#include "common.h"
#include "object.h"
#include "engine.h"
#include "game.h"
#include "smdp.h"

class ServerThread;
class ServerCommunicator;
class ServerMessageHandler;


class ServerMessageHandler : public MessageHandler, public HovercraftCtl {
    friend class ServerThread;

    double ltime;
    sptr<ServerActor> player;
    sptr<ServerThread> svr;

    map<int, int> last_timestamp;

    Uint32 magic;
    bool valid;
    bool suspend;

    int vf,vs,vz,vr;
    bool horn_;

public: DECLARE_OBJECT;

    int forward_accel() { return vf; }
    int strafe_accel() { return vs; }
    int vert_accel() { return vz; }
    int rotvel() { return vr; }
    bool horn() { if (horn_) { horn_ = false; return true; } return false; }


    ServerMessageHandler(ServerThread* svr);
    bool handle(Message* msg);
    bool timer();

    void newworld();
    void updateactors();
    void disconnected();
};


class ServerCommunicator : public Communicator {
public: DECLARE_OBJECT;

    sptr<ServerThread> svr;

    double stilltime;
    int deltacnt;
    vect3d lastloc;
    
    vect3d oloc;
    sptr<Area> oarea;
    double oang;
    string clclass;

    Uint32 master_ts;

    Uint32 loc_ts;
    Uint32 ang_ts;
    Uint32 size_ts;
    Uint32 clclass_ts;
    Uint32 param_ts;

    int orad;
    map<int, Parameter> params;
    bool sendstill, moving;

    ServerCommunicator(ServerActor* a, ServerThread* svr);
    void set_client_class(string clas);
    void playsound(const char* snd, double vol);
    void playsound(const char* snd, vect3d pos, double vol);
    void tick(double time);
    int writeactor(Message* msg);
    void setparameter(int num, string sval);
    void setparameter(int num, ParamType type, int ival);
    void setparameter(int num, ParamType type, double dval);
    void updatetimestamps();
    bool writeparameters(Message* msg, unsigned int last_timestamp);
    int writeactor(Message* msg, unsigned int last_timestamp);
};

class ServerConnectionManager : public SMDPConnectionManager {
    sptr<ServerThread> svr;

public: DECLARE_OBJECT;

    ServerConnectionManager(ServerThread* svr);
    sptr<MessageHandler> create_handler();
    ~ServerConnectionManager();
};

class ServerThread : public Thread {
    friend class ServerMessageHandler;
    friend class ServerCommunicator;

    sptr<World> world;
    sptr<ServerConnectionManager> mgr;
    sptr<SMDPUDPSocket> sock;
    sptr<SocketGroup> sg;

    int framems;
    double frametime;
    string worldname;
    vector< sptr<Actor> > startpts;
    vector< sptr<Message> > extramsgs;

    int current_timestamp;

public: DECLARE_OBJECT;

    ServerThread(int framems);

    void loadworld(string worldname);
    void listen(int port);
    void connect_loopback(SMDPLoopbackConnection* conn);
    void quit();
    void stop();

    int run();
};


#endif
