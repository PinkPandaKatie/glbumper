import sys
import math
import genmap


def sqr(val):
    return val * val

EPSILON = 1e-10

def fixplane(map, tag, mult, psel, inv=False):
    circs = {}

    for ar in map.areas:
        for a in ar.actors:
            if type(a) == genmap.AreaMod and a.lotag == tag:
                circs[a.hitag] = a
    runs = []
    crun = []
    lk = None

    for k in sorted(circs.iterkeys()):
        if lk != k - 1:
            if crun:
                runs.append(crun)
            crun = []
        lk = k
        crun.append(circs[k])
    if crun:
        runs.append(crun)

    for r in runs:
        len = sdist(r[0], r[-1])
        for i, p in enumerate(r[1:]):
            ln = sdist(p, r[0])
            ln2 = sdist(p, r[i])
            pl = getattr(p.area, psel)
            pl.x, pl.y = p.x, p.y
            if inv:
                pl.z = (r[0].z - mult * len)
            else:
                pl.z = (r[0].z + mult *
                        (math.sqrt((len * len) - (ln * ln)) - len))
            p.z = pl.z
            ix = (p.x - r[i].x) / ln2
            iy = (p.y - r[i].y) / ln2
            slope = (p.z - r[i].z) / ln2

            pl.ix = ix
            pl.iy = iy
            pl.ixs = ix*slope
            pl.iys = iy*slope
            pl.slope = slope
            


def copyplane(map, tag, sel):
    master = {}
    for ar in map.areas:
        for a in ar.actors:
            if type(a) == genmap.AreaMod and a.lotag == tag:
                master[a.hitag] = a
                
    for ar in map.areas:
        for a in ar.actors:
            if type(a) == genmap.AreaMod and a.lotag == tag + 1:
                if a.hitag in master:
                    pl = getattr(master[a.hitag].area, sel)
                    setattr(a.area, sel, pl)

def sdist(a, b):
    return math.sqrt(sqr(a.x-b.x) + sqr(a.y-b.y))


class Vect3d(object):
    def __init__(self, x=0, y=0, z=0):
        if isinstance(x, tuple):
            self.x = float(x[0])
            self.y = float(x[1])
            self.z = float(x[2])
        elif isinstance(x, Vect3d):
            self.x = x.x
            self.y = x.y
            self.z = x.z
        else:
            self.x = float(x)
            self.y = float(y)
            self.z = float(z)
            
    def __add__(self, o):
        return Vect3d(self.x + o.x, self.y + o.y, self.z + o.z)
    
    def __sub__(self, o):
        return Vect3d(self.x-o.x, self.y-o.y, self.z-o.z)
    
    def __mul__(self, o):
        if isinstance(o, Vect3d):
            return self.dot(o)
        return Vect3d(self.x*o, self.y*o, self.z*o)
    
    def __div__(self, s):
        return Vect3d(self.x / s, self.y / s, self.z / s)
    
    def __rdiv__(self, s):
        raise ArithmeticError("cannot divide by vect")
    
    def __neg__(self):
        return Vect3d(-self.x, -self.y, -self.z)
    
    def __pos__(self):
        return Vect3d(self.x, self.y, self.z)
    
    def len(self):
        return math.sqrt(self.x*self.x + self.y*self.y + self.z*self.z)
    
    def __eq__(self, o):
        return abs(self.x-o.x)<EPSILON and abs(self.y-o.y)<EPSILON and abs(self.z-o.z)<EPSILON
    
    def __ne__(self, o):
        return not self.__eq__(o)
    
    __abs__ = len;
    
    def dist(self, o):
        return abs(self-o)
    
    def len2(self):
        return self.x*self.x + self.y*self.y + self.z*self.z
    
    def dot(self, o):
        return self.x*o.x + self.y*o.y + self.z*o.z
    
    def cross(self, o):
        x = self.y*o.z - o.y*self.z
        y = o.x*self.z - self.x*o.z
        z = self.x*o.y - o.x*self.y
        return Vect3d(x, y, z)
    
    def normal(self):
        l = self.len()
        return Vect3d(self.x / l, self.y / l, self.z / l)
    
    def __str__(self):
        return "<%.4f, %.4f, %.4f>" % (self.x, self.y, self.z);
    
    def __repr__(self):
        return "Vect3d(%.4f, %.4f, %.4f)" % (self.x, self.y, self.z);

def plane_above(a, b, pts):
    """Returns True if plane a is above plane b for all points in pts."""

    agetz = a.getz
    bgetz = b.getz
    b_equal_a = True
    for x, y in pts:
        za = agetz(x, y)
        zb = bgetz(x, y)
        if not (za == zb):
            b_equal_a = False
        if not (zb >= za):
            return False
    return not b_equal_a

