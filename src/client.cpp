
#include "common.h"
#include "object.h"
#include "client.h"
#include "profile.h"

#define PF_CHANGED 1

class NullActorLoader : public ActorLoader {
public:
    sptr<Actor> load(string& clas, vect3d pt, Area* area, double ang, DataInput& ts) {
	return NULL;
    }

};

/*
 *
 * ClientMessageHandler
 *
 */

#include objdef(ClientMessageHandler)

ClientMessageHandler::ClientMessageHandler(ClientComponent* ccomp) : ccomp(ccomp), dropped_packet(false) { }

bool ClientMessageHandler::handle(Message* msg) {
    return ccomp->handle_message(msg);
}

void ClientMessageHandler::disconnected() {

}

/*
 *
 * ClientComponent
 *
 */

#include objdef(ClientComponent)


ClientComponent::ClientComponent() {
    cstate = CONN_DISCONNECTED;

    interp = true;

    showinfo = 0;
    suspended = true;

    lerpcount = 0;

    lsendtime = 0;
    framems = 0;
    frametime = 0;
    eyeactorid = -1;
    neweyeactorid = -1;
    
    dropwarn = false;

    ovf = 0; ovs = 0; ovz = 0; ovr = 0;

    vf = 0; vf_h = 0; vf_l = 0;
    vz = 0; vz_h = 0; vz_l = 0;
    vr = 0; vr_h = 0; vr_l = 0;
    vs = 0; vs_h = 0; vs_l = 0;

    vvd = 0; vvd_h = 0; vvd_l = 0;
    vvh = 0; vvh_h = 0; vvh_l = 0;
    vd2 = 0; vd2_h = 0; vd2_l = 0;

    viewarea = NULL;

    vdist = 0.0;
    vheight = 0.0;

    world = NULL;

#ifdef ENABLE_PROFILE
    profile = false;
#endif

    info = GC::track(new Label(color::Clear));
    add(info, 0, 0);

    mgr = GC::track(new ClientConnectionManager(this));
}

ClientComponent::~ClientComponent() {
    disconnect();
}

void ClientComponent::connect(string to) {
    disconnect();
    Address addr(to);
    
    sock = GC::track(new SMDPUDPSocket(mgr));
    sg = GC::track(new SocketGroup());
    sg->add(sock);

    conn = sock->connect(addr);
    
    sptr<Message> msg = GC::track(new Message(CM_HELLO));
    conn->sendmessage(msg, CHAN_RELIABLE);
    conn->sendpacket();

    cmh = conn->gethandler().statcast<ClientMessageHandler>();

    cstate = CONN_SENT_HELLO;

    sg->start();
}

void ClientComponent::connect_loopback(SMDPLoopbackConnection* loop) {
    disconnect();

    conn = loop;

    mgr->add(conn);
    
    cmh = conn->gethandler().statcast<ClientMessageHandler>();

    sptr<Message> msg = GC::track(new Message(CM_HELLO));
    conn->sendmessage(msg, CHAN_RELIABLE);
    conn->sendpacket();

    cstate = CONN_SENT_HELLO;

}

void ClientComponent::disconnect() {
    world = NULL;
    if (mgr == NULL) return;
    Lock l(mgr);
    if (conn != NULL) {
	Lock l(conn);
	conn->sendmessage(GC::track(new Message(CM_QUIT)));
	conn->sendpacket();
	conn->disconnect();

    }
    if (sg != NULL) {
	sg->stop();
	sg = NULL;
    }
    if (sock != NULL) {
	sock->close();
	sock = NULL;
    }
    cstate = CONN_DISCONNECTED;
}

void ClientComponent::quit() {
    disconnect();
    if (parent != NULL)
	parent->remove(this);
}

void ClientComponent::sendspecial(int cmd) {
    sptr<Message> msg = GC::track(new Message(CM_SPECIAL));
    msg->writeshort(cmd);
    conn->sendmessage(msg, CHAN_RELIABLE);
    conn->sendpacket();
    lsendtime = gettime();
}

void ClientComponent::suspend() {
    if (suspended) return;
    conn->sendmessage(GC::track(new Message(CM_SUSPEND)), CHAN_RELIABLE);
    conn->sendpacket();
    lsendtime = gettime();
    suspended = true;
}

