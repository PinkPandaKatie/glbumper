

#include "common.h"
#include "smdp.h"

/*
 *
 *  MessageHandler
 *
 */

#include objdef(Message)

#include objdef(MessageHandler)

void MessageHandler::dropped(int num) {
    cout<<"dropped: "<<num<<endl;
}


void MessageHandler::disconnected() {
    conn = NULL;
}

sptr<SMDPConnection> MessageHandler::getconn() {
    return conn;
}
/*
 *
 *  SMDPConnectionManager
 *
 */

#include objdef(SMDPConnectionManager)

void SMDPConnectionManager::add(SMDPConnection* c) {
    Lock l(this);
    if (c->handler == NULL) {
	c->handler = create_handler();
	c->handler->conn = c;
	c->mgr = this;
    }
    connections.insert(c);
}

void SMDPConnectionManager::remove(SMDPConnection* c) {
    Lock l(this);
    set< sptr<SMDPConnection> >::iterator itr = connections.find(c);
    if (itr != connections.end()) {
	connections.erase(itr);
    }
}


/*
 *
 *  SMDPConnection
 *
 */

#include objdef(SMDPConnection)

void SMDPConnection::putmessage(Message* msg) {
    Lock l(this);
    if (handler == NULL)
	return;
    receive_queue.push_back(msg);
}

void SMDPConnection::disconnect() {
    if (mgr != NULL) {
	mgr->remove(this);
	mgr = NULL;
    }
    if (handler != NULL) {
	handler->disconnected();
	handler = NULL;
    }
}
void SMDPConnection::handle_queue() {

    vector< sptr<Message> > recvq;

    swapqueue(recvq);

    FOREACH(msg, recvq) {
	if (handler == NULL || !handler->handle(*msg)) {
	    disconnect();
	    break;
	}
    }
}

/*
 *
 *  SMDPUDPConnection
 *
 */

#include objdef(SMDPUDPConnection)

SMDPUDPConnection::SMDPUDPConnection(SMDPUDPSocket* sock, Address peer) 
    : sock(sock), peer(peer), 
        queue_size(16), sendseq(0), recvseq(0), sendrelseq(0), recvrelseq(0), dropcount(0), isnew(true), holdoff(false)
{

}

SMDPUDPConnection::~SMDPUDPConnection() {
}

void SMDPUDPConnection::disconnect() {
    if (sock != NULL) {
	map<Address, sptr<SMDPUDPConnection> >::iterator itr = sock->connections.find(peer);
	if (itr != sock->connections.end()) sock->connections.erase(itr);
	sock = NULL;
    }
    
    SMDPConnection::disconnect();
}

void SMDPUDPConnection::handle_queue() {
    holdoff = true;
    
    SMDPConnection::handle_queue();

    holdoff = false;

    sendpacket();
}

