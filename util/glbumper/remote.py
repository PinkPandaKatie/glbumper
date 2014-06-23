import sys, random, socket, time, math

from glbumer.vector import vect3d

EF_REL = 0x0100

LM_IMM = 0x00
LM_LATCH = 0x40
LM_LERP = 0x80
LM_LERP_CYC = 0xc0

CONN_DISCONNECTED = 0
CONN_SENT_HELLO = 1
CONN_SENT_JOIN = 2
CONN_CONNECTED = 3


def lerpmode(e):
    return ((e) & 0xc0)
def wantlerp(e):
    return (((e) & 0x80) == 0x80)

COORD_2D = 0x00
COORD_3D = 0x10
COORD_3D_A = 0x20
COORD_3D_A_REL = 0x30

STAMP_CHANGED = 1
STAMP_UNCHANGED = 2
STAMP_FIRST = 3


DONT_SEND = 0
SEND_UNRELIABLE = 1
SEND_RELIABLE = 2

def int32(sgn):
    def read(msg):
        i = msg.readbits(32)
        if sgn and i >= 0x80000000L: i -= 0x80000000L
        return i
    
    def write(msg, val):
        msg.writebits(val & 0xffffffff, 32)

    return (read, write, ('unsigned', 'signed')[sgn] + '32')

def varint():
    def read(msg):
        bits = 0;
        while True:
            c = msg.readbits(8)
            bits <<= 7
            bits |= (c&0x7f)
            if not (c & 0x80):
                break
        return bits

    def write(msg, var):
        sh = 28
        f=w=0
        while True:
            w = (var>>sh)&127
            if not sh: break
            sh -= 7
            if w or f:
                f=1
                msg.writebits(w|128, 8)
        self.writebits(w, 8)

    return (read, write, 'small')

intcodecs = [ int32(True), int32(False), varint() ]

def float32():
    def read(msg):
        return u32_to_float(msg.readbits(32))

    def write(msg, val):
        msg.writebits(float_to_u32(val), 32)

    return (read, write, 'single')

def float_unit(nbits):
    div = (1.0 / (1 << nbits))
    def read(msg):
        b = msg.readbits(nbits)
        return (i * div)

    def write(msg, val):
        if val < 0.0: val = 0.0
        if val > 1.0: val = 1.0
        msg.writebits(int(val * 255.0), 8)

    return (read, write, 'unit(%d)'%nbits)

def float_var(scl):
    coordtbld=(68,67,10,13,70,16,72,19,75,77,22,78,28,25,32)
    coordtble=(
        (10,  0, 2), (13, 1,  2),
        (16,  2, 2), (19, 6,  3),
        (22, 28, 5), (25, 29, 5),
        (28, 30, 5), (32, 31, 5))
    
    iscl = 1.0 / scl
    def read(ds):
	sgn = ds.readbits(1)
	r = 0
        rb = 1
        while True:
            v = coordtbld[r]
            if not v & 64:
                break
            
            if ds.readbits(1):
                r = v&63
            else:
                r += 1
                
	ret = ds.readbits(v) * iscl
        
        if sgn:
            return -ret
	return ret

    def write(ds, val):
        if coord<0:
            ds.writebits(1, 1)
            coord = -coord
        else:
            ds.writebits(0,1)
            
        for l, c, cl in coordtble:
            if not (coord >> l):
		ds.writebits(c, cl);
		ds.writebits(coord, l);
		return
    
    return (read, write, 'var(%f)'%scl)
        
floatcodecs = [
    float32(),
    float_unit(8), float_unit(12),
    float_var(1.0), float_var(32.0), float_var(64.0), float_var(512.0)
]



proptypes = {}

class PropertyDef(object):
    def __init__(self, name, typ, enc, default=None):
        self.name = name
        self.typ = typ
        self.enc = enc
        if default == None:
            default = proptypes[chr(typ)].propclass()
        self.default = default

    def create_manager(self):
        return proptypes[chr(self.typ)](self)
    
    def __str__(self):
        return '%s: %s'%(self.name, self.create())
        
class Template(object):
    def __init__(self, name, props):
        self.name = name
        self.props = [propertydef(name, typ, enc, default) for name, typ, enc, default in props]

    def __str__(self):
        return 'template "%s" ['%self.name+', '.join([str(p) for p in self.props])+']'

