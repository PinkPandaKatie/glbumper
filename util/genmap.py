import sys
import os
import re
import time
import math
import struct
import gzip

from cStringIO import StringIO as sio

import buildmap

from glbumper.world import *
from os.path import join

# Use Psyco if available.
try:
    import psyco
    psyco.full()
except:
    pass

def sqr(x):
    return x*x

EPSILON = 1e-10

BUILD_DIR='../editor'
OUTPUT_DIR='../gamedata/worlds'


BARSIZE = 54

def progressbar(size, val):
    return '[%s%s]'%('.'*val,' '*(size-val)) 

class Timer(object):
    def __init__(self):
        self.ltime = 0
        self.label = ''
        self.lprogress = -1
        self.total = 0
        self.lrtime = 0
        self.baton = 0
        
    def begin(self,label,total=0):
        """Marks the beginning of a period to be measured."""

        self.label = label
        if len(label) < 24:
            self.label = label + ' '*(24-len(label))
        self.total = float(total)
        self.i = 0
        self.lrtime = 0
        self.lprogress = -1;
        self.progress(0)
        self.ltime = time.time()

    def progress(self, i=None):
        if not self.total:
            sys.stderr.write('%s ...\r'%self.label)
            sys.stderr.flush()
            return
        
        if i is not None:
            self.i = i
        else:
            self.i += 1

        nprog = int((self.i / self.total) * BARSIZE)
        ctime = time.time()
        if ctime - self.lrtime > 0.025:
            baton = int((ctime - self.ltime)*20)&3
            
            sys.stderr.write('%s %s %5d/%5d %s\r'%
                             (self.label, "/-\\|"[baton],self.i, self.total,
                              progressbar(BARSIZE, nprog)));
            sys.stderr.flush()
            self.lrtime = ctime

    def msg(self, msg):
        sys.stderr.write(' ' * (BARSIZE + 24 + 5 + 12) + '\r');
        print msg
        self.lrtime = 0
        self.progress(self.i)

    def end(self):
        """Clears the progress bar and prints the time since begin() was called"""
        sys.stderr.write(' ' * (BARSIZE + 24 + 5 + 12) + '\r');
        sys.stderr.write("%s   %f s\n" % (self.label, time.time() - self.ltime))

timer = Timer()

class vect2d(object):
    def __init__(self,x=0,y=0):
        if isinstance(x,tuple):
            self.x=float(x[0])
            self.y=float(x[1])
        elif isinstance(x,vect2d):
            self.x=x.x
            self.y=x.y
        else:
            self.x=float(x); self.y=float(y);
    def __add__(self,o):
        return vect2d(self.x+o.x,self.y+o.y)
    def __sub__(self,o):
        return vect2d(self.x-o.x,self.y-o.y)
    def __mul__(self,o):
        if isinstance(o,vect2d):
            return self.dot(o)
        return vect2d(self.x*o,self.y*o)
    def __div__(self,s):
        return vect2d(self.x/s,self.y/s)
    def __rdiv__(self,s):
        raise ArithmeticError,"cannot divide by vect2d"
    def __neg__(self):
        return vect2d(-self.x,-self.y)
    def __pos__(self):
        return vect2d(self.x,self.y)
    def len(self):
        return sqrt(self.x*self.x+self.y*self.y)
    def __eq__(self,o):
        return abs(self.x-o.x)<EPSILON and abs(self.y-o.y)<EPSILON
    def __ne__(self,o):
        return not self.__eq__(o)
    __abs__=len;
    def dist(self,o):
        return abs(self-o)
    def len2(self):
        return self.x*self.x+self.y*self.y
    def dot(self,o):
        return self.x*o.x+self.y*o.y
    def __invert__(self):
        return vect2d(self.y,-self.x);
    def normal(self):
        l=self.len()
        return vect2d(self.x/l,self.y/l)
    def __len__(self):
        return 2
    def __iter__(self):
        yield self.x
        yield self.y
    def __str__(self):
        return "<%.2f,%.2f>"%(self.x,self.y);
    def __repr__(self):
        return str(self)
        return "vect2d(%.2f,%.2f)"%(self.x,self.y);

_tfid = 0

