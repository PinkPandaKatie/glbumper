import math
import struct

def sqr(x):
    return x*x

def _bin_rw(fmt):
    fmt = ">%s"%fmt
    l = struct.calcsize(fmt)
    pak = struct.pack
    upk = struct.unpack
    def read(self):
        return upk(fmt, self.read(l))[0]
    def write(self, *v):
        for t in v:
            self.write(pak(fmt, t))
    return read, write

class datastream(object):
    def __init__(self,f):
        self.read = f.read
        self.write = f.write
        self.close = f.close
        self.f = f

    readu8, writeu8 = _bin_rw('B')
    readu16, writeu16 = _bin_rw('H')
    readu32, writeu32 = _bin_rw('I')
    reads8, writes8 = _bin_rw('b')
    reads16, writes16 = _bin_rw('h')
    reads32, writes32 = _bin_rw('i')
    readf, writef = _bin_rw('d')

    def writestring(self, str, usetbl=False):
        self.writeu16(len(str))
        self.write(str)
        
    def readstring(self):
        ln = self.readu16()
        return self.read(ln)

class Plane(object):
    def __init__(self, x, y, z, ix, iy, slope, tex, xpan, ypan, xscl, yscl, flags):
        self.x = x
        self.y = y
        self.z = z
        self.ix = ix
        self.iy = iy
        self.ixs = ix*slope
        self.iys = iy*slope
        self.slope = slope
        self.tex = tex
        self.xpan = xpan
        self.ypan = ypan
        self.xscl = xscl
        self.yscl = yscl
        self.flags = flags

    def copy(self):
        return Plane(self.x, self.y, self.z, self.ix, self.iy, self.slope, self.tex, 
                     self.xpan, self.ypan, self.xscl, self.yscl, self.flags)

    def writeto(self, f):
        f.writef(self.x, self.y, self.z, self.ix, self.iy, self.slope, self.xpan, self.ypan, self.xscl, self.yscl)
        f.writeu16(self.flags)
        f.writestring(self.tex or '')

    def getz(self, x, y):
        return self.z + ((x - self.x) * self.ixs +
                         (y - self.y) * self.iys)

    def transform(self, xo, yo):
        self.x += xo
        self.y += yo
        
        if self.flags & 8:
            return

        if self.flags & 4:
            xo, yo = yo, xo

        if self.flags & 2:
            xo = -xo

        if self.flags & 1:
            yo = -yo

        xo /= self.xscl
        yo /= self.yscl

        self.xpan += xo
        self.ypan += yo
        

class PlaneSpec(object):
    def __init__(self, area, floor):
        self.area = area
        self.floor = floor
        
    def getplane(self):
        if self.floor: return self.area.floor
        return self.area.ceil

        
class WallPart(object):
    def __init__(self, tex, flags, xpan, ypan, xs, ys, top, bot, align, w):
        self.tex = tex
        self.flags = flags
        self.xpan = xpan
        self.ypan = ypan
        self.xs = xs
        self.ys = ys
        self.top = top
        self.bot = bot
        self.w = w

    def copy(self):
        return WallPart(self.tex, self.flags, self.xpan, self.ypan, self.xs, 
                        self.ys, self.top, self.bot, self.align)

    def writeto(self, f):
        f.writestring(self.tex)
        f.writeu8(self.flags)
        f.writef(self.xpan, self.ypan, self.xs, self.ys)
        bot = self.bot.getplane()
        top = self.top.getplane()
        f.writef(
            bot.getz (self.w.x1, self.w.y1),
            bot.getz (self.w.x2, self.w.y2),
            top.getz (self.w.x1, self.w.y1),
            top.getz (self.w.x2, self.w.y2))

def copystruct(self, s):
    if hasattr(s, '_type'):
        for k, type, pos in s._type.field_order:
            setattr(self, k, getattr(s, k))
    else:
        self.__dict__.update(s.__dict__)