class Property(object):
    def __init__(self, val = None):
        self.stamp = STAMP_UNCHANGED

    def copy(self):
        pass

class PropertyManager(object):
    def __init__(self, pdef):
        self.prop = pdef.default.copy()
        self.updated = False
        self.pdef = pdef
        
    def read(self, ds, map):
        return False

    def latch(self):
        return False
    
    def lerp(self, t):
        return False

    def write(self, ds, map, lastseen, cstamp):
        return DONT_SEND
    
def primitiveproperty(typ):
    class PrimitiveProperty(Property):
        primtype = typ
        
        def __init__(self, v = typ()):
            Property.__init__(self)
            self._val = v
            
        def __setattr__(self, name, val):
            if name == 'val':
                if val != self._val:
                    self.stamp = STAMP_CHANGED
                    self._val = val
            else:
                object.__setattr__(self, name, val)

        def __getattr__(self, attr):
            if attr == 'val':
                return self._val
            else:
                return object.__getattribute__(self, attr)
            
        def copy(self):
            return PrimitiveProperty(self._val)

StringProperty = primitiveproperty(str)
IntProperty = primitiveproperty(int)
FloatProperty = primitiveproperty(float)
RefProperty = primitiveproperty(lambda: None)

class CoordProperty(Property):
    def __init__(self, loc = vect3d(), area = -1):
        Property.__init__(self)
        self._loc = loc
        self._area = area
        
    def __setattr__(self, name, val):
        if name == 'loc':
            if val != self._loc:
                self.stamp = STAMP_CHANGED
                self._loc = val
        elif name == 'area':
            if val != self._area:
                self.stamp = STAMP_CHANGED
                self._area = val
        elif name == 'val':
            loc, area = val
            if loc != self._loc or area != self._area:
                self.stamp = STAMP_CHANGED
                self._loc = loc
                self._area = area
        else:
            object.__setattr__(self, name, val)

    def __getattr__(self, attr):
        if attr == 'loc':
            return self._loc
        elif attr == 'area':
            return self._area
        elif attr == 'val':
            return self._loc, self._area
        else:
            return object.__getattribute__(self, attr)
        
    def copy(self):
        return coordproperty(self._loc, self._area)


class StringPropertyManager(PropertyManager):
    propclass = StringProperty
    
    def __init__(self, pdef):
        PropertyManager.__init__(self, pdef)
        
    def read(self, msg, map):
        msg.readalign()
        nval = msg.readstring()
        if nval != self.prop._val:
            self.prop.val = nval
            return True
        return False

    def __str__(self):
        return 'string %r'%self.prop._val

proptypes['S'] = StringPropertyManager

class IntPropertyManager(PropertyManager):
    propclass = intproperty
    def __init__(self, pdef):
        PropertyManager.__init__(self, pdef)
        self.codec = intcodecs[pdef.enc & 15]
        self.writev = pdef.default
        
    def read(self, msg, map):
        enc = self.pdef.enc
        lm = lerpmode(enc)
        nval = self.codec[0](msg)
        
        if lm == LM_IMM:
            if nval != self.prop._val:
                self.prop.val = nval
                return True
            return False
        
        self.writev = nval
        return False
    
    def latch(self):
        if lerpmode(self.pdef.enc) != LM_IMM:
            if self.writev != self.prop._val:
                self.prop.val = self.writev
                return True
        return False
            
    def __str__(self):
        return 'int(%s) %r'%(self.codec[2], self.prop._val)
        
proptypes['I'] = IntPropertyManager
    
class RefPropertyManager(PropertyManager):
    propclass = RefProperty
    def __init__(self, pdef):
        PropertyManager.__init__(self, pdef)
        self.writev = pdef.default
        self.id = -1
        
    def read(self, msg, map):
        enc = self.pdef.enc
        lm = lerpmode(enc)

        self.id = read_smallint(msg) - 1
        nval = map.get(self.id, None)

        if lm == LM_IMM:
            if nval != self.prop._val:
                self.prop.val = nval
                return True
            return False
        self.writev = nval
        return False
    
    def latch(self):
        if lerpmode(self.pdef.enc) != LM_IMM:
            if self.writev != self.prop._val:
                self.prop.val = self.writev
                return True
        return False
            
    def __str__(self):
        return 'ref -> %d(%r)'%(self.id, self.prop.val)
    