void ClientComponent::resume() {
    if (!suspended) return;
    conn->sendmessage(GC::track(new Message(CM_RESUME)), CHAN_RELIABLE);
    conn->sendpacket();
    lsendtime = gettime();
    suspended = false;
    for (int i = 0; i < SMOOTHCNT; i++) {
	smoothtimes[i] = frametime;
    }
    last_endframe_time = 0;
}

bool ClientComponent::event(SDL_Event& evt) {
    sptr<Message> msg;
    switch (evt.type) {
    case SDL_KEYDOWN:
	switch (evt.key.keysym.sym) {
	case SDLK_RETURN:
	    if (evt.key.keysym.mod & KMOD_ALT) {
		suspend();
		setmode(screenw, screenh, !screen_fs);
		resume();
	    }
	    return true;
	case SDLK_x:
	    suspend();
	    return true;
	case SDLK_c:
	    resume();
	    return true;
	case SDLK_t:
	    interp ^= 1;
	    break;
	case SDLK_b:
	case SDLK_g:
	case SDLK_m:
	case SDLK_k:
	case SDLK_l:
	case SDLK_SEMICOLON:
	case SDLK_COMMA:
	case SDLK_SPACE:
	    sendspecial(evt.key.keysym.sym);
	    break;
	case SDLK_TAB:
	    showinfo++;
	    if (showinfo == 3) showinfo = 0;
	    return true;

#ifdef ENABLE_PROFILE
	case SDLK_p:
	    profile^=1;
	    printf("\033[H\033[2J");
	    return true;
#endif
	case SDLK_ESCAPE:
	    quit();
	    return true;

	default: // get rid of "enumeration value not handled" warning
	    break;
	}
	// fallthrough
    case SDL_KEYUP:
	switch (evt.key.keysym.sym) {
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

	    VEL (LEFTBRACKET, RIGHTBRACKET, vd2);
	    VEL (d, e, vz);
	    VEL (f, s, vs);
	    VEL2 (RIGHT, KP6, LEFT, KP4, vr);
	    VEL2 (DOWN, KP2, UP, KP8, vf);
	    VEL (y, h, vvd);
	    VEL (j, u, vvh);

	default:
	    return true;
	}
	return false;
    case SDL_QUIT:
	quit();
	return true;
    }
    return false;
}

