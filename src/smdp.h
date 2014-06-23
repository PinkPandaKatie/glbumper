// -*- c++ -*-

#ifndef _SMDP_H
#define _SMDP_H

#include "common.h"
#include "object.h"
#include "network.h"
#include "profile.h"
#include <deque>

#define LOCMUL 512.0
#define ANG_TO_SHORT (4096.0/(M_PI*2))
#define SHORT_TO_ANG ((M_PI*2)/4096.0)
#define MAX_DGRAM 640
#define FRAMEMS 60
#define FRAMETIME (FRAMEMS/1000.0)

enum MessageChannel {
    CHAN_UNRELIABLE,
    CHAN_UNRELIABLE_QUEUE,
    CHAN_RELIABLE,
};

// 00 - 10
// 01 - 13
// 10 - 16
// 110 - 19
// 11100 - 22
// 11101 - 25
// 11110 - 28
// 11111 - 32

// How to read the table:
// JUMP(x) means jump to rule x
// if 1, go to next rule if 0

#define JUMP(x) 1024|x
static const int lenhufft[] = 
    {
	JUMP(4),  // 0  - ?
	JUMP(3),  // 1  - 0?
	10,       // 2  - 00.
	13,       // 3  - 01.
	JUMP(6),  // 4  - 1?
	16,       // 5  - 10.
	JUMP(8),  // 6  - 11?
	19,       // 7  - 110.
	JUMP(11), // 8  - 111?
	JUMP(13), // 9  - 1110?
	22,       // 10 - 11100.
	JUMP(14), // 11 - 1111?
	28,       // 12 - 11110.
	25,       // 13 - 11101.
	32,       // 14 - 11111.
    };

static struct lencode_s {
    int len, code, codelen;
} codes[] = {
    {10, 0, 2},
    {13, 1, 2},
    {16, 2, 2},
    {19, 6, 3},
    {22, 28, 5},
    {25, 29, 5},
    {28, 30, 5},
    {32, 31, 5},
};

enum {
    MSG_KEEPALIVE,
    COMMON_LAST
};

enum {
    SM_MAGIC=COMMON_LAST,
    SM_INIT,
    SM_WORLD,
    SM_SETEYE,
    SM_QUIT,
    SM_ACTOR_DEL,
    SM_BEGINFRAME,
    SM_ENDFRAME,
    SM_SOUND,
    SM_SOUND_POS,
};

enum {
    CM_HELLO=COMMON_LAST,
    CM_JOIN,
    CM_INPUT,
    CM_QUIT,
    CM_SPECIAL,
    CM_SUSPEND,
    CM_RESUME
};


#define UF_LOC 1
#define UF_ANG 2
#define UF_AREA 4
#define UF_CLASS 8
#define UF_SIZE 16
#define UF_NOLERP 32

#define UF_ALL 31
#define UF_INIT 31

#define UF_REMOVE 128

class Connection;

#define Swap16 SDL_SwapBE16
#define Swap32 SDL_SwapBE32

class Message;

static inline Uint32 float_to_u32(float x) {
    union {
        float f;
        Uint32 i;
    };
    f = x;
    return i;
}
static inline float u32_to_float(Uint32 x) {
    union {
        float f;
        Uint32 i;
    };
    i = x;
    return f;
}


class Message : public Object {
public:

    double time;

    int type;
    
    string data;

    /*Uint8* data;
      int len,maxlen;*/

    int bitsinbucket;
    Uint32 bitbucket;
    Uint32 fp;

    Message(int type=0) : type(type) {
	time = -1;
	bitsinbucket=0;
	bitbucket=0;
	fp = 0;
    }
    Message(int type, string data) : type(type),data(data) {
	bitbucket = bitsinbucket = 0;
	time = -1;
	fp = 0;
    }
    ~Message() {
    }
    
    inline int size() {
	if (data.size() == 0) return 1;
	if (data.size() < 128) return data.size() + 2;
	if (data.size() < 16384) return data.size() + 3;
	return data.size() + 4;
    }