def plane_between(pl, a, b, pts):
    pl_above_a = False
    pl_below_b = False
    for x, y in pts:
        za = a.getz(x, y)
        zb = b.getz(x, y)
        zp = pl.getz(x, y)
        if (zp > za):
            pl_above_a = True
            if pl_below_b:
                return True
        if (zp < zb):
            pl_below_b = True
            if pl_above_a:
                return True
    return False
    

def splitarea(map, ar, floors, ceils):

    ret = []
    pts = [(w.x, w.y) for w in ar.walls]

    floors.append(ar.floor)
    ceils.append(ar.ceil)
    tid = 0
    fc = floors + ceils

    for a in floors:
        for b in ceils:
            if not plane_above(a, b, pts):
                continue
            can_create = True
            for c in fc:
                if c == a or c == b:
                    continue

                if plane_between(c, a, b, pts):
                    break

            else:
                na = genmap.Area(ar)
                if a == ar.floor:
                    ar.floorreplace = na
                    na.isbottom = True
                if b == ar.ceil:
                    ar.ceilreplace = na
                    na.istop = True
                na.floor = a
                na.ceil = b
                ret.append(na)
    if not ret:
        print 'could not split area %s!' % ar.id
        return [ar]
    if not hasattr(ar, 'ceilreplace'):
        ar.ceilreplace = ret[0]

    if not hasattr(ar, 'floorreplace'):
        ar.floorreplace = ret[0]
            
    for i, w in enumerate(ar.walls):
        w.sid = i
        
    for tid, nar in enumerate(ret):
        nar.id = "%sx%d" % (ar.id, tid)
        nar.actors = []
        for a in list(ar.actors):
            fz = nar.floor.getz(a.x, a.y)
            cz = nar.ceil.getz(a.x, a.y)
            if a.z >= fz and a.z <= cz:
                a.area = nar
                nar.actors.append(a)
                ar.actors.remove(a)
        
        nar.walls = []
        for i, w in enumerate(ar.walls):
            nw = genmap.Wall(w)
            nw.nextid = w.point2.sid
            nw.area = nar
            nw.links = dict(w.links)
            nw.parts = []
            nar.walls.append(nw)
            
        for w in nar.walls:
            w.point2 = nar.walls[w.nextid]
            
        if hasattr(nar, 'floorreplace'):
            del nar.floorreplace
        if hasattr(nar, 'ceilreplace'):
            del nar.ceilreplace
    
    for a in ar.actors:
        if a.clas:
            print 'Warning: sprite %d (%s, %d, %d) stuck on area %s!' % \
                  (a.debugnum, a.clas, a.x*16, a.y*-16, area.id)
        
    for w1 in ar.walls:
        for oa in w1.links.iterkeys():
            for w2 in oa.walls:
                if ar in w2.links:
                    flags = w2.links[ar]
                    del w2.links[ar]
                    for a2 in ret:
                        w2.links[a2] = flags
    ar.group.areas.remove(ar)
    ar.group.areas.extend(ret)
    return ret


def calcplane(cx, cy, cz, v1, v2, v3, sp):
    r1 = v2 - v1
    r2 = v3 - v1
    norm = r1.cross(r2)
    norm.z = 0
    slope = 0
    ang = 0;
    tz = v1.z
    if abs(norm) != 0:
        dir = norm.normal()
        d1 = r1 * dir
        d2 = r2 * dir
        if abs(d2) > abs(d1):
            slope = r2.z / d2
        else:
            slope = r1.z / d1

        tz = v1.z + (dir.x * (cx - v1.x) + dir.y * (cy - v1.y)) * slope

    return genmap.Plane(cx, cy, tz, dir.x, dir.y, slope, sp.tex, sp.xpan, sp.ypan, sp.xscl, sp.yscl, sp.flags)




phases = 4
yscale = 160*3 / 4 / 64.0

def spiral(x, y, cpx, cpy, phase):
    cang = math.atan2(cpy, cpx) + (phase * 2 * math.pi)
    ang = math.atan2(y, x)
    while ang < cang - math.pi: ang += math.pi * 2
    while ang > cang + math.pi: ang -= math.pi * 2
    
    dist = 0
    
    return yscale + ang * yscale / math.pi + dist

def addf(func,add):
    def ret(*args):
        return func(*args)+add
    return ret
    
def mulf(func,mul):
    def ret(*args):
        return func(*args)*mul
    return ret