bool ClientComponent::handle_message(Message* msg) {
    string wname, wfilename;
    int aid = 0;
    int magic;
    sptr<Message> smsg;
    PROBEGINF;

    if (msg == NULL) {
	cerr << "YOW! KELLY CLARKSON!!!" << endl;
	PROENDF;
	return true;
    }
    if (msg->type&64 || msg->type==SM_ACTOR_DEL) {
	sptr<ActorProxy> ap;
	bool newact=false;
	PROBEGIN(update);
		
	if (cstate != CONN_CONNECTED) {
	    cerr << "server sent actor in invalid state!";
	    PROENDF;
	    return true;
	}

	if (world == NULL) {
	    cerr << "server sent actor without a world!\n";
	    PROENDF;
	    return true;
	}

	aid=msg->readvar();
	    
	ap = world->findactor(aid).cast<ActorProxy>();
	if (ap == NULL) {
	    if (msg->type != SM_ACTOR_DEL) {
		ap = new ActorProxy(aid);
		ap->world = world;
		newact = true;
	    }
	} else {
	    if (msg->type == SM_ACTOR_DEL) ap->remove();
	}

	if (msg->type != SM_ACTOR_DEL) {
	    int uflags;
		    
	    uflags = msg->type&63;
		    
	    ap->readfrom(msg,uflags);
		    
	    if (newact) {
		if (ap->world != NULL) {
		    if (ap->flagsset&(UF_LOC|UF_CLASS) != (UF_LOC|UF_CLASS)) {
			cerr << "server sent new actor "<<aid<<" without setting information!"<<endl;
			cerr << "server set flags "<<ap->flagsset<<endl;
		    } else {
			// If the server didn't send an area for the new actor,
			// ap->area will be NULL. This is BAD. In this case, it's
			// better to pretend the actor does not exist. If we're 
			// connected to a malicious or malfunctioning server,
			// we're not going to be getting a very good gameplay
			// experience anyway.
				
			ap->init();
			if (ap->area != NULL) {
			    world->add(ap);
			} else {
			    cout << aid << " has no area!!!!"<<endl;
			}
		    }
		}
	    }
	}
	PROEND(update);
    } else {

	switch (msg->type) {
	case SM_MAGIC:
	    if (cstate == CONN_SENT_HELLO) {
		magic = msg->readint();
		smsg = GC::track(new Message(CM_JOIN));
		smsg->writeint(magic);
		conn->sendmessage(smsg, CHAN_RELIABLE);
		conn->sendpacket();
		cstate = CONN_SENT_JOIN;
	    } else {
		cerr << "server sent magic again!"<<endl;
	    }
	    break;
	case SM_INIT:
	    if (cstate == CONN_SENT_JOIN) {
		framems = msg->readint();
		frametime =  framems / 1000.0;
		cstate = CONN_CONNECTED;

		cout << "server frame time is "<<framems<<endl;
	    } else {
		cerr << "server sent init out of order!"<<endl;
	    }
	    break;
	case SM_SOUND: 
	    if (cstate == CONN_CONNECTED) {
		string nm = msg->readstring();
		double vol = msg->readshort()/16384.0;
		sptr<Sound> s = Resource::get<Sound>(nm.c_str());
		if (s != NULL) {
		    s->play(vol);
		}
	    } else {
		cerr << "server sent sound in invalid state"<<endl;
	    }
	    break;

	case SM_SOUND_POS:
	    if (cstate == CONN_CONNECTED) {
		string nm = msg->readstring();
		double tx = msg->readfloat();
		double ty = msg->readfloat();
		double tz = msg->readfloat();
		double vol = msg->readshort()/16384.0;
		sptr<Sound> s = Resource::get<Sound>(nm.c_str());
		if (s != NULL) {
		    s->play(vect3d(tx,ty,tz),vol);
		}
	    } else {
		cerr << "server sent sound in invalid state"<<endl;
	    }
	    break;
	case SM_WORLD:
	    if (cstate == CONN_CONNECTED) {
		
		suspended = true;

		eyeactorid=-1;
		world=NULL;
	    
		wname = msg->readstring();

		const char * cwname = wname.c_str();
		bool ok=true;
		while (*cwname) {
		    if (!(isalnum(*cwname) || *cwname == '-' || 
			  *cwname == '.' || *cwname == '_')) {
			cout << "Something is fishy... server sent world "
			     << "name with suspicious characters"<<endl;
			cout << "World name is \""<<wname<<"\""<<endl;
			ok=false;
			break;
		    }
		    cwname++;
		}
		NullActorLoader loader;
		DecorationLoader dloader(loader);
		if (ok) {
		    wfilename = join_path(FileResourceLoader::get_data_dir(), "worlds", wname);
		    cout << wfilename<<endl;
		    try {
			world = GC::track(new World ((wfilename+".world.gz").c_str(), dloader));
		    } catch (exception& ex) {
			try {
			    world = GC::track(new World ((wfilename+".world").c_str(), dloader));
			} catch (exception& te) {
			    cout << "\nError loading world: " << wname << ":" << te.what() << "\n";
			    quit();
			    return true;
			}
		    }
		}
		GC::collect();
	    } else {
		cerr << "server sent world in invalid state"<<endl;
	    }
		
	    break;
	case SM_SETEYE:
	    resume();
	    neweyeactorid = msg->readshort();
	    break;

	case SM_QUIT:
	    cout << "server sent SM_QUIT"<<endl;
	    quit();
	    return true;

	case SM_ENDFRAME:
	    if (cstate == CONN_CONNECTED) {
		eyeactorid = neweyeactorid;
		if (world != NULL) {

		    dropwarn = false;
		    if (!got_endframe) {
			got_endframe = true;

			last_endframe_time = msg->time;

			FOREACH(itr,world->actors) {
			    sptr<ActorProxy> a = itr->cast<ActorProxy>(); 
			    a->commit = a->write;

			    // Reset lerp flags for next frame
			    a->write.lerp = true;
			    for (int cp = 0; cp < NUMPARAMS; cp++)
				a->write.params[cp].flags = 0;
			}
		    }

		    //cout << msg->time << ' ' << interp_begin << ' ' << interp_end<<endl;
#ifdef DEBUG_VEL
		    cerr << "--------"<<endl;
#endif
		    lerpcount=0;
		} else {
		    cerr << "server sent SM_ENDFRAME without a world!"<<endl;
		}
	    } else {
		cerr << "server sent SM_ENDFRAME in invalid state "<<cstate<<endl;
	    }
	    break;
	default:
	    cout << "??????????????????? "<<msg->type<<endl;
	}
    }
    PROENDF;
    return true;
}