class Transform(object):
    def __init__(self):
        global _tfid

        self.id = _tfid
        _tfid += 1
        self.areas = []
        self.links = []
        self.offset = None
        self.anchor = False
    
    def transform(self, group, xo=0.0, yo=0.0):
        if self.offset:
            if self.offset != (xo, yo):
                timer.msg("WARNING: transform group can't be in two places at once.")
            return

        self.offset = xo, yo

        group.areas.extend(self.areas)
        
        for ar in self.areas:
            ar.gluepos = xo, yo
            ar.transform(xo, yo)

        for t, xofs, yofs in self.links:
            t.transform(group, xo + xofs,yo + yofs)
        self.transformed = True

class AreaGroup(object):
    def __init__(self):
        self.areas = []
        
    def writeto(self, f):
        for i,ar in enumerate(self.areas):
            if hasattr(ar,'contours'): continue
            ar.contours = []
            for w in ar.walls:
                cont = []
                while not hasattr(w, 'seen'):
                    w.seen = True
                    cont.append(w)
                    w = w.point2
                if cont:
                    ar.contours.append(cont)
            ar.idx = i
        
        f.writeu16(len(self.areas))

        ncont = nwalls = nparts = nlinks = nactors = 0

        for ar in self.areas:
            timer.progress()
            f.writestring(ar.id, usetbl = False)

            ar.ceil.writeto(f);
            ar.floor.writeto(f);
            f.writeu8(len(ar.contours))

            for c in ar.contours:
                ncont += 1
                f.writeu8(len(c))
                for w in c:
                    nwalls += 1
                    f.writef(w.x1, w.y1)
                    f.writeu8(len(w.links))
                    for oa, flags in w.links.iteritems():
                        nlinks += 1
                        f.writeu16(oa.idx)
                        f.writeu8(flags)

                    parts = [p for p in w.parts if p.tex]
                    f.writeu8(len(parts))
                    for p in parts:
                        nparts += 1
                        p.writeto(f)

            actors = [a for a in ar.actors if a.clas]
            f.writeu16(len(actors))
            for a in actors:
                nactors += 1
                f.writestring(a.clas)
                f.writef(a.x, a.y, a.z, a.ang)

                nf = sio();
                nds = datastream(nf)

                a.extra(nds)

                extra = nf.getvalue()
                f.writeu16(len(extra))
                f.write(extra)
        return len(self.areas), ncont, nwalls, nlinks, nparts, nactors

actorclasses={}            

