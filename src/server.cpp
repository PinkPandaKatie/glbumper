#include "common.h"
#include "object.h"
#include "server.h"

enum {
    DONT_SEND,
    SEND_UNRELIABLE,
    SEND_RELIABLE,
    ERASE_ACTOR
};

/*
 *
 * ServerMessageHandler
 *
 */

#include objdef(ServerMessageHandler)

ServerMessageHandler::ServerMessageHandler(ServerThread* svr) : svr(svr) {
    horn_ = false;
    vf = vs = vz = vr = 0;
    player = NULL;
    valid = false;
    suspend = true;
    magic = rand();
}

void ServerMessageHandler::newworld() {
    last_timestamp.clear();

    player = GameActorLoader::create_hovercraft(this).statcast<ServerActor>();
    
    sptr<Actor> t=NULL;
    t = svr->startpts[rand() % svr->startpts.size()];

    player->loc = t->loc;
    player->area = t->area;
    player->ang = t->ang;

    svr->world->add(player);
    
    suspend = true;

    sptr<Message> smsg = GC::track(new Message(SM_WORLD));
    smsg->writestring(svr->worldname);
    conn->sendmessage(smsg, CHAN_RELIABLE);
    
    smsg = GC::track(new Message(SM_SETEYE));
    smsg->writeshort(player->id);
    conn->sendmessage(smsg, CHAN_RELIABLE);
}

bool ServerMessageHandler::handle(Message* msg) {
    ltime = gettime();
    int inpcmd;
    Uint32 tmagic;
    sptr<Message> smsg;
    if (!valid) {
	if (msg->type == CM_HELLO) {
	    smsg = GC::track(new Message(SM_MAGIC));
	    smsg->writeint(magic);
	    conn->sendmessage(smsg, CHAN_RELIABLE);

	    valid=true;
	    return true;
	} else {
	    cout << "Spurious packet from "<< conn->describe_peer() <<endl;
	    smsg = GC::track(new Message(SM_QUIT));
	    conn->sendmessage(smsg);

	    return false;
	}
    }
    switch (msg->type) {
    case CM_INPUT: 
	inpcmd=msg->readshort();
	vf = ((inpcmd&3)-1);
	vs = (((inpcmd>>2)&3)-1);
	vz = (((inpcmd>>4)&3)-1);
	vr = (((inpcmd>>6)&3)-1);
	break;
    case CM_SPECIAL: 
	    
	inpcmd=msg->readshort();
	switch(inpcmd) {
	case SDLK_SPACE:
	    horn_ = true;
	    break;
	case SDLK_b:
	    _game_debug ^= 1;
	    break;
	case SDLK_k:
	    break;
	default:
	    break;
	}
	break;
    case CM_JOIN:
	tmagic = msg->readint();
	if (tmagic != magic) {
	    cout << "Magic from "<< conn->describe_peer() <<"does not match"<<endl;
	    return false;
	}
	if (player == NULL) {
	    cout << "New connection from "<< conn->describe_peer() <<"."<<endl;
		
	    smsg = GC::track(new Message(SM_INIT));
	    smsg->writeint(svr->framems);
	    conn->sendmessage(smsg, CHAN_RELIABLE);

	    newworld();
	} else {
	    cout << "Spurious join from "<< conn->describe_peer() <<endl;
	}
	break;

    case CM_SUSPEND:
	if (player != NULL)
	    suspend = true;
	break;

    case CM_RESUME:
	if (player != NULL)
	    suspend = false;
	break;

    case CM_QUIT:
	cout << "Connection to "<< conn->describe_peer() <<" quit."<<endl;
	return false;
    }
    return valid;
}

bool ServerMessageHandler::timer() {
    double ctime = gettime();
    bool timeout=false;

    if (ltime != -1)
	if (ctime - ltime > (suspend ? 40 : 10))
	    timeout = true;

    if (timeout) {
	cout << "Connection to "<< conn->describe_peer() <<" timed out ("<<(ctime - ltime)<<")."<<endl;
	return false;
    }
    return true;
}

