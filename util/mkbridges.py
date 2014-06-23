from buildmap import *
from math import *
from genmap import plane
import sys
ostartbr = 15
startbr = 15
cbr = 0
phases = 4


EPSILON=1e-10





class v(object):
    def __init__(self,x=0,y=0,z=0):
        if isinstance(x,tuple):
            self.x=float(x[0])
            self.y=float(x[1])
            self.z=float(x[2])
        elif isinstance(x,v):
            self.x=x.x
            self.y=x.y
            self.z=x.z
        else:
            self.x=float(x); self.y=float(y); self.z=float(z);
    def __add__(self,o):
        return v(self.x+o.x,self.y+o.y,self.z+o.z)
    def __sub__(self,o):
        return v(self.x-o.x,self.y-o.y,self.z-o.z)
    def __mul__(self,o):
        if isinstance(o,v):
            return self.dot(o)
        return v(self.x*o,self.y*o,self.z*o)
    def __div__(self,s):
        return v(self.x/s,self.y/s,self.z/s)
    def __rdiv__(self,s):
        raise ArithmeticError,"cannot divide by vect"
    def __neg__(self):
        return v(-self.x,-self.y,-self.z)
    def __pos__(self):
        return v(self.x,self.y,self.z)
    def len(self):
        return sqrt(self.x*self.x+self.y*self.y+self.z*self.z)
    def __eq__(self,o):
        return abs(self.x-o.x)<EPSILON and abs(self.y-o.y)<EPSILON and abs(self.z-o.z)<EPSILON
    def __ne__(self,o):
        return not self.__eq__(o)
    __abs__=len;
    def dist(self,o):
        return abs(self-o)
    def len2(self):
        return self.x*self.x+self.y*self.y+self.z*self.z
    def dot(self,o):
        return self.x*o.x+self.y*o.y+self.z*o.z
    def cross(self,o):
        x=self.y*o.z - o.y*self.z;
        y=o.x*self.z - self.x*o.z;
        z=self.x*o.y - o.x*self.y;
        return v(x,y,z);
    def normal(self):
        l=self.len()
        return v(self.x/l,self.y/l,self.z/l)
    def __str__(self):
        return "<%.4f,%.4f,%.4f>"%(self.x,self.y,self.z);
    def __repr__(self):
        return "v(%.4f,%.4f,%.4f)"%(self.x,self.y,self.z);



def calcplane(v1,v2,v3,tex):
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

        tz = v1.z + (dir.x*(cx - v1.x) + dir.y*(cy - v1.y)) * slope

        ang = atan2(dir.y,dir.x)*180/pi
    return plane(cx,cy,tz,ang,slope,tex,0,0,'')


hmap = map('hovertest.map')

for s in hmap.sectors:
    s.bridges = []

cx = 0
cy = 0
cz = 0

planes=[]

for spr in hmap.sprites:
    sect = hmap.sectors[spr.sectnum]
    if spr.picnum == 20 and spr.hitag == 0:
        spr.cstat = 0
        spr.xrepeat = 8
        spr.yrepeat = 8
        tw = []
        sect.bridges = range(cbr,cbr+phases)
        print '#',sect.bridges
        cbr += phases
        sect.cpx = spr.x/16.0;
        sect.cpy = spr.y/-16.0;
    if spr.picnum == 20 and spr.hitag == 42:
        cx = spr.x/16.0;
        cy = spr.y/-16.0;
        cz = spr.z/-256.0
cz = -32
yscale=160*3/4
print '#',cx,cy,cz


def zfunc(x,y,cpx,cpy,phase):
    cang = atan2(cpy,cpx)+(phase*2*pi)
    ang = atan2(y,x)
    while ang < cang-pi: ang += pi*2
    while ang > cang+pi: ang -= pi*2

    dist=0 #sqrt(x*x+y*y)/4
    
    return yscale+ang*yscale/pi+dist

for i,s in enumerate(hmap.sectors):
    for ph,bn in enumerate(s.bridges):
        tw = []
        for i in xrange(3):
            cw = hmap.walls[s.wallptr+i]
            tx = cw.x/16.0
            ty = cw.y/-16.0
            tz = zfunc(tx-cx,ty-cy,s.cpx-cx,s.cpy-cy,ph)+cz
            tw.append(v(tx,ty,tz))
        planes.append((i,bn)+tuple(tw))

zeroplane=plane(0,0,0,0,0,0,0,0,'')
bridges=[(zeroplane,zeroplane)]*cbr

for i,b,v1,v2,v3 in planes:
    pl = calcplane(v1,v2,v3,0)
    pl2 = pl.copy()
    pl2.z -= 16
    pl.tex = 81
    pl2.tex = 20
    bridges[b] = (pl,pl2)
print 'from genmap import plane, bridge'
print 'phases=%d'%phases
print 'tbr = ['
for top,bot in bridges:
    print '    bridge(%r,'%top
    print '           %r),'%bot
print ']'
print '# top',cz+yscale*(phases)*2

osprites=hmap.sprites
hmap.sprites = []
for spr in osprites:
    sect = hmap.sectors[spr.sectnum]
    if not (spr.picnum == 2 and spr.hitag >= ostartbr and sect.bridges):
        hmap.sprites.append(spr);
        
    if spr.picnum == 20 and spr.hitag == 0:
        spr.cstat = 0
        ox = spr.x
        for bn in sect.bridges:
            spr = spr.copy()
            spr.picnum = 2
            spr.hitag = bn + startbr
            spr.x = ox+64
            spr.xrepeat = 32
            spr.yrepeat = 32

            cbr += 1
            hmap.sprites.append(spr);
        

hmap.writeto('hovertest.map')