proptypes['R'] = RefPropertyManager

def frac(t):
    return t - math.floor(t)

def frac_rel(t, rel):
    return t - math.floor(t + 0.5 - rel)

class FloatPropertyManager(PropertyManager):
    propclass = FloatProperty
    def __init__(self, pdef):
        PropertyManager.__init__(self, pdef)
        self.writeold = 0.0
        self.writenew = 0.0
        self.readold = 0.0
        self.readnew = 0.0
        self.codec = floatcodecs[pdef.enc & 15]
    
    def read(self, msg, map):
        enc = self.pdef.enc
        lm = lerpmode(enc)
        
        if wantlerp(enc):
            dolerp = msg.readbits(1)
        else:
            dolerp = 0
            
        nval = self.codec[0](msg)
        #print '%s: %f'%(self.pdef.name, nval)
        
        # If we change immediately on receipt, change _val_ now.
        if lm == LM_LERP_CYC:
            self.prop._val = frac(self.prop._val)
            
        if lm == LM_IMM:
            if self.prop._val != nval:
                self.prop.val = nval
                return True
            return False
        
        # So we don't change immediately. Write the new value into
        # _writenew_. 

        self.writenew = nval

        # If we don't lerp, or the server does not want us to lerp to the
        # new value, write the new value into _writeold_ so lerp() will
        # always see the new value.

        if not dolerp or self.updated:
            self.writeold = nval
            
        self.updated = True
        
        # Unless we're immediate, we don't change the value in this
        # function.
        return False

    def latch(self):
        enc = self.pdef.enc
        lm = lerpmode(enc)
        if lm == LM_IMM:
            return False
        elif lm == LM_LATCH:
            if self.writenew != self.prop._val:
                self.prop.val = self.writenew
                return True
            return False
        elif lm == LM_LERP:
            self.readold = self.writeold
            self.readnew = self.writenew
        elif lm == LM_LERP_CYC:
            self.readold = frac(self.writeold)
            self.readnew = frac_rel(self.writenew, self.readold)
        self.writeold = self.writenew
        self.updated = False
        return False
        
    def lerp(self, t):
        enc = self.pdef.enc
        lm = lerpmode(enc)

        if not wantlerp(enc):
            return False

        nval = (self.readnew - self.readold) * t + self.readold;

        if lm == LM_LERP_CYC:
            nval = frac(nval)
    
        if nval != self.prop._val:
            self.prop.val = nval
            return True
        return False

    def __str__(self):
        return 'float(%s) %r'%(self.codec[2], self.prop.val)

proptypes['F'] = FloatPropertyManager



class CoordPropertyManager(PropertyManager):
    propclass = CoordProperty
    
    def __init__(self, pdef):
        PropertyManager.__init__(self, pdef)
        self.writeold = self.defaultval
        self.writenew = self.defaultval
        self.readold = self.defaultval
        self.readnew = self.defaultval
        self.codec = floatcodecs[pdef.enc & 15]
    
    def read(self, msg, map):
        enc = self.pdef.enc
        lm = lerpmode(enc)
        
        vectyp = (enc & 0x30) >> 4
        
        if wantlerp(enc):
            dolerp = msg.readbits(1)
        else:
            dolerp = 0

        narea = -1
        if vectyp > 1:
            narea = read_smallint(msg) - 1
            
        nx = self.codec[0](msg)
        ny = self.codec[0](msg)
        nz = 0.0
        if vectyp > 0:
            nz = self.codec[0](msg)
            
        nval = vect3d(nx, ny, nz), narea

        self.lastread = nval
        
        #print '%s: %f'%(self.pdef.name, nval)
        
        # If we change immediately on receipt, change _val_ now.
        if lm == LM_IMM:
            if self.prop.val != nval:
                self.prop.val = nval
                return True
            return False
        # So we don't change immediately. Write the new value into
        # _writenew_. 

        self.writenew = nval;

        # If we don't lerp, or the server does not want us to lerp to the
        # new value, write the new value into _writeold_ so lerp() will
        # always see the new value.

        if not dolerp or self.updated:
            self.writeold = nval
            
        self.updated = True
        # Unless we're immediate, we don't change the value in this
        # function.
        return False

    def latch(self):
        enc = self.pdef.enc
        lm = lerpmode(enc)
        # If we're immediate, we don't change anything here.
        if lm == LM_IMM:
            return False
        elif lm == LM_LATCH:
            if self.prop.val != nval:
                self.prop.val = nval
                return True
            return False
        else:
            self.readold = self.writeold
            self.readnew = self.writenew
            self.writeold = self.writenew
            self.updated = False
            return False
        
    def lerp(self, t):
        enc = self.pdef.enc

        if not wantlerp(enc):
            return False

        oloc, oarea = self.readold
        nloc, narea = self.readnew
        nloc = (nloc - oloc) * t + oloc;

        nval = nloc, oarea
    
        if nval != self.prop.val:
            self.prop.val = nval
            return True
        return False

    def __str__(self):
        return 'coord(%s) %r'%(self.codec[2], self.prop.val)