void ServerMessageHandler::updateactors() {

    for (map<int, int>::iterator itr = last_timestamp.begin(); itr != (last_timestamp).end();) {
	sptr<Actor> act = svr->world->findactor(itr->first);
	if (act == NULL) {
	    sptr<Message> msg = GC::track(new Message(SM_ACTOR_DEL));
	    msg->type = SM_ACTOR_DEL;
	    msg->writevar(itr->first);
	    conn->sendmessage(msg, CHAN_RELIABLE);

	    map<int, int>::iterator old = itr;
	    itr++;
	    last_timestamp.erase(old);
	} else
	    itr++;
    }

    FOREACH(aitr, svr->world->actors) {
	sptr<ServerActor> a = aitr->statcast<ServerActor>();
	sptr<ServerCommunicator> comm = a->comm.statcast<ServerCommunicator>();
	if (comm == NULL) continue;

	int lts = last_timestamp[a->id];
	sptr<Message> msg = GC::track(new Message(0));

	int ret = comm->writeactor(msg, lts);
	bool sent;

	switch(ret) {
	case ERASE_ACTOR:
	    msg->type = SM_ACTOR_DEL;
	    msg->writevar(a->id);
	    conn->sendmessage(msg, CHAN_RELIABLE);
	    last_timestamp.erase(a->id);
	    break;
	case SEND_RELIABLE:
	case SEND_UNRELIABLE:
	    sent = conn->sendmessage(msg, ret == SEND_RELIABLE ? CHAN_RELIABLE 
				     : CHAN_UNRELIABLE);
	    if (!sent)
		break;
	    // fall through
	case DONT_SEND:
	    last_timestamp[a->id] = svr->current_timestamp;
	}

    }

    sptr<Message> smsg = GC::track(new Message(SM_ENDFRAME));
    conn->sendmessage(smsg, CHAN_UNRELIABLE_QUEUE);
	
    conn->sendpacket();
}

void ServerMessageHandler::disconnected() {
    if (player != NULL) {
	player->remove();
	player = NULL;
    }
}


/*
 *
 * ServerCommunicator
 *
 */

#include objdef(ServerCommunicator)

ServerCommunicator::ServerCommunicator(ServerActor* a, ServerThread* svr) 
    : Communicator(a), svr(svr) {

    stilltime = -1;
    orad = -1;
    deltacnt = (rand()&127)+24;
    sendstill = false;
    moving = false;
    master_ts = loc_ts = ang_ts = size_ts = svr->current_timestamp;
}


void ServerCommunicator::set_client_class(string clas) {
    clclass = clas;

    params.clear();

    clclass_ts = param_ts = 
	master_ts = svr->current_timestamp;
	
}
void ServerCommunicator::playsound(const char* snd, double vol) {
    sptr<Message> smsg = GC::track(new Message(SM_SOUND));
    smsg->writestring(snd);
    smsg->writeshort((short)(vol*16384));
    svr->extramsgs.push_back(smsg);
}
#if 0
void hexdump(Uint8* pkt, int len) {
    int j;
    for (j = 0; j < len; j++) {
        printf("%02x%s", pkt[j], (j&15) == 15 ? "\n": " ");
    }
    printf("\n");
}
#endif
void ServerCommunicator::playsound(const char* snd, vect3d pos, double vol) {
    sptr<Message> smsg = GC::track(new Message(SM_SOUND_POS));
    smsg->writestring(snd);
    smsg->writefloat(pos.x);
    smsg->writefloat(pos.y);
    smsg->writefloat(pos.z);
    smsg->writeshort((short)(vol*16384));
    svr->extramsgs.push_back(smsg);
}
void ServerCommunicator::tick(double time) {
    updatetimestamps();
    if (moving) {
	stilltime = 2.5;
    }
    sendstill = false;
    if (stilltime >= 0) {
	stilltime -= time;
	if (stilltime<0) {
	    sendstill = true;
	}
    }
}

int ServerCommunicator::writeactor(Message* msg) {
    return writeactor(msg, 0);
}

void ServerCommunicator::setparameter(int num, string sval) {
    Parameter& p(params[num]);
    p.type = PT_STRING;
    p.sval = sval;
    p.timestamp = param_ts = 
	master_ts = svr->current_timestamp;
}

void ServerCommunicator::setparameter(int num, ParamType type, int ival) {
    Parameter& p(params[num]);
    p.type = type;
    p.ival = ival;
    p.dval = (double)ival;
    p.timestamp = param_ts = 
	master_ts = svr->current_timestamp;
	
}

void ServerCommunicator::setparameter(int num, ParamType type, double dval) {
    Parameter& p(params[num]);
    p.type = type;
    p.ival = (int)dval;
    p.dval = dval;
    p.timestamp = param_ts = 
	master_ts = svr->current_timestamp;
}

void ServerCommunicator::updatetimestamps() {
    if (a->loc != oloc || a->area != oarea) {
	master_ts = loc_ts = svr->current_timestamp;
	oloc = a->loc;
	oarea = a->area;
    }

    if (a->ang != oang) {
	master_ts = ang_ts = svr->current_timestamp;
	oang = a->ang;
    }
	
    int crad = (int)ceil(a->rad*4.0);
    if (orad != crad) {
	master_ts = size_ts = svr->current_timestamp;
	orad = crad;
    }
}