class Area(object):
    def __init__(self, ca=None):
        self.tess = None
        if ca is not None:
            copystruct(self, ca)

        self.vis = ''
                    
    def pointin(self, x, y):
        cnt = 0;
        for w in self.walls:
            if ((w.y1 < y) == (w.y2 < y)):
                continue
            xi = (w.x1 - ((w.x2 - w.x1) / (w.y2 - w.y1)) * (w.y1 - y));
            if xi < x:
                cnt += 1
        return cnt&1
    
    def findpt(self):
        mpx = mpy = 0
        for w in self.walls:
            mpx += w.x
            mpy += w.y
        mpx /= len(self.walls)
        mpy /= len(self.walls)
        if self.pointin(mpx, mpy): return (mpx, mpy)
        for i, wa in enumerate(self.walls):
            for wb in self.walls[i+1:]:
                mpx = (wa.x1+wb.x1)/2
                mpy = (wa.y1+wb.y1)/2
                if self.pointin(mpx, mpy): return (mpx, mpy)
        timer.msg('%s has no points inside!?'%self.id)
        return (0, 0)
    
    def getreplace(self):
        area = self
        while hasattr(area, 'replace'):
            if area == area.replace:
                return area
            area = area.replace
        return area
    
    def transform(self, xo, yo):
        self.ceil.transform(xo, yo)
        self.floor.transform(xo, yo)
        
        for w in self.walls:
            w.x += xo
            w.y += yo
            
        for a in self.actors:
            a.x += xo
            a.y += yo
        
class Wall(object):
    def __init__(self, cw=None):
        if cw is not None:
            copystruct(self, cw)

    def __getattr__(self, attr):
        if attr == 'x1':
            return self.x
        elif attr == 'y1':
            return self.y
        elif attr == 'x2':
            return self.point2.x
        elif attr == 'y2':
            return self.point2.y
        else:
            return object.__getattribute__(self, attr)

    def parallel(self, o):
        if self.x1 == self.x2:
            return o.x1 == o.x2
        
        if o.x1 == o.x2:
            return False
        
        return (self.y2 - self.y1) /  (self.x2 - self.x1) == (o.y2 - o.y1) /  (o.x2 - o.x1)

    def intersect(self, o):
        jx = self.x2 - self.x1
        jy = self.y2 - self.y1
        ojx = o.x2 - o.x1
        ojy = o.y2 - o.y1
        rx1 = o.x1 - self.x1
        ry1 = o.y1 - self.y1
        rx2 = o.x2 - self.x1
        ry2 = o.y2 - self.y1

        if ((rx1 > jx and rx2 > jx and jx > 0) or
            (rx1 < jx and rx2 < jx and jx < 0)):
            return None

        if jx == 0:
            if ojx == 0:
                return None
            else:
                return o.intersect(self)
        else:
            cof = jy/jx
            ry1 -= cof*rx1
            ry2 -= cof*rx2

            if (ry1 > 0) == (ry2 > 0):
                return None

            xint = rx1 - ((rx2 - rx1) / (ry2 - ry1)) * ry1;
            if (xint < 0) == (xint < jx):
                return None
            return (xint + self.x1, self.y1 + cof * xint)
        

    def __setattr__(self, name, val):
        object.__setattr__(self, name, val)
        if not hasattr(self, 'x') or not hasattr(self, 'y'): return
        if name == 'point2' or name == 'x' or name == 'y':
            if not hasattr(val, 'x') or not hasattr(val, 'y'): return
            self.jx = self.x2 - self.x1
            self.jy = self.y2 - self.y1

            self.len = math.sqrt(sqr(self.jx)+sqr(self.jy))

            self.jx /= self.len
            self.jy /= self.len

class Actor(object):
    def __init__(self, cs=None):
        if cs is not None:
            copystruct(self, cs)
        if 'extra' in self.__dict__:
            del self.extra
    def extra(self, ds):
        pass
    def link(self):
        pass

class World(object):
    def __init__(self, file_or_stream=None):
        self.areas = []
        self.groups = []
        self.actors = []
        self.staticactors = []
