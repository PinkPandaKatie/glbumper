import sys,random,socket,select,time,struct
from cStringIO import StringIO as sio

from glbumper.dataio import DataIO

MSG_KEEPALIVE=0

SM_MAGIC=1
SM_INIT=2
SM_QUIT=3
SM_TEMPLATEDEF=4
SM_NEW_OBJECT=5
SM_DEL_OBJECT=6
SM_MOMENTARY_OBJECT=7
SM_SYNC=8

CM_HELLO=1
CM_JOIN=2
CM_QUIT=3
CM_INPUT=4 
CM_CMD=5
CM_SUSPEND=6
CM_RESUME=7




CHAN_UNRELIABLE = 0
CHAN_UNRELIABLE_QUEUE = 1
CHAN_RELIABLE = 2

MAX_DGRAM = 640

class Message(DataIO):
    def __init__(self, type=0, data=None):
        DataIO.__init__(self, data)
        self.type = type
    
    def message_size(self):
        ln = len(self.getvalue())
        if ln == 0: return 1
        if ln < 128: return ln+2
        if ln < 16384: return ln+3
	return ln + 4

    def writeto(self, ds):
        stat = self.type & 127
        data = self.getvalue()
        ml = len(data)
        if ml:
	    ds.writebyte(stat|128);
	    ds.writevar(ml);
	else:
            ds.writebyte(stat);
	ds.write(data)

def readmessage(ds):
    stat = ds.readbyte()
    typ = stat&127

    len = 0
    if stat & 128: len = ds.readvar()

    dat = ds.read(len)

    return Message(typ, dat)

    
class ConnectionManager(object):
    def __init__(self, handler_creator):
        self.connections = set()
        self.creator = handler_creator
    def create_handler(self):
        return None
    
    def add(self, conn):
        if conn.handler is None:
            conn.handler = self.creator(conn)
            conn.mgr = self
        self.connections.add(conn)
        
    def remove(self, conn):
        if conn in self.connections:
            self.connections.remove(conn)


class Connection(object):
    def __init__(self):
        self.handler = None
        self.mgr = None
        self.receive_queue = []

    def putmessage(self, msg):
        if not self.handler:
            return
        self.receive_queue.append(msg)
    
    def disconnect(self):
        if self.mgr:
            self.mgr.remove(self)
            self.mgr = None
        if self.handler:
            self.handler.disconnected()
            self.handler = None
    
    def handle_queue(self):
        q = self.receive_queue
        self.receive_queue = []
        for msg in q:
            if not self.handler or not self.handler.handle(msg):
                self.disconnect()
                break
    
    def sendmessage(self, msg, chan = CHAN_UNRELIABLE):
        pass
    
    def sendpacket(self):
        pass
    
    def gethandler(self):
        return self.handler