bool ServerCommunicator::writeparameters(Message* msg, unsigned int last_timestamp) {
    bool wrote = false;
    if (param_ts <= last_timestamp)
	return false;

    FOREACH(itr, params) {
	int i = itr->first;
	Parameter& p(itr->second);
	if (p.timestamp > last_timestamp) {
	    wrote = true;
	    int type = p.type;
	    // Lerp only applies if we updated last frame
	    if (type == PT_FLOAT && last_timestamp < p.timestamp - 1)
		type = PT_FLOAT_NOLERP;
	    int stat = type<<5;
	    stat |= i;
	    msg->writebyte(stat);
	    switch (type) {
	    case PT_ID:
		msg->writevar(p.ival);
		break;
	    case PT_INT:	
		msg->writeint(p.ival);
		break;
	    case PT_FLOAT:
	    case PT_FLOAT_NOLERP:
		msg->writefloat(p.dval);
		break;
	    case PT_STRING:
		msg->writestring(p.sval);
		break;
	    }
	}
    }
    return wrote;
}

int ServerCommunicator::writeactor(Message* msg, unsigned int last_timestamp) {
    // If we don't draw the actor, why does the client
    // need to know about it?

    int uflags = 0;
    if (a->world == NULL)
	return ERASE_ACTOR;

    if (master_ts <= last_timestamp)
	return DONT_SEND;

    if (clclass.empty()) {
	if (last_timestamp != 0)
	    return ERASE_ACTOR;
	else
	    return DONT_SEND;
    }

    if (loc_ts > last_timestamp)
	uflags |= UF_LOC;

    if (ang_ts > last_timestamp)
	uflags |= UF_ANG;

    if (size_ts > last_timestamp)
	uflags |= UF_SIZE;

    if (clclass_ts > last_timestamp) 
	uflags |= UF_CLASS;

    if (last_timestamp < (loc_ts - 1) || 
	last_timestamp < (ang_ts - 1)) {	
	uflags |= UF_NOLERP;
    }
    int crad = (int)ceil(a->rad*4.0);

    bool rel = last_timestamp == 0 || (uflags&UF_CLASS) ? true : false;

    vect3d sloc = a->loc - a->area->floor->cpt;
    vect3d sloc2 = lastloc - a->area->floor->cpt;

    int nx = (int)(LOCMUL*sloc.x);
    int ny = (int)(LOCMUL*sloc.y);
    int nz = (int)(LOCMUL*sloc.z);

    if (sendstill || rel) {
	stilltime = -1;
	uflags |= UF_LOC;
	rel = true;
    } else {
	//if (ox == nx && oy == ny && oz == nz) uflags &= ~UF_LOC;
    }

    moving = (!rel) && ((uflags&UF_LOC)?true:false);

    if (a->is_static) {
	return DONT_SEND;
    } else {
	msg->type = 64|(uflags);
	msg->writevar(a->id);
    }
    if (!uflags) {
	if (writeparameters(msg, last_timestamp)) {
	    return rel ? SEND_RELIABLE : SEND_UNRELIABLE;
	}
	return DONT_SEND;
    }
	

    if (uflags&UF_LOC)
	msg->writevar(a->area->id);

    if (uflags&UF_CLASS)
	msg->writestring(clclass);

    if (uflags&UF_SIZE) {
	msg->writevar(crad);
    }
    if (uflags&UF_LOC) {
	msg->writecoord(nx);
	msg->writecoord(ny);    
	msg->writecoord(nz);    
    }

    if (uflags&UF_ANG)
	msg->writebits((int)(a->ang*ANG_TO_SHORT),12);

    msg->writealign();

    writeparameters(msg, last_timestamp);

    //cout << "len: "<<msg->size()<<endl;

    return rel ? SEND_RELIABLE : SEND_UNRELIABLE;

}


/*
 *
 * ServerConnectionManager
 *
 */

#include objdef(ServerConnectionManager)

ServerConnectionManager::ServerConnectionManager(ServerThread* svr) : svr(svr) { }
sptr<MessageHandler> ServerConnectionManager::create_handler() {
    return GC::track(new ServerMessageHandler(svr));
}
ServerConnectionManager::~ServerConnectionManager() { }

/*
 *
 * ServerThread
 *
 */

#include objdef(ServerThread)

ServerThread::ServerThread(int framems) : framems(framems) {
    frametime = framems / 1000.0;
    mgr = GC::track(new ServerConnectionManager(this));
}

void ServerThread::loadworld(string worldname) {
    Lock l1(this);

    current_timestamp = 1;

    cout << "Loading world... ";
    cout.flush();

    startpts.clear();
    GameActorLoader loader(startpts);

    string tworldname(join_path(FileResourceLoader::get_data_dir(), "worlds", worldname));
    double begin = gettime();
    try {
	world = GC::track(new World ((tworldname + ".world.gz").c_str(), loader));
    } catch (exception& te) {
	try {
	    world = GC::track(new World ((tworldname + ".world").c_str(), loader));
	} catch (exception& te) {
	    cout << "\nError loading world: " << worldname << ": " << te.what() << "\n";
	    world = NULL;
	    return;
	}
    }
    this->worldname = worldname;
    double end = gettime();
    cout << end - begin << " sec" << endl;
    

    { 
	Lock l(mgr);
	FOREACH(itr, mgr->connections) {
	    sptr<ServerMessageHandler> smh = (*itr)->gethandler().statcast<ServerMessageHandler>();
	    if (smh != NULL) {
		smh->newworld();
		(*itr)->sendpacket();
	    }
	}
    }


}

