
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
    
    for w, x, y, z in wall_list:
        nx, ny, nz = x, y, z
        if w == 0:
            nx += 1
        elif w == 1:
            ny += 1
        else:
            nz += 1

        ccell = cells[x, y, z]
        ncell = cells[nx, ny, nz]
        if ccell is ncell:
            continue

        mz[x, y, z] &= ~(1<<w)
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
    nw.shade = -1
    nw.pal = 0
    nw.picnum = 23
    nw.overpicnum = 0
    nw.xrepeat = nw.yrepeat = 8
    nw.xpanning = nw.ypanning = 0
    nw.lotag = nw.hitag = nw.extra = 0

    return nw

def createsect(pts, xo, yo, zo):
    nsect = namestruct.struct(buildmap.sectorstruct)
    nsect.xo = xo
    nsect.yo = yo
    nsect.zo = zo
    nsect.wallptr = -1
    nsect.wallnum = -1
    nsect.ceilingz = -49152
    nsect.floorz = 0
    nsect.ceilingstat = nsect.floorstat = 0
    nsect.ceilingheinum = nsect.floorheinum = 0
    nsect.ceilingpal = nsect.floorpal = 0
    nsect.floorshade = nsect.ceilingshade = -1
    nsect.ceilingxpanning = nsect.ceilingypanning = 0
    nsect.floorxpanning = nsect.floorypanning = 0
    nsect.floorpicnum = 22
    nsect.ceilingpicnum = 20
    nsect.visibility = 0
    nsect.filler = 0
    nsect.lotag = nsect.extra = 0
    nsect.hitag = 1

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
    nspr.sectnum = 0
    return nspr
    


def getwid(map,w):
    if w in map.walls:
        return map.walls.index(w)
    map.walls.append(w)
    return len(map.walls) - 1
    
def gridbase(v):
    return v * 4096 - 131072

map = buildmap.map()

map.x = map.y = gridbase(0)+1536
map.z = 1024
map.ang = 0
map.sectnum = 0
map.sectors = []
map.walls = []
map.sprites = []


xs = 5
ys = 5
zs = 5

mz = kruskal(xs,ys,zs)

cellsect={}

gb = 1024

zxo = 0
zyo = 0
for z in xrange(zs):
    zxo = z * xs
    cg = gb
    for y in xrange(ys):
        for x in xrange(xs):
            xb = gridbase(x)
            yb = gridbase(y)

            zb = - z * 65536

            s = createsect(((0,0),(3072,0),
                            (3072,3072),(0,3072),),x,y,z)

            s.floorz = zb
            s.ceilingz = zb - 49152

            
            #
            #cg += 1

            #s.sprites = [na]

            #if z == 0:
            #    na.lotag = 1
                
            na = createspr(1536,1536,zb-1536*16,3,0,0)
            s.sprites.append(na)
            
            cellsect[x,y,z] = s

            c = mz[x,y,z]

            if x < xs-1:
                s = createsect(((3072,0),(4096,0),
                                (4096,3072),(3072,3072),),x,y,z)
                s.floorz = zb
                s.ceilingz = zb - 49152
                map.sectors.append(s)
                if c&1:
                    s.hitag = 65535

            if y < ys-1:
                s = createsect(((0,3072),(3072,3072),
                                (3072,4096),(0,4096),),x,y,z)
                s.floorz = zb
                s.ceilingz = zb - 49152
                map.sectors.append(s)

                if c&2:
                    s.hitag = 65535

firstsect = [cellsect[0, 0, z] for z in xrange(zs)]


map.sectors.extend(cellsect.itervalues())

ncellsect = dict(cellsect)

for z in xrange(zs-1):
    for y in xrange(ys):
        for x in xrange(xs):
            cs = ncellsect[x, y, z]
            c = mz[x, y, z]
            
            if not c&4:
                cs2 = ncellsect[x, y, z+1]
                cs.ceilingz = cs2.ceilingz
                cs2.hitag = 65535

                #for s in cs2.sprites:
                #    s.x -= 4096 * xs

                cs.sprites.extend(cs2.sprites)
                cs2.sprites = []
                #map.sectors.remove(cs2)
                ncellsect[x, y, z+1] = cs
                
for z in xrange(zs):
    zb = - z * 65536

    na = createspr(2048,2048,zb,6,42,0)
    cellsect[0, 0, z].sprites.append(na)

    zxo = z * xs

mxs = zs*xs

while mxs > 64:
    mxs -= xs

mzo = 4096 + (zs - 1) * 65536 + 16384*3
for cs in map.sectors:
    zxo = cs.zo * xs
    zyo = 0

    cs.ceilingz += mzo
    cs.floorz += mzo
    
    while zxo >= mxs:
        zyo += ys
        zxo -= mxs
    for w in cs.walls:
        w.x += gridbase(cs.xo+zxo)
        w.y += gridbase(cs.yo+zyo)
        
    for s in cs.sprites:
        s.x += gridbase(cs.xo+zxo)
        s.y += gridbase(cs.yo+zyo)
        s.z += mzo
    cs.wallnum = len(cs.walls)
    cs.wallptr = len(map.walls)
    map.walls.extend(cs.walls)

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

    for sp in s.sprites:
        sp.sectnum = i
        map.sprites.append(sp)
    for j in xrange(s.wallnum):
        map.walls[j+s.wallptr].s = s

map.sectnum = cellsect[0, 0, 0].i
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