void ClientComponent::tick(double deltatime) {
    sptr<Message> smsg;
    double ctime = gettime();
    {
	Lock l(mgr);

	if (conn != NULL && (vf != ovf || vs != ovs || vz != ovz || vr != ovr)) {
	    ovf = vf;
	    ovs = vs;
	    ovr = vr;
	    ovz = vz;
	    
	    int cmd = (vf+1) | (vs+1)<<2 | (vz+1)<<4 | (vr+1)<<6;
	    
	    smsg = GC::track(new Message(CM_INPUT));
	    smsg->writeshort(cmd);
	    conn->sendmessage(smsg, CHAN_RELIABLE);
	    conn->sendpacket();
	    lsendtime = ctime;
	}
    }
    vdist += vvd * 3 * deltatime;
    vheight += vvh * 3 * deltatime;

    lerpcount++;
    
    vector< sptr<Message> > recvqueue;
    {    
	Lock l(mgr);
	if(conn != NULL) {
	    if ((ctime - lsendtime) > 1.4) {
		conn->sendmessage(GC::track(new Message(MSG_KEEPALIVE)));
		conn->sendpacket();
		lsendtime=ctime;
	    }
	    PROBEGIN(handle_queue);
	    conn->handle_queue();
	    PROEND(handle_queue);
	    
	}
    }

    if (cstate == CONN_CONNECTED && world != NULL) {

	if (ctime >= interp_end) {
	    PROBEGIN(newframe);
	    if (got_endframe) {

		got_endframe = false;
		FOREACH(itr,world->actors) {
		    sptr<ActorProxy> a = itr->cast<ActorProxy>(); 
		    a->readold = interp ? a->readnew : a->commit;
		    a->readnew = a->commit;

		    // Reset lerp flags for next frame
		    a->write.lerp = true;	
		    /*for (int cp = 0; cp < NUMPARAMS; cp++)
		      a->write.params[cp].flags = 0;*/
			
		    // If packets were dropped, don't interpolate.
		    if (cmh->dropped_packet) {
			a->readold = a->readnew;
		    }

		    a->loc = a->readold.loc;
		    a->area = a->readold.area;
		    a->ang = a->readold.ang;
		    a->rad = a->readold.rad;
		    if (a->appearance != NULL) {
			for (int i = 0; i < NUMPARAMS; i++) {
			    Parameter& p(a->readold.params[i]);
			    if (p.type != PT_UNUSED && p.type != PT_FLOAT) {
				a->appearance->params[i] = p;
				if (p.flags & PF_CHANGED) 
				    a->appearance->paramchanged(i);
			    }
			}
		    }
		}
		world->remove_stale_actors();
		world->add_pending_actors();

		cmh->dropped_packet = false;
		interp_begin = interp_end;
		interp_end = interp_begin + (frametime);
		if (interp_begin > interp_end-0.001) interp_begin = interp_end-0.001;
	    } else {
		if (!dropwarn)
		    cout << "\007!"<<endl;
		dropwarn = true;
		interp_end = ctime;
	    }
	    PROEND(newframe);
	}

	double frac = (ctime - interp_begin) / (interp_end - interp_begin);
	if (frac < 0) frac = 0;

	FOREACH(itr,world->actors) {
	    sptr<ActorProxy> ap = itr->cast<ActorProxy>();
	    ap->update(frac);
	    
#ifdef DEBUG_VEL
	    if (ap->id == 7) {
		double xv = (ap->loc.x - ap->lastx) / deltatime;
		double xa = (xv - ap->lastxv) / deltatime;
		double xda = (xa - ap->lastxa) / deltatime;
		double xta = (xda - ap->lastxda) / deltatime;
		cout << ap->loc.x << ' ' << xv << ' ' << xa<< ' ' << xda << ' ' << xta << endl;
		ap->lastx = ap->loc.x;
		ap->lastxv = xv;
		ap->lastxa = xa;
		ap->lastxda = xda;
		

	    }
#endif
	}
	
    }

    TextLayout& tl(info->layout());
    tl.clear();
    tl.shadow(color::Black).font(Resource::get<Font>("default16"));

    switch(cstate) {
    case CONN_DISCONNECTED:
	tl << color::Red << "disconnected" << endl;
	break;
    case CONN_SENT_HELLO:
	tl << color::Yellow << "Connecting ..." << endl;
	break;
    case CONN_SENT_JOIN:
	tl << color::Green << "Connected!" << endl;
	break;
    case CONN_CONNECTED:
	sptr<ActorProxy> player;
	if (world != NULL)
	    player = world->findactor(eyeactorid).cast<ActorProxy>();

	if (player == NULL) {
	    tl << color::Green << "Connected! Receiving initial data" << endl;
	} else {
	    if (showinfo > 0) {
		tl << player->loc.x << ' ' << htab(100) 
		   << player->loc.y << ' ' << htab(200)
		   << player->loc.z << htab(300) << '\n';
		FOREACH (itr,player->areas) {
		    tl << (*itr)->name << ' ';
		}
		tl << '\n';
	    }
	}
	if (showinfo > 0) {
	    tl << color::LtBlue << "Real FPS: " 
	       << color::White << (1.0 / deltatime) << '\n';
	}
	if (showinfo > 1) {
	    GC::printstats(tl.stream());
	}
    }
    
#ifdef ENABLE_PROFILE
    if (profile) {
	profile_show();
    }
#endif

}