proptypes['C'] = CoordPropertyManager





class RemoteObject(object):
    def __init__(self, templ):
        self.templ = templ
        self.props = [pd.create_manager() for pd in templ.props]
        self.byname = dict((pr.pdef.name, pr) for pr in self.props)
        self._debug_id = None
        
    def prop_changed(self, name):
        #print "object %d(%s).%s = %r"%(self._debug_id, self.templ.name, name, getattr(self, name))
        n = 'prop_change_'+name
        if hasattr(self, n):
            f = getattr(self, n)
            if callable(f): f()
            
    def read(self, msg, omap):
        n = -1
        try:
            for p in self.props:
                exist = msg.readbits(1)
                if not exist:
                    continue
                #print 'prop %s exists'%p.pdef.name
                chg = p.read(msg, omap)
                if chg:
                    self.prop_changed(p.pdef.name)

        except EOFError:
            pass

    def latch(self):
        for p in self.props:
            if p.latch():
                self.prop_changed(p.pdef.name)
               
    def lerp(self, t):
        for p in self.props:
            if p.lerp(t):
                #print 'prop %s lerped'%p.pdef.name
                self.prop_changed(p.pdef.name)

    def __getattr__(self, attr):
        try:
            return object.__getattribute__(self, attr)
        except:
            pass
        try:
            return self.byname[attr].prop.val
        except:
            pass
        return None

    def __repr__(self):
        return "%s(%s)"%(self.templ.name, ', '.join('%s: %s'%(p.pdef.name, p) for p in self.props))

    def __setattr__(self, name, val):
        if name in self.byname:
            self.byname[name].prop.val = val
        else:
            object.__setattr__(self, name, val)

class ObjectGroup(object):
    def __init__(self):
        self.objects = set()
        self.watchers = set()

    def add_object(self, o):
        if o not in self.objects:
            self.objects.add(o)
            for w in self.watchers:
                w.object_added(o)
                
    def rem_object(self, o):
        if o in self.objects:
            self.objects.remove(o)
            for w in self.watchers:
                w.object_removed(o)

    def send_momentary(self, o):
        for w in self.watchers:
            w.send_momentary(o)

    def add_watcher(self, w):
        if w not in self.watchers:
            self.watchers.add(w)
            for o in self.objects:
                w.object_added(o)
                
    def rem_watcher(self, w):
        if w in self.watchers:
            self.watchers.remove(w)
            for o in self.objects:
                w.object_removed(o)
classes = {}