    inline Message& writemessage(Message* msg) {
	int stat = msg->type & 127;
	if (msg->data.size()) {
	    writebyte(stat|128);
	    writevar(msg->data.size());
	} else writebyte(stat);
	write(msg->data);
        return *this;
    }
    inline void readinto(Message* msg) {
	int stat = readbyte();
	if (stat==-1) {
	    msg->type=0;
	    msg->fp=0;
	    
	    return;
	}
	msg->fp = 0;
	msg->type = stat&127;
	if (!(stat&128)) return;
	int msglen = readvar();
	msg->data = read(msglen);

    }
    inline sptr<Message> readmessage() {
	sptr<Message> ret = GC::track(new Message(0));
	readinto(ret);
	return ret;
    }

    inline void truncate(int nlen=-1) {
	data.resize(nlen);
	if (fp>data.size()) fp=data.size();
    }

    inline void readalign() {
	bitbucket = bitsinbucket = 0;
    }

    inline Uint32 readbits(int nbits) {
	Uint32 ret;
	if (nbits<=bitsinbucket) {
	    ret = bitbucket>>(bitsinbucket-nbits);
	    bitsinbucket -= nbits;
	    bitbucket &= (1<<bitsinbucket)-1;
	    return ret;
	} else {
	    ret = bitbucket;
	    nbits -= bitsinbucket;
	    bitsinbucket = bitbucket = 0;
	    while (nbits) {
		if (nbits>=16) {
		    ret <<= 16; 
		    ret |= readshort();
		    nbits -= 16;
		} else if (nbits >= 8) {
		    ret <<= 8; 
		    ret |= readbyte();
		    nbits -= 8;
		} else {
		    ret <<= nbits;
		    bitbucket = readbyte();
		    ret |= (bitbucket>>(8-nbits));
		    bitsinbucket = 8-nbits;
		    bitbucket &= (1<<bitsinbucket)-1;
		    nbits = 0;
		}
	    }
	    return ret;
	}
    }

    inline Message& writealign() {
	while (bitsinbucket>=16) {
	    writeshort(bitbucket>>(bitsinbucket-16));
	    bitsinbucket -= 16;
	}
	if (bitsinbucket>=8) {
	    writebyte(bitbucket>>(bitsinbucket-8));
	    bitsinbucket -= 8;
	}

	if (bitsinbucket>0) {
	    writebyte(bitbucket<<(8-bitsinbucket));
	}
	bitsinbucket = bitbucket=0;

	return *this;
    }

    inline Message& writebits(Uint32 val, unsigned int nbits) {
	if (bitsinbucket==0)
	    switch (nbits) {
	    case 8: writebyte(val); return *this;
	    case 16: writeshort(val); return *this;
	    case 32: writeint(val); return *this;
	    }

	while (nbits) {
	    unsigned int b2w = 32-bitsinbucket;
	    if (b2w > nbits) b2w = nbits;
	    bitbucket <<= b2w;
	    bitsinbucket += b2w;
	    bitbucket |= (val>>(nbits-b2w))&((1<<b2w)-1);
	    nbits -= b2w;
	    if (bitsinbucket == 32) {
		writeint(bitbucket);
		bitsinbucket = bitbucket = 0;
	    }
	}
	return *this;
    }
    Message& writecoord(int coord) {
	int sgn = 0;
	if (coord<0) {
	    sgn=1;
	    coord=-coord;
	}
	writebits(sgn,1);
	for (int i=0;;i++) {
	    if (!((unsigned)coord>>codes[i].len) || (codes[i].len==32)) {
		writebits(codes[i].code,codes[i].codelen);
		writebits(coord,codes[i].len);
		return *this;
	    }
	}
	
	return *this;
    }
    int readcoord() {
	int sgn = readbits(1);
	int v, r = 0;
	while ((v=lenhufft[r])&1024)
	    r=readbits(1)?v&1023:r+1;
	int ret = readbits(v);
	if (sgn) return -ret;
	return ret;
	
    }
    