bool SMDPUDPConnection::gotpacket(Uint8* data, int len) {
    sptr<MessageHandler> handler = this->handler;
    if (handler == NULL) return false;
    
    double ctime = gettime();

    if (len<16) {
	if (isnew) return false;
	return true;
    }
    holdoff = true;

    sptr<Message> pkt = GC::track(new Message(0, string((char*)data, len)));

    Uint32 seq = pkt->readint();
    Uint32 relseq = pkt->readint();
    Uint32 relack = pkt->readint();
    int numrel = pkt->readshort();
    int numunrel = pkt->readshort();


    if (seq < recvseq) return true;
    if (seq != recvseq) {
	int num = seq - recvseq;
	dropcount += num;
	handler->dropped(num);
    }

    recvseq = seq + 1;
    // numrel == 65535 has a special meaning: it means there are no reliable
    // messages, but we should send an acknowledgement anyway.
    if (numrel != 0 && numrel != 65535) {
	need_ack = true;
	Uint32 numdups = recvrelseq - relseq;
	recvrelseq = relseq + numrel;
	for (int i = 0; i < numrel; i++) {
	    sptr<Message> msg = pkt->readmessage(); 
	    msg->time = ctime;
	    if ((unsigned)i >= numdups)
		putmessage(msg);
	}
    }
    for (int i = 0; i < numunrel; i++) {
	sptr<Message> msg = pkt->readmessage(); 
	msg->time = ctime;
	putmessage(msg);
    }
	
    // okay, process the acks
    if (relack > sendrelseq) {
	Lock l(this);
	Uint32 numrecv = relack - sendrelseq;

	sendrelseq = relack;

	while (numrecv-- && !reliable_messages.empty()) {
	    queue_size -= reliable_messages.front()->size();
	    reliable_messages.pop_front();
	}
    }	
    if (((numrel == 65535 || numrel == 0) && numunrel == 0) && isnew) return false;

    isnew = false; 
    holdoff = false;

    return true;

}
void SMDPUDPConnection::sendpacket() {

    if (!need_ack && reliable_messages.empty() && unreliable_messages.empty())
	return;

    if (sock == NULL)
	return;
	
    if (holdoff) return;

    sptr<Message> pkt = GC::track(new Message(0));
    pkt->writeint(sendseq);
    pkt->writeint(sendrelseq);
    pkt->writeint(recvrelseq);
    pkt->writeshort(0);
    pkt->writeshort(0);

    int numrel = 0;
    int numunrel = 0;
    int trunc = 0;
    {
	Lock l(this);
	FOREACH(itr, reliable_messages) {
	    sptr<Message> msg = *itr;
	    if (msg == NULL) {
		cout <<"FUCK ME! IN THE ASSHOLE!"<<endl;
	    }
	    if (msg->size() + pkt->tell() >= (MAX_DGRAM-4)) {
		trunc = 1;
		break;
	    }
	    numrel++;
	    pkt->writemessage(msg);
	}
    }

    while (!trunc && !unreliable_messages.empty()) {
	sptr<Message> msg = unreliable_messages.front();
	if (msg->size() + pkt->tell() >= (MAX_DGRAM-4)) {
	    trunc = 1;
	    break;
	}

	pkt->writemessage(msg);
	numunrel++;

	queue_size -= msg->size();
	unreliable_messages.pop_front();
    }

    sendseq++;

    need_ack = false;

    pkt->seek(12);
    pkt->writeshort((numrel == 0 && trunc) ? 65535 : numrel);
    pkt->writeshort(numunrel);

    if (trunc) holdoff=true;

    sock->send((const Uint8*)pkt->data.data(), pkt->data.size(), peer);

}

bool SMDPUDPConnection::sendmessage(Message* msg, MessageChannel chan) {

    isnew = false;

    switch (chan) {
    case CHAN_UNRELIABLE:
	if (msg->size() + queue_size >= (MAX_DGRAM - 4))
	    return false;
	// fall through
    case CHAN_UNRELIABLE_QUEUE:
	queue_size += msg->size();
	unreliable_messages.push_back(msg);
	return true;

    case CHAN_RELIABLE:
	queue_size += msg->size();
	reliable_messages.push_back(msg);
	return true;
    }

    return false;
}



/*
 *
 *  SMDPUDPSocket
 *
 */

#include objdef(SMDPUDPSocket)


sptr<SMDPConnection> SMDPUDPSocket::connect(const Address& to) {
    sptr<SMDPUDPConnection> conn = GC::track(new SMDPUDPConnection(this, to));
    mgr->add(conn);
    connections[to] = conn;
    
    return conn;

}

void SMDPUDPSocket::readready(SocketGroup* sg) {
    Address addr;
    Uint8 data[MAX_DGRAM];

    int len = recv(data, MAX_DGRAM, addr);

    sptr<SMDPUDPConnection> conn;
    bool newconn=false;
    map<Address, sptr<SMDPUDPConnection> >::iterator itr = connections.find(addr);
    
    if (itr == connections.end()) {
	if (!listen) return;

	conn = GC::track(new SMDPUDPConnection(this, addr));
	mgr->add(conn);

	newconn=true;

    } else {
	conn = itr->second;
    }
    
    if (conn->gotpacket(data,len)) {
	if (newconn) {
	    connections[addr] = conn;
	}
    } else {
	conn->disconnect();
    }
}


/*
 *
 *  SMDPLoopbackConnection
 *
 */

#include objdef(SMDPLoopbackConnection)

sptr<SMDPLoopbackConnection> SMDPLoopbackConnection::getpeer() {
    if (peer == NULL) {
	peer = GC::track(new SMDPLoopbackConnection());
	peer->peer = this;
    }
    return peer;
}
void SMDPLoopbackConnection::disconnect() {
	sptr<SMDPLoopbackConnection> tpeer = peer;
	if (tpeer != NULL) {
	    peer = NULL;
	    tpeer->disconnect();
	    SMDPConnection::disconnect();
	}
    }

bool SMDPLoopbackConnection::sendmessage(Message* msg, MessageChannel chan) {
    if (peer != NULL) {
	msg->time = gettime();
	msg->seek(0);
	peer->putmessage(msg);
    }
    return true;
}