class ClientMessageHandler(object):

    def __init__(self, connection):
        self.interp_begin = self.interp_end = 0.0;

        self.cstate = CONN_DISCONNECTED

        self.interp = True

        self.suspended = True

        self.lerpcount = 0

        self.lsendtime = 0.0
        self.framems = 0
        self.frametime = 0.0

        self.got_sync = 0

        self.dropwarn = False
        self.lerp_frame = False
        self.dropped_packet = False
        self.objects = ObjectGroup()
        
        self.conn = connection

        self.objectids = {}
        self.templates = {}

    def dropped(self, num):
        self.dropped_packet = True

    def disconnected(self):
        pass
    
    def quit(self):
        pass


    def input(vf, vs, vz, vr):
        lsendtime = time.time()
    
    def command(cmd):
        lsendtime = time.time()

    def suspend(self):
        if self.suspended or not self.conn:
            return
        
        msg = smdp.Message(CM_SUSPEND)
        self.conn.sendmessage(msg, smdp.CHAN_RELIABLE)
        lsendtime = time.time()

    def resume(self):
        if not self.suspended or not self.conn:
            return

        msg = smdp.Message(CM_RESUME)
        self.conn.sendmessage(msg, smdp.CHAN_RELIABLE)
        lsendtime = time.time()
        
    def tick(self):
        ctime = time.time()
        if self.conn:
            if (ctime - self.lsendtime) > 1.4:
		self.conn.sendmessage(smdp.Message(smdp.MSG_KEEPALIVE))
		lsendtime = ctime

	    self.conn.handle_queue()
        
        if self.cstate != CONN_CONNECTED:
            return
        
        self.lerpcount += 1
        if ctime > self.interp_end:
            if self.got_sync:
                self.lerp_frame = (self.got_sync == 1 and not self.dropped_packet)
                self.got_sync = 0
                self.dropped_packet = False
                
                for id, o in self.objects.iteritems():
                    o.latch()
                    
                self.interp_begin = self.interp_end;
                self.interp_end = self.interp_end + self.frametime
            else:
                sys.stdout.write('\007'); sys.stdout.flush()
                self.interp_end = ctime
        if self.lerp_frame:
            frac = (ctime - self.interp_begin) / (self.interp_end - self.interp_begin)
        else:
            frac = 1.0

        if frac < 0:
            frac = 0
            
        for id, o in self.objects.iteritems():
            o.lerp(frac)

    def create_object(self, tid, msg):
        if tid in self.templates:
            templ = self.templates[tid]
            try:
                obj = classes[templ.name](templ)
            except:
                obj = clientobject(templ)
                    
            obj.read(msg, self.objects);
            return obj
            
        
    def disconnected(self):
        pass

    def disconnected(self):
        pass
    
    def processmsg(self, msg):
        #print "message: ",msg.type
        if msg.type == SM_MAGIC:
            magic = msg.readint()
            print "magic: ",magic
            imsg = smdp.message(CM_JOIN)
            imsg.writeint(magic)
            self.rel.append(imsg)
            self.joined = True
            self.resume()
            self.send()
        elif msg.type == SM_TEMPLATEDEF:
            id = msg.readvar()
            name = msg.readstring()
            props = []
            while True:
                t = msg.readbyte()
                if not t:
                    break
                e = msg.readbyte()
                nm = msg.readstring()
                props.append((t, e, nm))

            defs = []
            for t, e, nm in props:
                val = None
                try:
                    mgr = propertydef(nm, t, e).create()
                    exist = msg.readbits(1)
                    if exist:
                        mgr.read(msg, None)
                        mgr.latch()
                        mgr.lerp(1.0)
                    val = mgr.val
                except EOFError:
                    pass
                defs.append((nm, t, e, val))
                
            temp = Template(name, defs)
            self.templates[id] = temp

            print '%d: %s'%(id, temp)
        
        elif msg.type == SM_NEW_OBJECT:
            id = msg.readvar()
            tid = msg.readvar()
            obj = self.create_object(tid, msg)
            if obj:
                self.objects[id] = obj
                obj._debug_id = id
                print "new object %d, template %d: %r"%(id, tid, obj)
                if hasattr(obj, 'draw'):
                    drawobjects.add(obj)
            else:
                print "unknown object template %d!"%(tid)

        elif msg.type == SM_MOMENTARY_OBJECT:
            tid = msg.readvar()
            obj = self.create_object(tid, msg)
            if obj:
                obj.read(msg, self.objects);
                print "momentary object %d, template %d: %r"%(id, tid, obj)
            else:
                print "unknown object template %d!"%(clasid)
                
        elif msg.type == SM_DEL_OBJECT:
            id = msg.readvar()
            print "del object:",id
            if id in self.objects:
                del self.objects[id]
            if obj in drawobjects:
                drawobjects.remove(obj)
                
        elif msg.type == SM_SYNC:
            #print 'sync'
            self.got_sync += 1
        elif msg.type == SM_QUIT:
            return False
        elif msg.type == SM_INIT:
            self.framems = msg.readint()
            self.frametime = self.framems / 1000.0
        elif msg.type >= 32:
            id = msg.type - 32
            if id >= 64:
                id -= 64
                id |= msg.readvar()<<5
            #print "update object:",id
            if id in self.objects:
                self.objects[id].read(msg, self.objects)
            self.unrel.append(smdp.message(0))
            self.send()
            
        return True