void ClientComponent::draw(int x, int y) {

    if (cstate != CONN_CONNECTED || world == NULL) return;

    sptr<ActorProxy> ceyeactor = world->findactor(eyeactorid).cast<ActorProxy>();
    
    if (ceyeactor == NULL) return;
	
    FOREACH(itr,world->actors) {
	if ((*itr)->appearance != NULL)
	    (*itr)->appearance->runsound();
    }
    Sound::update_pos(ceyeactor->loc,ceyeactor->ang);

    viewpt = ceyeactor->loc;
    viewarea = ceyeactor->area;
    viewpt.z += 13.0 / 128.0;
    double s = sin (ceyeactor->ang);
    double c = cos (ceyeactor->ang);
    
    double vofs = 0;
    
    if (vdist>0) {
	vect3d ovpt(viewpt);
	vect3d visofs(-c*vdist,-s*vdist,vheight);
	
	vect3d tv2(viewpt+visofs);
	
	viewpt = world->trace(viewpt, ceyeactor->area, tv2, noactors);
	viewpt -=  0.5*vnormal(visofs);
	if ((viewpt-ovpt)*visofs<0) viewpt=ovpt;
	
	viewarea=world->findarea(ceyeactor->loc, ceyeactor->area,viewpt);
	vofs=-vheight/vdist;
    }
    vect3d tviewpt(viewpt);
    sptr<Area> tviewarea(viewarea);
    double tvofs(vofs);
    double tvang(ceyeactor->ang);

    sptr<Actor> eyeactor = ceyeactor;
    sptr<Actor> rveyeactor = ceyeactor;

    if (vdist>0)
	eyeactor = NULL;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDepthMask(1);

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


    ExcludeOneFilter drawfilter(eyeactor);

    if (vdist < 0) vdist = 0;
    
    PROBEGIN(draw_view);
    world->drawview (tviewpt, tviewarea, tvang, tvofs, 1.0, 0.75, drawfilter);
    PROEND(draw_view);
    
    glPopAttrib();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

/*
 *
 * ClientConnectionManager
 *
 */

#include objdef(ClientConnectionManager)

ClientConnectionManager::ClientConnectionManager(ClientComponent* ccomp) : ccomp(ccomp) { }

sptr<MessageHandler> ClientConnectionManager::create_handler() { 
    return GC::track(new ClientMessageHandler(ccomp));
}

/*
 *
 * ActorProxy
 *
 */

#include objdef(ActorProxy)

void ActorProxy::readfrom(Message* s, int flags) {
    if (world == NULL) return;

    flagsset |= flags;

    if (flags&UF_LOC) {
	int narea = s->readvar();
	    
	if (narea < 0 || (unsigned)narea >= world->areas.size()) {
	    remove();
	    cerr << "server sent bad area "<<narea<<" for actor "<<id<<endl;
	    return;
	}
	write.area = world->areas[narea];
    }


    if (flags&UF_CLASS) {
	cclass=s->readstring();
	set_appearance_class(this, cclass);
    }

    if (flags&UF_SIZE) {
	write.rad = s->readvar() * (1.0/4.0);
    }

    if (flags&UF_LOC) {
	int tx = s->readcoord();
	int ty = s->readcoord();
	int tz = s->readcoord();
	//cout << id << ' ' << tx << ' ' << ty << ' ' << tz << endl;
	write.loc.x = tx*(1.0/LOCMUL);
	write.loc.y = ty*(1.0/LOCMUL);
	write.loc.z = tz*(1.0/LOCMUL);
	write.loc += write.area->floor->cpt;
    } 

    if (flags&UF_ANG) {
	write.ang = s->readbits(12)*SHORT_TO_ANG;
    }

    if (flags&UF_NOLERP) {
	write.lerp = false;
    }
	
    s->readalign();
    while (!s->iseof()) {
	int stat = s->readbyte();
	ParamType type = (ParamType)(stat>>5);
	int num = stat&31;
	Parameter& p(write.params[num]);
	p.type = type;
	p.flags |= PF_CHANGED;
	switch (type) {
	case PT_STRING:
	    p.sval = s->readstring();
	    break;
	case PT_INT:
	    p.ival = s->readint();
	    p.dval = (double)p.ival;
	    break;
	case PT_ID:
	    p.ival = s->readvar();
	    p.dval = (double)p.ival;
	    break;
	case PT_FLOAT:
	case PT_FLOAT_NOLERP:
	    p.dval = s->readfloat();
	    p.ival = (int)p.dval;
            break;
        default:
            break;
	}
    }
}

void ActorProxy::init() {
    readold = readnew = write;

    loc = readnew.loc;
    area = readnew.area;
    ang = readnew.ang;
    rad = readnew.rad;

    checkareas();

    for (int i = 0; i < NUMPARAMS; i++) {
	Parameter& newpr(readold.params[i]);
	newpr.flags = 0;
	if (newpr.type != PT_UNUSED && appearance != NULL) {
	    appearance->params[i] = newpr;
	    appearance->paramchanged(i);
	}
    }
}

void ActorProxy::update(double frac) {
    if (world == NULL) return;

    double tfrac = readnew.lerp ? frac : 1.0;

    readnew.ang = normalize_angle(readnew.ang, readold.ang);
    //rad = (readnew.rad - readold.rad) * tfrac + readold.rad;
    rad = readnew.rad;
    ang = (readnew.ang - readold.ang) * tfrac + readold.ang;
    vect3d oldloc(loc);
    loc = readold.loc + (readnew.loc - readold.loc) * tfrac;
    area = readnew.lerp ? world->findarea(oldloc, area, loc) : readnew.area;

    checkareas();
    if (appearance != NULL) {
	for (int i = 0; i < NUMPARAMS; i++) {
	    Parameter& oldpr(readold.params[i]);
	    Parameter& newpr(readnew.params[i]);
	    if (oldpr.type == PT_UNUSED) continue;
	    Parameter& pr(appearance->params[i]);
	    if ((oldpr.type == PT_FLOAT || oldpr.type == PT_FLOAT_NOLERP) &&
		(newpr.type == PT_FLOAT)) {
		double cdval = pr.dval;
		double ndval = oldpr.dval + (newpr.dval - oldpr.dval) * frac;
		if (cdval != ndval) {
		    pr.dval = ndval;
		    appearance->paramchanged(i);
		}
	    }
	}
    }
}
