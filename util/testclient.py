import sys,os,socket,struct,time;

def readbyte(s):
    return ord(s.recv(1));
def readshort(s):
    return struct.unpack(">H",s.recv(2))[0];
def readint(s):
    return struct.unpack(">i",s.recv(4))[0];
def readfloat(s):
    return struct.unpack(">f",s.recv(4))[0];
def readvect(s):
    x=readfloat(s);
    y=readfloat(s);
    z=readfloat(s);
    return x,y,z

def readstring(s):
    len = readshort(s)
    return s.recv(len);

UF_LOC=1
UF_ANG=2
UF_VEL=4
UF_ANGV=8
UF_DRAW=16
UF_SIZE=32
UF_REMOVE=64

class actor(object):
    def __init__(self,id):
        self.loc=0,0,0
        self.sect=0;
        self.vel=0,0,0
        self.ang=0
        self.angv=0
        self.drdesc = ''
        self.size=0,0,0
        self.id=id;
    def dbg(self,msg):
        print "actor %d: %s"%(self.id,msg)
    def read(self,s):
        uf = readbyte(s)
        if uf&UF_REMOVE:
            self.dbg("remove");
            return False
        if uf&UF_LOC:
            self.loc=readvect(s)
            self.sect=readint(s)
            self.dbg("set loc "+str(self.loc)+" "+str(self.sect))
        if uf&UF_ANG:
            self.ang=readfloat(s)
            self.dbg("set ang "+str(self.ang))
        if uf&UF_VEL:
            self.vel=readvect(s)
            self.dbg("set vel "+str(self.vel))
        if uf&UF_ANGV:
            self.angv=readfloat(s)
            self.dbg("set angv "+str(self.angv))
        if uf&UF_DRAW:
            self.drdesc=readstring(s)
            self.dbg("set draw "+str(self.drdesc))
        if uf&UF_SIZE:
            self.drdesc=readvect(s)
            self.dbg("set size "+str(self.size))
        return True

actors={}

stoptim=time.time()+3;

def readloop(s):
    global stoptim
    while True:
        if stoptim != 0 and time.time()>=stoptim:
            sock.send(struct.pack(">BBB",0,0x55,0));
            stoptim = 0;
            print "Stop!"
            
        aid = readshort(s);
        ca = actors.get(aid,None)
        if ca is None:
            ca = actor(aid);
            actors[aid]=ca
        if not ca.read(s):
            del actors[aid];

sock=socket.socket(socket.AF_INET,socket.SOCK_STREAM)
sock.connect(('localhost',4242))

wname = readstring(sock)
print "world name: %s"%wname;
myplayerid = readshort(sock)
print "my player id: %d"%myplayerid;

sock.send(struct.pack(">BBB",0,0x54,0));

readloop(sock);