def bridgehmap(map,areas,tag,tzfunc,bzfunc,phases):
    cz = None
    areas=list(areas)
    for a in areas:
        for spr in a.actors:
            if type(spr) == genmap.AreaMod and spr.lotag == tag and spr.hitag == 1:
                cx = spr.x;
                cy = spr.y;
                cz = spr.z;
                break
            
    if cz is None:
        return
    
    for s in areas:
        floors = []
        ceils = []
        cpx = 0
        cpy = 0
        for sp in s.actors:
            if type(sp) == genmap.AreaMod and sp.lotag == tag and sp.hitag == 0:
                cpx = sp.x
                cpy = sp.y
                break
        else:
            continue


        if len(s.walls) != 3:
            print 'barf!'
            continue

        for phase in xrange(phases):
            bpts = []
            tpts = []
            for w in s.walls:
                bpts.append(Vect3d(w.x, w.y, bzfunc(
                    w.x - cx, w.y - cy, cpx - cx, cpy - cy, phase) + cz))
                tpts.append(Vect3d(w.x, w.y, tzfunc(
                    w.x - cx, w.y - cy, cpx - cx, cpy - cy, phase) + cz))

            tpl = calcplane(cx, cy, cz, tpts[0], tpts[1], tpts[2], s.floor)
            bpl = calcplane(cx, cy, cz, bpts[0], bpts[1], bpts[2], s.ceil)

            floors.append(tpl)
            ceils.append(bpl)

        map.areas.remove(s)
        map.areas.extend(splitarea(map, s, floors, ceils))




class Hooks:
    def __init__(self):
        self.map = None
        
    def preconvertactors(self):
        for ar in self.map.areas:
            for a in ar.actors:
                if type(a) == genmap.AreaMod and a.lotag == 0 and a.hitag == 0:
                    md = None
                    ma = None
                    nal = [ar]
                    for w in ar.walls:
                        for oa in w.links.iterkeys():
                            if not oa in nal: nal.append(oa)
                    for nar in nal:
                        for na in nar.actors:
                            if na == a: continue
                            distsq = sqr(na.x-a.x) + sqr(na.y-a.y)
                            if md is None or distsq<md:
                                md = distsq
                                ma = na
                    if ma:
                        #print 'move', ma.x, ma.y, ma.z, 'to', a.x, a.y, a.z
                        ma.x, ma.y, ma.z = a.x, a.y, a.z
                    

        
    def postglue(self):


        copyplane(self.map, 30, 'ceil')
        copyplane(self.map, 32, 'floor')


        
        fixplane(self.map, 42, 1, 'floor')
        fixplane(self.map, 43, -1, 'ceil')
        fixplane(self.map, 44, -1, 'floor')
        fixplane(self.map, 45, 1, 'ceil')


                    
        bridgehmap(self.map,self.map.areas,100,spiral,addf(spiral,-16 / 64.0),4)
        #bridgehmap(self.map,self.map.areas,101,mulf(spiral,-1),addf(mulf(spiral,-1),32*4),1,"checker.png","ceiltex.png")
        
    
    def pregenparts(self):
        nseekers = 0
        nplayerdel = 0
        for ar in self.map.areas:
            for a in list(ar.actors):
                #if a.clas == 'node':
                #    ar.actors.remove(a)
                
                if a.clas == 'seeker':
                    if nseekers:
                        nseekers -= 1
                    else:
                        ar.actors.remove(a)
                        
                if a.clas == 'player':
                    if ar.mapname != 'demo05':
                        ar.actors.remove(a)
        for ar in self.map.areas:
            for w in ar.walls:
                if (w.hitag == 10 or w.hitag == 12) and w.links:
                    ofz = w.links.iterkeys().next().floor.getz(w.x1, w.y1)
                    fl = ar.floor
                    fl.slope = (ofz - fl.z) / ((w.x1 - fl.x)*fl.ix + (w.y1 - fl.y)*fl.iy)
                    
                if (w.hitag == 11 or w.hitag == 12) and w.links:
                    ocz = w.links.iterkeys().next().ceil.getz(w.x1, w.y1)
                    ce = ar.ceil
                    ce.slope = (ocz - ce.z) / ((w.x1 - ce.x)*ce.ix + (w.y1 - ce.y)*ce.iy)

        
dmaps=(0,1,2,3,4,5,6,7,8,9,10,11,12,13)
#dmaps=(0,1,2,3,4,5,6,7,8,9,10,11,12,13)
genmap.convert(['demo%02d'%mn for mn in dmaps], 'demo.world', Hooks())
#genmap.convert(['demo5'], 'demo.world', Hooks())

