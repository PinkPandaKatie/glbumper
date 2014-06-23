from struct import *
import Image,sys,os
from cStringIO import StringIO as sio
f=file(sys.argv[1],'r')
basen = os.path.splitext(sys.argv[1])[0]

dum=f.read(8)

classnames={}
def readcn():
    cntemp,=unpack("<H",f.read(2));
    if cntemp==65535:
        cnum,=unpack("<H",f.read(2))
        cnlen,=unpack("<H",f.read(2))
        cn=f.read(cnlen)
        classnames[cnum] = cn

        return cn
    if not cntemp: return None

    return classnames[cntemp&0x7FFF]


def readid():
    tlen=ord(f.read(1))
    return f.read(tlen)

statics=[]

def doclass_CMerlinStatic():
    spos=f.tell()
    ids=[readid() for i in xrange(2)]
    ints = unpack("<10B",f.read(10));
    ids+=[readid() for i in xrange(7)]
    ints += unpack("<18b",f.read(18));
    statics.append((ids,ints))
    dum,=unpack("<H",f.read(2));
    if dum < 32768 and dum != 0: f.seek(-2,1)
    #print 'CMerlinStatic',(i0,i1,i2,i3,i4,ids)
    
def doclass_CMerlinLocation():
    spos=f.tell()
    id=readid();
    ints = [unpack("<H",f.read(2))[0] for i in xrange(6)]
    dum,=unpack("<H",f.read(2));
    if dum < 32768 and dum != 0: f.seek(-2,1)
    #print 'CMerlinLocation',(id,ints)


lines = []
def doclass_CMerlinBSP():
    dum = f.read(1)
    ints = unpack("<20h",f.read(40))
    lines.append(ints)
    #print ints[5:20]
    #print (''.join(['%5d '%c for c in ints]))
    dum = f.read(2)
    

while True:
    spos=f.tell()
    dat = f.read(8)
    if not dat: break
    ni=0
    
    numitems,dum1,dum2,cnlen=unpack("<HHHH",dat);
    print numitems,dum1,dum2,cnlen
    clas=f.read(cnlen)
    func=globals().get('doclass_'+clas,None)
    if not func:
        print spos,"don't know how to read %r"%clas
        break
    for i in xrange(numitems):
        func()

#print f.tell()


class line:
    def __init__(self,i,x1,y1,x2,y2,i1,i2,i3,fi,bi):
        self.i = i
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        self.i1 = i1
        self.i2 = i2
        self.i3 = i3
        self.fi = fi
        self.bi = bi
        self.f = None
        self.b = None
        self.s1 = None
        self.s2 = None
        self.m1=[]
        self.m2=[]
    def visit(self,vproc,l=0):
        vproc(self,l)
        if self.f: self.f.visit(vproc,l+1)
        if self.b: self.b.visit(vproc,l+1)

tree = None
lins = []
for i,l in enumerate(lines):
    x1,y1,x2,y2 = l[1:5]
    nl = line(*((i,)+l[1:10]))
    lins.append(nl)

linmap={}

for i,a in enumerate(lins):
    v = linmap.get((a.x1,a.y1,a.x2,a.y2),0)
    linmap[a.x1,a.y1,a.x2,a.y2] = a
    a.ol = None
    for b in lins[i+1:]:
        if a.x1 == b.x1 and a.y1 == b.y1:
            a.m1.append(b)
            b.m1.append(a)
            
        if a.x1 == b.x2 and a.y1 == b.y2:
            a.m1.append(b)
            b.m2.append(a)

        if a.x2 == b.x1 and a.y2 == b.y1:
            a.m2.append(b)
            b.m1.append(a)
            
        if a.x2 == b.x2 and a.y2 == b.y2:
            a.m2.append(b)
            b.m2.append(a)


tree = lins[0]

for l in lins:

    l.ol = linmap.get((l.x2,l.y2,l.x1,l.y1),None)
    
    if l.fi != -1:
        l.f = lins[l.fi]

    if l.bi != -1:
        l.b = lins[l.bi]

    l.s1 = statics[l.i2]
        
        
    l.visited = 0

def doprint(lin,lev):
    lin.visited += 1
    #print ' '.join([repr(i) for i in (lin.i,lin.x1, lin.y1, lin.x2, lin.y2, l.ol and l.ol.i)])

tree.visit(doprint)

import Image, ImageDraw, math
w, h = 768, 768

scl = 1/32.0

if len(sys.argv)==3:
    img = Image.new("RGB",(w,h))

    draw = ImageDraw.Draw(img)

    draw.rectangle((0,0,w,h),fill=(0,0,0))

    cols = [(255,255,255),(255,0,0),(0,255,0),(0,0,255)]
    for l in lins:
        #if not len(l.m1) and not len(l.m2): continue 
        cc = cols[0]
        s1 = l.s1

        if s1[1][18]:
            continue
        #print ' '.join(['%3d '%i for i in s1[1]])
        #if not s1[1][-17]:
        #    cc = cols[1]
        draw.line((w-l.x1*scl,h-l.y1*scl,w-l.x2*scl,h-l.y2*scl),fill=cc)

    #draw.line((0,0,100,100),fill=(1,1,1))


    img.save(sys.argv[2])

else:
    pts = []
    def addspr(x,y):
        if not (x,y) in pts: pts.append((x,y))
        
    import namestruct,buildmap,sys
    map=buildmap.map(sys.argv[2])

    nspr = []
    for s in map.sprites:
        if s.picnum != 1: nspr.append(s)
    map.sprites=nspr

    for l in lins:
        #if l.s1[1][18]:
        #    continue
        ln = math.sqrt((l.x2-l.x1)*(l.x2-l.x1) + (l.y2-l.y1)*(l.y2-l.y1))
        numspr = int(ln/200)
        if numspr < 3: numspr = 3

        for i in xrange(numspr):
            f = (float(i)/float(numspr-1))
            tx = int((l.x2 - l.x1)* f + l.x1)
            ty = int((l.y2 - l.y1)* f + l.y1)
            addspr(tx,ty)

    for x,y in pts:
        nspr = namestruct.struct(buildmap.spritestruct)
        nspr.x = (x) * -8 + 131072
        nspr.y = (y) * -8 + 131072
        nspr.z = 0
        nspr.cstat = 0
        nspr.picnum = 1
        nspr.shade = nspr.pal = nspr.clipdist = nspr.filler = 0
        nspr.xrepeat = nspr.yrepeat = 8
        nspr.xoffset = nspr.yoffset = 0
        nspr.sectnum = nspr.statnum = nspr.ang = nspr.owner = 0
        nspr.xvel = nspr.yvel = nspr.zvel = nspr.hitag = nspr.lotag = nspr.extra = 0
        map.sprites.append(nspr)
    print len(map.sprites)
    map.writeto(sys.argv[3])