class Player(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = 'player'
        
class Glue(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = None
actorclasses[6] = Glue

class WallMod(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = None
actorclasses[5] = WallMod

class AreaMod(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = None
actorclasses[1] = AreaMod

class Water(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = 'water'

class Decoration(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.params={}
        self.clas = 'decoration'
        self.aclass = ''
        
    def extra(self, ds):
        ds.writestring(self.aclass)
        for n, v in self.params.iteritems():
            ds.writeu8(n)
            if type(v) == str:
                ds.writeu8(5);
                ds.writestring(v)
            elif type(v) == int:
                ds.writeu8(2);
                ds.writeu32(v)
            elif type(v) == float:
                ds.writeu8(3);
                ds.writef(v)
                
                

actorclasses[10] = Water

class Node(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = 'node'

actorclasses[3] = Node
        
class Solid(Actor):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = None

    def getgroup(self):
        return self.map.solidgroups[self.hitag]

    def extra(self, ds):
        ag, exact = self.getgroup()
        ds.writeu16(ag.idx)

        boundrad = 0
        boundheight = 0

        for ar in ag.areas:
            for w in ar.walls:
                rad = math.sqrt(w.x*w.x + w.y*w.y)
                ch = abs(ar.ceil.getz(w.x,w.y))
                fh = abs(ar.floor.getz(w.x,w.y))
                if rad > boundrad: boundrad = rad
                if ch > boundheight: boundheight = ch
                if fh > boundheight: boundheight = fh
                
                
        ds.writef(boundrad+EPSILON)
        ds.writef(boundheight+EPSILON)
actorclasses[2] = Solid
        

class Debug(Solid):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = 'debug'
        #print 'debug!'

actorclasses[101] = Debug

class Platform(Solid):
    def __init__(self, cs=None):
        Actor.__init__(self,cs);
        self.clas = None

    def link(self):
        if hasattr(self,'delay'): return
        pts = []
        for ar in self.map.areas:
            for act in ar.actors:
                if type(act) == Platform and act.hitag == self.hitag:
                    pts.append(act)
        pts.sort(key=lambda a: a.lotag)
        llotag = -1

        for a in pts:
            a.clas = None
            a.delay = (a.lotag - llotag - 1) / 10.0
            llotag = a.lotag
        pts[0].clas = 'platform'
        pts[0].pts = pts
        
    def extra(self, ds):
        Solid.extra(self,ds)
        for a in self.pts:
            ds.writef(a.x,a.y,a.z,a.delay)
        
        
actorclasses[102] = Platform




def genparts(w):
    #if w.area.id != 's70x1': return
    mpx = (w.x1+w.x2)/2
    mpy = (w.y1+w.y2)/2

    def getz(oa):
        return oa.floor.getz(mpx,mpy)

    linkss = sorted(w.links.iterkeys(), key=getz)

    starts = [(w.area.floor, PlaneSpec(w.area, 1))]+\
             [(oa.ceil, PlaneSpec(oa, 0)) for oa in linkss]
    
    ends = [(oa.floor, PlaneSpec(oa, 1)) for oa in linkss]+\
             [(w.area.ceil, PlaneSpec(w.area, 0))]

    seenol = False
    if not w.olink: seenol = True

    for i, ((s, sps), (e, eps)) in enumerate(zip(starts, ends)):
        tw = w
        tflip = w.tflip
        talign = w.topalign


        if hasattr(w, 'bflip') and w.links and not seenol:
            tw = w.bw
            tflip = w.bflip
            talign = w.botalign

        if w.olink and w.olink.floor == e: seenol = True 

        sza = s.getz(w.x1, w.y1)
        szb = s.getz(w.x2, w.y2)
        eza = e.getz(w.x1, w.y1)
        ezb = e.getz(w.x2, w.y2)

        if (eza > sza or ezb > szb):
            xpan = tw.xpanning / 128.0
            ypan = tw.ypanning / 256.0
            if tw.xrepeat == 0: tw.xrepeat = 1
            if tw.yrepeat == 0: tw.yrepeat = 1
            xscl = tw.len * 16.0 / tw.xrepeat
            yscl = 16.0 / tw.yrepeat
            alignpt = talign.getplane().z
            ypan += alignpt / yscl
            if tflip&1:
                #xpan = -xpan
                xscl = -xscl
                
            if tflip&2:
                ypan = -ypan
                yscl = -yscl
            #xpan -= math.floor(xpan)
            #ypan -= math.floor(ypan)
            w.parts.append(WallPart(tw.tex, 0, xpan, ypan,
                           xscl, yscl, eps, sps, talign, w))


def sameplane(a, b):
    if a.tex != b.tex: return False
    if a.xpan != b.xpan: return False
    if a.ypan != b.ypan: return False
    if a.flags != b.flags: return False
    
    for x, y in ((0, 0), (42, 0), (0, 100)):
        if abs(a.getz(x, y)-b.getz(x, y))>0.0000001: return False

    return True
def canjoinarea(a, b):
    #return False
    if not a.canjoin: return False
    if not b.canjoin: return False
    if hasattr(a, 'replace'): return False
    if hasattr(b, 'replace'): return False

    if not sameplane(a.floor, b.floor): return False
    if not sameplane(a.ceil, b.ceil): return False

    for w in a.walls:
        for oa in w.links.keys():
            if oa == b:
                return True
    
    return False

def floodarea(area, z):
    if hasattr(area, 'noflood') and area.noflood: return
    if area.waterlevel is not None and z <= area.waterlevel:
        #print 'area %s already flooded'%(area.id)
        return
    
    #print 'flood area %s at %f'%(area.id, z)
    area.waterlevel = z
    for w in area.walls:
        for oa in w.links.keys():
            za = oa.floor.getz(w.x1, w.y1)
            zb = oa.floor.getz(w.x2, w.y2)
            if za < z or zb < z: floodarea(oa, z)

def joinareas(a, b):
    allwalls = a.walls+b.walls
    for w in allwalls:
        w.point2.point0 = w
        if hasattr(w, 'sid'):
            del w.sid
    
    for wa in a.walls:
        for wb in b.walls:
            if (wa.x1 == wb.x2 and wa.y1 == wb.y2 and
                wa.x2 == wb.x1 and wa.y2 == wb.y1 and
                wa in allwalls and wb in allwalls):
                wa.links.clear()
                wb.links.clear()
                wa.point2.point0 = wb.point0
                wa.point0.point2 = wb.point2
                wb.point2.point0 = wa.point0
                wb.point0.point2 = wa.point2
                allwalls.remove(wa)
                allwalls.remove(wb)
                
    for w in allwalls:
        w.area = a
    

    #a.id = '%sj%s'%(a.id, b.id)
    a.actors.extend(b.actors)
    a.walls = allwalls
    b.replace = a

def eachpair(l):
    for i,a in enumerate(l):
        for b in l[i+1:]:
            yield a, b


def processglue(glue):
    for k, lst in glue.items():
        if len(lst)<2:
            timer.msg('mismatched glue set %d'%k)
            continue

        for a in lst:
            for b in lst:
                if a == b: continue
                mtf = a.area.tform
                stf = b.area.tform
                stf.links.append((mtf, b.x - a.x, b.y - a.y))

class imgdict(dict):
    def __init__(self, dict_or_seq = (), default = None):
        dict.__init__(self, dict_or_seq);
        self.default = default
    def __getitem__(self, key):
        try:
            return dict.__getitem__(self, key);
        except KeyError:
            #print "warning: key %r not found"%key
            return self.default

def planeflags(f):
    of = 0
    if f&4:
        of |= 4
    if f&16:
        of |= 2
    if f&32:
        of |= 1
    if f&64:
        of |= 8
    
    return of
        

def preconvertmap(newmap, imgmap, mn, mapn):

    total = float()
    p = 0
    map = buildmap.BuildMap.from_file(join(BUILD_DIR, mapn+'.map'))
    
    timer.begin("preconvert %s"%mapn, len(map.sectors)*2 + len(map.walls) + len(map.sprites))
    
    areas = [Area(s) for s in map.sectors]
    walls = [Wall(w) for w in map.walls]
    actors = [clas(s) for clas,s in ((actorclasses.get(s.picnum,None),s) for s in map.sprites) if clas]

    for i,ar in enumerate(areas):
        timer.progress()

        ar.ceilingz /= -16384.0
        ar.floorz /= -16384.0
        ar.mapname = mapn

        ar.invert = False

        if not ar.ceilingstat&2:
            ar.ceilingheinum = 0;
            
        if not ar.floorstat&2:
            ar.floorheinum = 0;

        ar.ceilingheinum /= 4096.0
        ar.floorheinum /= 4096.0
        ar.defaultcanjoin = False
        ar.canjoin = None
        ar.tform = None
        ar.waterlevel = None

        ar.id = "m%da%d"%(mn, i)
        
        ar.walls = walls[ar.wallptr:ar.wallptr+ar.wallnum]

        for w in ar.walls:
            w.x /= 1024.0
            w.y /= -1024.0
            w.flags = 0
            w.area = ar

        ar.remove = (ar.hitag == -1)
        ar.actors = []

    for i, act in enumerate(actors):
        timer.progress()
        act.x /= 1024.0
        act.y /= -1024.0
        act.z /= -16384.0
        act.ang = ((2048-act.ang) * math.pi / 1024.0);
        #act.clas = None
        act.debugnum = i
        act.area = areas[act.sectnum]
        act.area.actors.append(act)

    if map.sectnum != 65535:
        start = Player()
        start.x = map.x/1024.0
        start.y = map.y/-1024.0
        start.z = map.z/-16384.0
        start.area = areas[map.sectnum]
        start.ang = ((2048-map.ang) * math.pi / 1024.0); 
        start.picnum = -1
        start.area.actors.insert(0,start)
    
    # Convert numeric refs to object refs, assign ids, etc.

    for i, w in enumerate(walls):
        timer.progress()
        ar = w.area
        w.point2 = walls[w.point2]

        w.links = {}
        w.olink = None
        w.blockall = False
        if w.nextsector != 65535:
            w.olink = areas[w.nextsector]
            if w.cstat&1:
                w.links[w.olink] = 2
            else:
                w.links[w.olink] = 0
        else:
            if w.cstat&1:
                w.blockall = True
            

        del w.nextsector
        
        w.jx = w.x2 - w.x1
        w.jy = w.y2 - w.y1

        w.len = math.sqrt(sqr(w.jx)+sqr(w.jy))

        w.jx /= w.len
        w.jy /= w.len

        w.parts = []

        w.tex = imgmap[w.picnum]
        w.topalign = PlaneSpec(w.area, 0)
        if w.cstat&4:
            w.topalign = PlaneSpec(w.area, 1)
            
        w.tflip = 0;

        if w.cstat&8:
            w.tflip |= 1;
        if w.cstat&256:
            w.tflip |= 2;

        w.bw = w
        
        if w.cstat&2 and w.nextwall != 65535:
            w.bw = walls[w.nextwall];

        del w.nextwall
        if w.links and not w.links.keys()[0].remove:
            w.topalign = PlaneSpec(w.links.keys()[0], 0)
            if w.cstat&4:
                w.topalign = PlaneSpec(w.area, 0)

            w.botalign = PlaneSpec(w.links.keys()[0], 1)

            if w.bw.cstat&4:
                w.botalign = PlaneSpec(w.area, 0)

            w.bflip = 0;

            if w.bw.cstat&8:
                w.bflip |= 1;
            if w.bw.cstat&256:
                w.bflip |= 2;





    for ar in areas:
        timer.progress()
        fw = ar.walls[0]

        iy = (fw.x2-fw.x1)
        ix = (fw.y1-fw.y2)

        ln = math.sqrt(ix*ix + iy*iy)
        ix /= ln;
        iy /= ln;

        scl = 2.0
        if ar.ceilingstat&8:
            scl = 1.0
        
        ar.ceil = Plane(fw.x1, fw.y1, ar.ceilingz, ix, iy, 
                        ar.ceilingheinum, imgmap[ar.ceilingpicnum], 
                        ar.ceilingxpanning/256.0, ar.ceilingypanning/256.0, 
                        scl, scl, planeflags(ar.ceilingstat))

        if not ar.ceil.slope:
            ar.ceil.ang = 0


        scl = 2.0
        if ar.floorstat&8:
            scl = 1.0

        ar.floor = Plane(fw.x1, fw.y1, ar.floorz, ix, iy,
                         ar.floorheinum, imgmap[ar.floorpicnum],
                           ar.floorxpanning/256.0, ar.floorypanning/256.0, 
                         scl, scl, planeflags(ar.floorstat))

        if not ar.floor.slope:
            ar.floor.ang = 0
        
        tforms = set()
        for w in ar.walls:
            for oa in w.links.keys():
                if oa.tform:
                    tforms.add(oa.tform)
        if tforms:
            ar.tform = ctf = tforms.pop()
            for t in tforms:
                for s2 in t.areas:
                    s2.tform = ctf
                ctf.areas.extend(t.areas)
        else:
            ar.tform = Transform()

        ar.tform.areas.append(ar)

    glue = {}
    for ar in areas:
        for a in ar.actors:
            if type(a) == Glue and a.lotag == 0:
                glue.setdefault(a.hitag, []).append(a)

    processglue(glue)
    

    newmap.areas.extend(areas)
    timer.end()



def calcvolume(world):
    totvol = 0
    totarea = 0

    timer.begin("calc volume",len(world.areas))

    for ar in world.areas:
        timer.progress()


        yp = sorted(w.y1 for w in ar.walls)

        #tess=[]
        avol = 0
        aarea = 0

        
        fl = ar.floor
        cl = ar.ceil

        fsx = fl.ix * fl.slope
        fsy = fl.iy * fl.slope
        csx = cl.ix * cl.slope
        csy = cl.iy * cl.slope

        A = (csx - fsx) / 2
        B = csy - fsy
        C = (cl.z - fl.z) + fl.x*fsx - cl.x*csx + fl.y*fsy - cl.y*csy


        #myvol = vol(A, B, C)
        
        for i in xrange(len(yp)-1):
            ya = yp[i]
            yb = yp[i+1]
            if ya == yb: continue
            ym = (ya+yb)/2
            xpa, xpb = [sorted((w.x1 - ((w.x2 - w.x1) / (w.y2 - w.y1)) * (w.y1 - y)) for w in ar.walls if (ym < w.y1) != (ym < w.y2)) for y in ya, yb]
            for i in xrange(0,len(xpa),2):
                xa1 = xpa[i]
                xa2 = xpa[i+1]
                xb1 = xpb[i]
                xb2 = xpb[i+1]

                # f(y) = my + b
                
                m1 = (xb1 - xa1) / (yb - ya)
                m2 = (xb2 - xa2) / (yb - ya)
                b1 = xa1 - m1 * ya
                b2 = xa2 - m2 * ya
                
                # f(x,y) = Ax + By + C
                
                D = m2 - m1
                E = b2 - b1
                F = A * (m1 + m2) + B
                G = A * (b1 + b2) + C
                
                #    _
                #   /  yb
                #   |
                #   |   (Hy^2 + Iy + J) dy
                #   |
                #  _/  ya
                
                
                H = D*F/3
                I = (E*F + D*G)/2
                J = E*G

                totvol += (H*(yb*yb*yb)  +  I*(yb*yb)  +  J*yb)  - \
                          (H*(ya*ya*ya)  +  I*(ya*ya)  +  J*ya)
                totarea += ((xa2 - xa1) + (xb2 - xb1))/2 * (yb - ya)


        #ar.qtess()
        #totvol += avol
        #totarea += aarea

    timer.end()

    return totvol, totarea

def calcvis(areas):

    timer.begin("calcvis",len(areas))

    abitsize = (len(areas)+7)>>3;
    input_raw, output = os.popen2("../src/vis 2> dbg")
    f = datastream(input_raw)

    for i,ar in enumerate(areas):
        #print i, ar.id
        if hasattr(ar,'contours'): continue
        ar.contours = []
        for w in ar.walls:
            cont = []
            while not hasattr(w, 'seen'):
                w.seen = True
                cont.append(w)
                w = w.point2
            if cont:
                ar.contours.append(cont)
        ar.idx = i

    f.writeu16(len(areas))
    for ar in areas:
        f.writestring(ar.id)
        f.writeu8(len(ar.contours))
        for c in ar.contours:
            f.writeu8(len(c))
            for w in c:
                f.writef(w.x1, w.y1)
                f.writeu8(len(w.links))
                for oa, flags in w.links.iteritems():
                    f.writeu16(oa.idx)
    
    f.close()

    for i,ar in enumerate(areas):
        timer.progress()
        ar.vis = output.read(abitsize)
    output.close()

    timer.end()
    
spcre = re.compile(r'\s+');
def convert(inputmaps, output, mangler):

    sttime = time.time()

    from textures import textures

    imgmap = imgdict([(num, tex) for num, (ingame, tex) in textures.iteritems() if ingame], None)

    def hook(fname, *args):
        
        if hasattr(mangler, fname):
            timer.begin("hook: %s"%fname, False)
            getattr(mangler, fname)(*args)
            timer.end()

    hook('preconvert')

    # Step 1: convert coordinates

    world = World()
    mangler.map = world

    for i, m in enumerate(inputmaps):
        preconvertmap(world, imgmap, i, m)


    # Step 5: convert actors

    hook('preconvertactors')



    iglue = {}
    gluelnk = {}

    
    timer.begin("convert actors")
    for ar in world.areas:
        if ar.hitag == 1:
            gluelnk.setdefault(ar.lotag, []).append(ar)
    
    for ar in world.areas:
        for a in ar.actors:
            a.map = world
            if a.clas: continue
            clas = None
            extra = ''
            if (a.cstat&224) == 0:
                a.z += 0.25
                
            if type(a) == Glue:
                if a.lotag == 1:
                    iglue.setdefault(a.hitag, []).append(a)
                elif a.lotag == 2:
                    gluelnk.setdefault(a.hitag, []).append(a.area)

    for ar in world.areas:
        for a in ar.actors:
            a.link()
    timer.end()

    hook('postconvertactors')


    hook('preglue')

    timer.begin('processglue')
    processglue(iglue)
    timer.end()


    areagroups=[]

    timer.begin("transform",len(world.areas))
    for ar in world.areas:
        timer.progress()
        if not ar.tform.offset:
            lst = AreaGroup()
            ar.tform.transform(lst)
            areagroups.append(lst)
    
    timer.end()

    for g in areagroups:
        for ar in g.areas:
            ar.group = g

    timer.begin("build wall index", len(world.areas))
    #for v in gluelnk.values():
    wallmap = {}
    for ar in world.areas:
        timer.progress()
        for w in ar.walls:
            wallmap.setdefault((w.x1, w.y1, w.x2, w.y2), []).append(w)
    timer.end()
    
    timer.begin("find links", len(world.areas))
    for ar in world.areas:
        timer.progress()
        for w in ar.walls:
            for w2 in wallmap.get((w.x2, w.y2, w.x1, w.y1), ()):
                if not w2.area in w.links:
                    if w.blockall or w2.blockall:
                        w.links[w2.area] = 2
                    else:
                        w.links[w2.area] = 0
                    
                        
    timer.end()
        

    hook('postglue')



    hook('preculllink')

    timer.begin("cull links",len(world.areas))

    # Step 7: eliminate impossible links
    for ar in world.areas:
        afl = ar.floor.getz
        acl = ar.ceil.getz
        timer.progress()
        for w in ar.walls:
            mfza = afl(w.x1, w.y1)
            mfzb = afl(w.x2, w.y2)
            mcza = acl(w.x1, w.y1)
            mczb = acl(w.x2, w.y2)
            wlinks = w.links
            for oa in wlinks.keys():
                ofza = oa.floor.getz(w.x1, w.y1)
                ofzb = oa.floor.getz(w.x2, w.y2)
                ocza = oa.ceil.getz(w.x1, w.y1)
                oczb = oa.ceil.getz(w.x2, w.y2)
                if ((mfza > ocza and mfzb > oczb) or
                    (mcza < ofza and mczb < ofzb)):
                    del wlinks[oa]
    timer.end()
    
    hook('postculllink')


    # Process WALLMODS and WATER
    
    timer.begin("water and wallmod")
    for ar in world.areas:
        for a in ar.actors:
            if type(a) == WallMod:
                mind = None
                minw = None
                for w in ar.walls:
                    mx = (w.x1+w.x2)/2
                    my = (w.y1+w.y2)/2
                    rx = a.x - mx
                    ry = a.y - my
                    d2w = math.sqrt(sqr(rx) + sqr(ry))
                    if mind is None or d2w < mind:
                        mind = d2w
                        minw = w
                ow = None
                for oa in minw.links.keys():
                    if a.z >= oa.floor.getz(a.x, a.y) and \
                           a.z <= oa.ceil.getz(a.x, a.y):
                        for w2 in oa.walls:
                            if w2.x1 == minw.x2 and w2.y == minw.y2 and \
                                   minw.x1 == w2.x2 and minw.y1 == w2.y2:
                                ow = w2
                                break
                        break
                
                w = minw
                if a.hitag == 1:
                    del minw.links[ow.area]
                    del ow.links[minw.area]
                elif a.hitag == 2:
                    if ow:
                        minw.links[ow.area] |= a.lotag
                        ow.links[minw.area] |= a.lotag
                elif a.hitag == 3:
                    if ow:
                        minw.links[ow.area] |= a.lotag
                        
    for ar in world.areas:
        for a in ar.actors:
            if type(a) == Water:
                a.area.wateractor = a
                if a.hitag == 0:
                    a.area.waterlevel = a.z
                    a.area.noflood = True
                    
    for ar in world.areas:
        for a in ar.actors:
            if type(a) == Water:
                a.area.wateractor = a
                if a.hitag == 1:
                    floodarea(a.area, a.z)

    dbgwater = ('water', 'flagpad', 'skidpad', 'suckpad', 'checker', 'yellowrock', 'ceiltex', 'marble1',
                'checker2', 'brick', 'dmud')

    for ar in world.areas:
        if ar.waterlevel is not None:
            if hasattr(ar,'wateractor'):
                act = ar.wateractor
            else:
                act = Water()
                ar.actors.append(act)
            act.x, act.y = ar.findpt()
            act.z = ar.waterlevel
            #act.clas = "water"
            act.area = ar
            act.ang = 0
            act.picnum = -1

            dec = Decoration();
            dec.x, dec.y, dec.z = act.x, act.y, act.z
            dec.area = ar
            dec.ang = 0
            dec.aclass = 'water'
            dec.params[0] = dbgwater[0]
            dec.params[1] = 0.5
            #if hasattr(ar, 'wdebug_trans'):
            #    dec.params[1] = ar.wdebug_trans
            ar.actors.append(dec)


    timer.end()

    for ar in world.areas:
        for w in ar.walls:
            for oa, fl in w.links.items():
                if oa.remove:
                    del w.links[oa]

    
    hook('pregenparts')

    timer.begin("genparts",len(world.areas))
    # Step 11: generate wallparts

    for ar in world.areas:
        timer.progress()
        for w in ar.walls:
            genparts(w)
    timer.end()

    hook('postconvert')

    world.solidgroups={}

    maingroup = AreaGroup()
    newgroups = [maingroup]

    for g in areagroups:
        for ar in g.areas:
            for act in ar.actors:
                if type(act) == Solid:
                    break
            else:
                continue
            break
        else:
            maingroup.areas.extend(g.areas)
            continue
        extraactors = []
        newgroups.append(g)
        world.solidgroups[act.hitag] = g, extraactors
        for ar in g.areas:
            #ar.invert = True
            ar.ceil.x -= act.x
            ar.ceil.y -= act.y
            ar.ceil.z -= act.z
            ar.floor.x -= act.x
            ar.floor.y -= act.y
            ar.floor.z -= act.z
            ar.ceil.xpan += act.x
            ar.ceil.ypan += act.y
            ar.floor.xpan += act.x
            ar.floor.ypan += act.y

            ar.ceil.xpan -= math.floor(ar.ceil.xpan)
            ar.ceil.ypan -= math.floor(ar.ceil.ypan)
            ar.floor.xpan -= math.floor(ar.floor.xpan)
            ar.floor.xpan -= math.floor(ar.floor.ypan)
            for w in ar.walls:
                w.point2.point0 = w
                w.ox = w.x1 - act.x
                w.oy = w.y1 - act.y
            for w in ar.walls:
                for p in w.parts:
                    nxp = w.len / p.xs + p.xpan
                    nxs = -p.xs

                    #if w.ox+act.x == (43008/1024.0) and w.oy+act.y == (-10240.0/-1024):
                    #    print w.len, p.xpan, p.xs, nxp, nxs

                    p.xpan = nxp
                    p.xs = nxs

                    p.ypan -= act.z / p.ys
                    p.xpan -= math.floor(p.xpan)
                    p.ypan -= math.floor(p.ypan)
                w.x = w.point2.ox
                w.y = w.point2.oy
                w.point2 = w.point0
                w.links={}
            extraactors.extend(ar.actors);
            ar.actors=[]
            
        

    for i, g in enumerate(newgroups):
        nareas = []
        for ar in g.areas:
            if not ar.remove:
                nareas.append(ar)
        g.areas = nareas

        g.idx = i
        
    # Step 13: write
    totvol = 0
    totarea = 0

    if False:
        totvol, totarea = calcvolume(world)

    #calcvis(newgroups[0].areas)

    timer.begin("write",sum(len(g.areas) for g in newgroups))


    #f = datastream(gzip.GzipFile(output+".gz", 'wb'));
    f = datastream(file(join(OUTPUT_DIR,output), 'wb'));

    f.writeu16(len(newgroups))


    stats = [0] * 6
    for g in newgroups:
        nstat = g.writeto(f)
        stats = [a + b for a, b in zip(stats, nstat)]
        
    
    f.close();
   
    timer.end();

    print "total: %f"%(time.time()-sttime)

    nareas, ncont, nwalls, nlinks, nparts, nactors = stats

    print '============== STATS ==============='
    print 'Groups:',len(newgroups)
    print 'Areas:',nareas
    print 'Walls:',nwalls
    print 'Wallparts:',nparts
    print 'Contours:',ncont
    print 'Actors:',nactors
    print 'Links:',nlinks
    if totvol:
        print
        print '2d area:',totarea
        print 'Volume:',totvol
        print 'Average area:',(totarea/len(world.areas))
        print 'Average volume:',(totvol/len(world.areas))
        
