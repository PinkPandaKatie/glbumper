
import sys
import os
import math
import random

import buildmap
import namestruct


M_X = 1
M_Y = 2
M_Z = 4

def kruskal(w,h,d=1):
    mz = {}
    cells = {}
    for x in xrange(w):
        for y in xrange(h):
            for z in xrange(d):
                mz[x, y, z] = 7
                cells[x, y, z] = [(x, y, z)]
    
    wall_list = []

    n = 0
    for k in xrange(d):
        for j in xrange(h):
            for i in xrange(w):
                if i < w - 1:
                    wall_list.append((0, i, j, k))
                if j < h - 1:
                    wall_list.append((1, i, j, k))
                if k < d - 1:
                    wall_list.append((2, i, j, k))
                n += 1
                
    random.shuffle(wall_list)
    
    for ww, x, y, z in wall_list:
        nx, ny, nz = x, y, z
        if ww == 0:
            nx += 1
        elif ww == 1:
            ny += 1
        else:
            nz += 1

        ccell = cells[x, y, z]
        ncell = cells[nx, ny, nz]
        if ccell is ncell:
            continue

        mz[x, y, z] &= ~(1<<ww)
        for pt in ncell:
            cells[pt] = ccell
            
        ccell.extend(ncell)
    return mz
    

def createwall(pt):
    nw = namestruct.struct(buildmap.wallstruct)
    nw.x, nw.y = pt
    nw.point2 = -1
    nw.nextwall = -1
    nw.nextsector = -1
    nw.cstat = 0
    nw.shade = 0
    nw.pal = 0
    nw.picnum = 23
    nw.overpicnum = 0
    nw.xrepeat = nw.yrepeat = 8
    nw.xpanning = nw.ypanning = 0
    nw.lotag = nw.hitag = nw.extra = 0

    return nw

def createsect(pts):
    nsect = namestruct.struct(buildmap.sectorstruct)
    nsect.wallptr = -1
    nsect.wallnum = -1
    nsect.ceilingz = -32768
    nsect.floorz = 0
    nsect.ceilingstat = nsect.floorstat = 0
    nsect.ceilingheinum = nsect.floorheinum = 0
    nsect.ceilingpal = nsect.floorpal = 0
    nsect.floorshade = nsect.ceilingshade = 0
    nsect.ceilingxpanning = nsect.ceilingypanning = 0
    nsect.floorxpanning = nsect.floorypanning = 0
    nsect.floorpicnum = 22
    nsect.ceilingpicnum = 20
    nsect.visibility = 0
    nsect.filler = 0
    nsect.lotag = nsect.hitag = nsect.extra = 0

    nsect.walls = [createwall(pt) for pt in pts]

    for w1,w2 in zip(nsect.walls[:-1],nsect.walls[1:]):
        w1.p2 = w2
    nsect.walls[-1].p2 = nsect.walls[0]

    nsect.sprites = []

    return nsect

def createspr(x,y,z,pn,ht,lt):
    nspr = namestruct.struct(buildmap.spritestruct)
    nspr.x = x
    nspr.y = y
    nspr.z = z
    nspr.cstat = 0
    nspr.picnum = pn
    nspr.shade = nspr.pal = nspr.clipdist = nspr.filler = 0
    nspr.xrepeat = nspr.yrepeat = 32
    nspr.xoffset = nspr.yoffset = 0
    nspr.statnum = nspr.ang = nspr.owner = 0
    nspr.xvel = nspr.yvel = nspr.zvel = 0
    nspr.hitag = ht
    nspr.lotag = lt
    nspr.extra = 0
    return nspr
    


def getwid(map,w):
    if w in map.walls:
        return map.walls.index(w)
    map.walls.append(w)
    return len(map.walls) - 1
    
def gridbase(v):
    return v * (5 * 1024) - 131072

map = buildmap.map()

map.x = map.y = gridbase(0)+1536
map.z = 1024
map.ang = 0
map.sectnum = 0
map.sectors = []
map.walls = []
map.sprites = []


xs = 9
ys = 9

mz = kruskal(xs,ys,1)

cellsect={}

for y in xrange(ys):
    for x in xrange(xs):
        xb = gridbase(x)
        yb = gridbase(y)

        c = mz[x,y,0] & 3

        a = 4
        b = 5

        lx = (x == 0 or mz[x - 1, y, 0] & 1)
        ly = (y == 0 or mz[x, y - 1, 0] & 2)

        if c==0:
            pts = [(0, 0), (a, 0), (b, 0), (b, a), (a, a), (a, b), (0, b), (0, a)]
            if lx:
                pts.remove((0, a))
            if ly:
                pts.remove((a, 0))
        elif c == 1:
            pts = [(0, 0), (a, 0), (a, b), (0, b), (0, a)]
            if lx:
                pts.remove((0, a))
        elif c == 2:
            pts = [(0, 0), (a, 0), (b, 0), (b, a), (0, a)]
            if ly:
                pts.remove((a, 0))
        elif c == 3:
            pts = [(0, 0), (a, 0), (a, a), (0, a)]

        #if c != 3 and (x == 0 or mz[x - 1, y, 0] & 1):
        #    pts.remove((0, a))
        #if c != 3 and (y == 0 or mz[x, y - 1, 0] & 2):
        #    pts.remove((a, 0))
                       
        s = createsect([(xb+xx*1024, yb+yy*1024) for xx, yy in pts])

        map.sectors.append(s)

        cellsect[x,y,0] = s



for s in map.sectors:
    s.wallnum = len(s.walls)
    s.wallptr = len(map.walls)
    map.walls.extend(s.walls)

for i,w in enumerate(map.walls):
    w.id = i

for w in map.walls:
    dx = w.p2.x - w.x
    dy = w.p2.y - w.y
    
    ln = math.sqrt(dx * dx + dy * dy)
    w.xrepeat = int(ln / 128.0)
    w.point2 = w.p2.id






for i,s in enumerate(map.sectors):
    s.i = i
    s.hitag = 1
    for sp in s.sprites:
        sp.sectnum = i
        map.sprites.append(sp)
    
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


map.writeto(sys.argv[1])