void ServerThread::listen(int port) {
    if (sg != NULL) {
	sg->stop();
	sg = NULL;
    }
    
    if (sock != NULL) {
	sock->close();
	sock = NULL;
    }

    sock = GC::track(new SMDPUDPSocket(mgr, port, true));
    sg = GC::track(new SocketGroup());
    sg->add(sock);
    sg->start();
}

void ServerThread::connect_loopback(SMDPLoopbackConnection* conn) {
    mgr->add(conn);
}

int ServerThread::run() {
    lock();
    vector< sptr<ServerMessageHandler> > handlers;
    double next = gettime();
    while (world != NULL) {
	double ctime;

	unlock();
	dsleep(0);
	while ((ctime=gettime())<next) {
	    dsleep(next - ctime);
	}
	lock();
	
	if (world == NULL) break;
	next += frametime; // * (1.0 - WAIT_FRAC);
	if (ctime > next) next = ctime;
	world->remove_stale_actors();
	world->moveactors(frametime);
	FOREACH(a, world->pendingactors) {
	    a->statcast<ServerActor>()->comm = 
		GC::track(new ServerCommunicator(a->statcast<ServerActor>(), this));
	}
	world->add_pending_actors();
	
	/*
	while ((ctime=gettime())<next) {
	    dsleep(next - ctime);
	}
	next += frametime * WAIT_FRAC;
	*/

	{ 
	    Lock l(mgr);
	    FOREACH(itr, mgr->connections) {
		handlers.push_back((*itr)->gethandler().statcast<ServerMessageHandler>());
	    }
	}
	FOREACH(itr, handlers) {
	    try {
		(*itr)->getconn()->handle_queue();
	    } catch (exception& ex) {
		string desc = "<unknown>";	       
		cout << (*itr)->getconn()->describe_peer() << " caught unexpected exception: " << ex.what() << endl;
	    }
	}
	
	
	FOREACH(itr,world->actors) {
	    sptr<ServerActor> a = itr->statcast<ServerActor>();
	    sptr<ServerCommunicator> comm = a->comm.statcast<ServerCommunicator>();
	    if (comm == NULL) continue;
	    
	    comm->tick(frametime);
	    
	    if (a->thinkinterval) {
		if (!a->nextthink) a->nextthink=ctime;
		while (a->nextthink<=ctime) {
		    a->think();
		    a->nextthink += a->thinkinterval;
		}
	    } else a->nextthink=0;
	}
	
	
	FOREACH(itr, handlers) {
	    sptr<ServerMessageHandler> ccl = *itr;
	    
	    if (ccl->getconn() == NULL) continue;
	    
	    if (!ccl->timer()) {
		ccl->getconn()->disconnect();
		continue;
	    }

	    if (ccl->suspend) {
		ccl->getconn()->sendpacket();
		continue;
	    }

	    
	    FOREACH(msgi, extramsgs) {
		ccl->getconn()->sendmessage(*msgi, CHAN_UNRELIABLE_QUEUE);
	    }
	    
	    ccl->updateactors();
	}
	
	handlers.clear();
	extramsgs.clear();
	
	current_timestamp++;
	
	// Create and destroy an Object to bump the garbage
	// collector's counter.
	sptr<Object> tmpobject = GC::track(new Object());
	
    }
    unlock();
    return 0;
}

void ServerThread::quit() {
    Lock l(this);
    if (sg != NULL) {
	sg->stop();
	sg = NULL;
    }
    
    world = NULL;
    if (mgr != NULL) {
	{ 
	    Lock l(mgr);
	    
	    sptr<Message> smsg = GC::track(new Message(SM_QUIT));
	    FOREACH(itr, mgr->connections) {
		sptr<SMDPConnection> ccl = *itr;
	    ccl->sendmessage(smsg, CHAN_UNRELIABLE_QUEUE);
	    ccl->sendpacket();
	    ccl->disconnect();
	    }
	    FOREACH(itr, mgr->connections) {
		//sptr<ServerMessageHandler> ccl = itr->statcast<ServerMessageHandler>();
	    }
	}
	mgr = NULL;
    }

    if (sock != NULL) {
	sock->close();
	sock = NULL;
    }

    GC::collect();
}

void ServerThread::stop() {
    quit();
    Thread::stop();
}