    inline string read(int ln) {
	if (fp + ln > data.size()) {
	    ln = data.size() - fp;
	}
	string ret = data.substr(fp, ln);
	fp += ln;
	return ret;
    }
    inline int read(Uint8* buf, int ln) {
	if (fp + ln > data.size()) {
	    ln = data.size() - fp;
	}
	memcpy(buf, data.data()+fp, ln);
	fp += ln;
	return ln;
    }
    
    inline Message& write(const Uint8* buf, int ln) {
	data.replace(fp, ln, (const char*)buf, ln);
	fp += ln;
	return *this;
    }

    inline Message& write(const string& buf) {
	data.replace(fp, buf.size(), buf);
	fp += buf.size();
	return *this;
    }

    inline Message& writebyte(Uint8 i) {
	data.replace(fp, 1, 1, i);
	fp++;
	return *this;
    }

    inline Message& writeshort(Uint16 i) {
	Uint16 ni = htons(i);
	return write((Uint8*)&ni, 2);
    }

    inline Message& writeint(Uint32 i) {
	Uint32 ni = htonl(i);
	return write((Uint8*)&ni, 4);
    }

    inline Message& writevar(Uint32 var) {
	Uint8 buf[6];
	int pos=6;
	do {
	    buf[--pos]=var|128;
	    var >>= 7;
	} while (var);
	buf[5] &= 127;
	return write(buf+pos,6-pos);
    }

    inline Message& writefloat(float f) {
	return writeint(float_to_u32(f));
    }

    inline Message& writedouble(double f) {
	if (SDL_BYTEORDER==SDL_BIG_ENDIAN) {
	    return write((Uint8*)&f, 8);
	} else {
	    Uint8* tbuf = (Uint8*)&f;
	    for (int i=0; i<8; fp++, i++) {
		data.replace(fp, 1, 1, tbuf[7-i]);
	    }
	    return *this;
	}
    }

    inline Message& writevect(vect3d v) {
	writefloat(v.x);
	writefloat(v.y);
	writefloat(v.z);
	return *this;
    }

    inline Message& writestring(const string& buf) {
	writevar(buf.size());
	write(buf);

	return *this;
    }

    inline bool iseof(int ofs=1) {
	return (fp+ofs>data.size());
    }
    inline void checkeof(int ofs=1) {
	if (iseof(ofs)) throw SocketError(string("Unexpected EOF"));
    }
    inline int tell() {
	return fp;
    }

    inline void seek(int pos) {
	fp = pos;
	if (fp < 0) fp = 0;
	if (fp > data.size()) fp = data.size();
    }

    inline int readbyte() {
	checkeof();
	return (unsigned char)data[fp++];
    }

    inline int readshort() {
	checkeof(2);
	int ret = Swap16(*((const Uint16*)(data.data()+fp)));
	fp += 2;
	return ret;
    }

    inline int readint() {
	checkeof(4);
	int ret = Swap32(*((const Uint32*)(data.data()+fp)));
	fp += 4;
	return ret;
    }

    inline Uint32 readvar() {
	Uint32 ret = 0;
	int cb;
	int safety = 6;
	do {
	    ret <<= 7;
	    ret |= ((cb = readbyte())&127);
	} while ((--safety) && (cb&0x80));
	return ret;
    }

    inline float readfloat() {
	int i = readint();
	return u32_to_float(i); // this should be NaN if i is -1
    }

    inline double readdouble() {
	checkeof(8);
	double ret;
	if (SDL_BYTEORDER==SDL_BIG_ENDIAN) {
	    memcpy(&ret,data.data()+fp,8);
	    fp += 8;
	} else {
	    Uint8* tbuf = (Uint8*)&ret;
	    for (int i=0; i<8; i++) {
		tbuf[7-i] = data[fp++];
	    }
	}
	return ret;
    }

    inline string readstring() {
	int ln = readvar();
	checkeof(ln);
	string ret(data.substr(fp,ln));
	fp += ln;
	return ret;
    }