class UDPConnection(Connection):
    def __init__(self, sock, raddr):
        Connection.__init__(self)
        self.peer = raddr
        self.sock = sock
        
        self.queue_size = 16

        self.sendseq = 0
        self.recvseq = 0
        self.sendrelseq = 0
        self.recvrelseq = 0
        
        self.reliable_messages = []
        self.unreliable_messages = []
        
        self.dropcount = 0
        
        self.need_ack = False
        self.isnew = True
        self.holdoff = False

    def disconnect(self):
        if self.sock:
            if self.peer in self.sock.connections:
                del self.sock.connections[self.peer]
                self.sock = None

        Connection.disconnect(self)

    def handle_queue(self):
        self.holdoff = True
        Connection.handle_queue(self)
        self.holdoff = False
        self.sendpacket()

    def gotpacket(self, data):
        print 'got packet %r'%data
        if not self.handler:
            return False

        ctime = time.time();

        if len(data) < 16:
            if self.isnew:
                return False
            return True

        self.holdoff = True

        pkt = DataIO(data)

        seq = pkt.readint()
        relseq = pkt.readint()
        relack = pkt.readint()
        numrel = pkt.readshort()
        numunrel = pkt.readshort()

        if seq < self.recvseq:
            return True
        
        if seq != self.recvseq:
            num = seq - self.recvseq;
            self.dropcount += num
            self.handler.dropped(num)

        self.recvseq = seq + 1
        
        # numrel == 65535 has a special meaning: it means there are no reliable
        # messages, but we should send an acknowledgement anyway.
        if numrel != 0 and numrel != 65535:
            need_ack = True
            numdups = self.recvrelseq - relseq
            self.recvrelseq = relseq + numrel
            for i in xrange(numrel):
                msg = readmessage(pkt)
                msg.time = ctime
                if i >= numdups:
                    self.putmessage(msg)

        for i in xrange(numunrel):
            msg = readmessage(pkt)
            msg.time = ctime
            self.putmessage(msg)

        # okay, process the acks
        if relack > self.sendrelseq:
            numrecv = relack - self.sendrelseq;

            self.sendrelseq = relack;

            while self.reliable_messages:
                msg = self.reliable_messages[0]
                self.queue_size -= msg.message_size()
                del self.reliable_messages[0]

        if ((numrel == 65535 or numrel == 0) and numunrel == 0) and self.isnew:
            return False

        self.isnew = False
        self.holdoff = False

        return True

    def sendpacket(self):
        if not self.need_ack and not self.reliable_messages and not self.unreliable_messages:
            return

        if not self.sock:
            return

        if self.holdoff:
            return

        pkt = DataIO()
        pkt.writeint(self.sendseq)
        pkt.writeint(self.sendrelseq)
        pkt.writeint(self.recvrelseq)
        pkt.writeshort(0)
        pkt.writeshort(0)

        numrel = 0
        numunrel = 0
        trunc = False
        for msg in self.reliable_messages:
            if not msg:
                print "FUCK ME! IN THE ASSHOLE!"
            if msg.message_size() + pkt.tell() >= (MAX_DGRAM-4):
                trunc = True
                break
            numrel += 1
            msg.writeto(pkt)

        while not trunc and self.unreliable_messages:
            msg = self.unreliable_messages[0];
            if msg.message_size() + pkt.tell() >= (MAX_DGRAM-4):
                trunc = True
                break
            msg.writeto(pkt)
            numunrel += 1

            self.queue_size -= msg.message_size()
            del self.unreliable_messages[0]

        self.sendseq += 1

        self.need_ack = False

        pkt.seek(12)
        if numrel == 0 and trunc:
            pkt.writeshort(65535)
        else:
            pkt.writeshort(numrel)
        pkt.writeshort(numunrel)

        if trunc:
            self.holdoff = True


        print 'sending %r to %r'%(pkt.getvalue(), self.peer)
        self.sock.sock.sendto(pkt.getvalue(), self.peer);
        

    def sendmessage(self, msg, chan):
        self.isnew = False
        msg.writealign();
        if chan == CHAN_UNRELIABLE:
            if msg.message_size() + self.queue_size >= (MAX_DGRAM - 4):
                return False
            
        self.queue_size += msg.message_size()
        if chan == CHAN_RELIABLE:
            self.reliable_messages.append(msg)
        else:
            self.unreliable_messages.append(msg)
                
        return True

    def fileno(self):
        return self.sock.sock.fileno()

    def processmsg(self,msg):
        print msg.type
        return True
    
class UDPSocket(object):
    def __init__(self, mgr, port = 0, listen = False):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('', port))
        self.listen = listen
        self.mgr = mgr
        self.connections = {}

    def fileno(self):
        return self.sock.fileno()

    def process_packet(self, conn, data):
        if not conn.gotpacket(data):
            self.mgr.remove(conn)

    def read_ready(self):
        data, peer = self.sock.recvfrom(1024)
        if peer in self.connections:
            self.process_packet(self.connections[peer], data)
            return
        
        if not self.listen:
            return

        nconn = UDPConnection(self, peer)
        self.mgr.add(nconn)
        self.process_packet(nconn, data)
        self.connections[peer] = nconn
        
    def connect(self, addr):
        peer = (socket.gethostbyname(addr[0]), addr[1])
        nconn = UDPConnection(self, peer)
        nconn.isnew = False
        self.mgr.add(nconn)
        self.connections[peer] = nconn
        return nconn
