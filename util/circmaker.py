
import sys
import buildmap
import namestruct
import math

map = buildmap.map()

def circ(cx, cy, rad, npts, dir = 1):
    mul =(math.pi*2)/npts * dir
    for n in xrange(npts):
        yield (cx + int(math.cos(mul * n) * rad), cy + int(math.sin(mul * n) * rad))

def rotpts(cx, cy, ang, pts):
    cosa = math.cos(ang)
    sina = math.sin(ang)
    for x, y in pts:
        rx = x - cx
        ry = y - cy
        nrx = rx * cosa + ry * sina
        nry = ry * cosa - rx * sina
        yield nrx + cx, nry + cy 

class Sect(object):
    def __init__(self):
        self.polys = []

mnpts = 60
circ1 = list(circ(0, 0, 2048 * 6, mnpts / 2))
circ2 = list(circ(0, 0, 2048 * 6 - 256, mnpts))
circ3 = list(circ(0, 0, 2048 * 6 - 384, mnpts))

bigsz = 2048*7

big= [ (-bigsz, -bigsz), (+bigsz, -bigsz), (+bigsz, +bigsz), (-bigsz, +bigsz) ]

rcx = 2048 * 6 - 330
rr = 24
rpts = [ (rcx - rr, - rr), (rcx - rr, + rr), (rcx + rr, + rr), (rcx + rr, - rr) ]
rail = [circ2, reversed(circ3)]
print rpts, list(rotpts(0, 0, 0, rpts))

npts = mnpts * 2
mul = (math.pi*2)/npts
for n in xrange(npts):
    pass
    #rail.append(list(rotpts(0, 0, n * mul, rpts)))


sects = [
    [circ1]
    #[ big, reversed(circ1) ],
    #[circ3], #inner
    #rail, #rail
    #[circ1, reversed(circ2)] #outer
    ]

cw = 0
cs = 0

for s in sects:
    sec = namestruct.struct(buildmap.sectorstruct);
    
    sec.wallptr = cw
    sec.ceilingz = 114688
    sec.floorz = 155648
    sec.ceilingstat = 0
    sec.floorstat = 0
    sec.ceilingpicnum = 20
    sec.floorpicnum = 28
    sec.ceilingshade = -1
    sec.floorshade = -1
    sec.ceilingpal = 0
    sec.ceilingxpanning = 0
    sec.ceilingypanning = 0
    sec.floorxpanning = 0
    sec.floorypanning = 0
    sec.floorheinum = 0
    sec.ceilingheinum = 0
    sec.floorpal = 0
    sec.lotag = 0
    sec.hitag = 1
    sec.filler = 0
    sec.extra = 0
    sec.visibility = -1
    

    for p in s:
        cbegin = cw
        for x, y in p:
            wal = namestruct.struct(buildmap.wallstruct);
            wal.x = x
            wal.y = y
            wal.point2 = cw + 1
            wal.nextwall = 65535
            wal.nextsector = 65535
            wal.cstat = 0
            wal.picnum = 28
            wal.overpicnum = 0
            wal.shade = -1
            wal.pal = 0
            wal.xrepeat = 2
            wal.yrepeat = 64
            wal.xpanning = 0
            wal.ypanning = 0
            wal.lotag = 0
            wal.hitag = 0
            wal.extra = 0
            map.walls.append(wal)
            cw += 1
        map.walls[-1].point2 = cbegin

    sec.wallnum = cw - sec.wallptr

    map.sectors.append(sec)
    cs += 1

for i,s in enumerate(map.sectors):
    s.i = i
    
    for j in xrange(s.wallnum):
        map.walls[j+s.wallptr].s = s

walldct={}

for i,w in enumerate(map.walls):
    w.i=i
    w.nextsector = w.nextwall = 65535
    w.n=map.walls[w.point2]
    walldct[(w.x,w.y,w.n.x,w.n.y)] = w

for w in map.walls:
    ow=walldct.get((w.n.x,w.n.y,w.x,w.y),None)
    if ow:
        w.nextwall = ow.i
        w.nextsector = ow.s.i

map.writeto('../editor/circ.map')