    inline vect3d readvect() {
	double x = readfloat();
	double y = readfloat();
	double z = readfloat();
	return vect3d(x,y,z);
    }

    DECLARE_OBJECT;
};

class MessageHandler;
class SMDPConnectionManager;
class SMDPConnection;

class SMDPUDPConnection;
class SMDPUDPSocket;

class MessageHandler : virtual public Object {
protected:
    sptr<SMDPConnection> conn;
    friend class SMDPConnectionManager;
public:

    virtual bool handle(Message* msg) = 0;
    virtual void dropped(int num);
    virtual void disconnected();
    sptr<SMDPConnection> getconn();

    DECLARE_OBJECT;
};

class SMDPConnectionManager : virtual public Object, public Mutex {
public:

    set< sptr<SMDPConnection> > connections;

    virtual sptr<MessageHandler> create_handler() = 0;
    void add(SMDPConnection* c);
    void remove(SMDPConnection* c);

    DECLARE_OBJECT;
};

class SMDPConnection : public Object, public Mutex {
    vector< sptr<Message> > receive_queue;
    
protected:
    friend class SMDPConnectionManager;
    sptr<MessageHandler> handler;
    sptr<SMDPConnectionManager> mgr;

    void swapqueue(vector< sptr<Message> >& rqueue) {
	Lock l(this);
	receive_queue.swap(rqueue);
    }

public:
    virtual void putmessage(Message* msg);
    virtual void disconnect();
    virtual string describe_peer() = 0;
    virtual void handle_queue();
    virtual bool sendmessage(Message* msg, MessageChannel chan = CHAN_UNRELIABLE) = 0;
    virtual void sendpacket() { }
    sptr<MessageHandler> gethandler() { return handler; }

    DECLARE_OBJECT;
};

class SMDPUDPConnection : public SMDPConnection {
protected:
    sptr<SMDPUDPSocket> sock;
    Address peer;
    friend class SMDPUDPSocket;

public:


    deque< sptr<Message> > reliable_messages;
    deque< sptr<Message> > unreliable_messages;

    Uint32 queue_size;

    Uint32 sendseq; // the number of the next packet we send
    Uint32 recvseq; // the number of the next packet we expect to receive
    Uint32 sendrelseq; // the logical number of the first message in the reliable vector
    Uint32 recvrelseq; // the logical number of the next reliable message we receive

    Uint32 dropcount;

    bool need_ack;
    bool isnew;
    bool holdoff;

    SMDPUDPConnection(SMDPUDPSocket* sock, Address peer);
    ~SMDPUDPConnection();


    
    void disconnect();

    bool gotpacket(Uint8* data, int len);

    virtual string describe_peer() { return peer.str(); }
    void handle_queue();
    void sendpacket();
    bool sendmessage(Message* msg, MessageChannel chan = CHAN_UNRELIABLE);

    DECLARE_OBJECT;
};


class SMDPUDPSocket : public UDPSocket {
    friend class SMDPUDPConnection;
    bool listen;

    sptr<SMDPConnectionManager> mgr;
    map<Address, sptr<SMDPUDPConnection> > connections;
public:


    SMDPUDPSocket(SMDPConnectionManager* mgr, int port = 0, bool listen = false) 
	: UDPSocket(port), listen(listen), mgr(mgr) { }
    sptr<SMDPConnection> connect(const Address& to);

    void readready(SocketGroup* sg);

    DECLARE_OBJECT;
};

class SMDPLoopbackConnection : public SMDPConnection {
    sptr<SMDPLoopbackConnection> peer;
    bool isconnected;

public:
    SMDPLoopbackConnection() : isconnected(false) { }
    sptr<SMDPLoopbackConnection> getpeer();
    void disconnect();
    string describe_peer() { return "<loopback>"; }
    bool sendmessage(Message* msg, MessageChannel chan = CHAN_UNRELIABLE);

    DECLARE_OBJECT;
};

#endif
