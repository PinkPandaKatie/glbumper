import sys
import os
import re
import math
import struct, gzip

import Image, ImageDraw

from gtk import gdk
import pango

import gdkkeysyms as ks

from pygraph import *

try:
    import psyco
    psyco.full()
except:
    pass

overlap={}

def _bin_rw(fmt):
    l = struct.calcsize(fmt)
    pak = struct.pack
    upk = struct.unpack
    def read(self):
        return upk(">%s"%fmt,self.read(l))[0]
    def write(self,*v):
        self.write(pak(">%d%s"%(len(v),fmt),*v))
    return read, write

class datastream(object):
    def __init__(self,f):
        self.read = f.read
        self.write = f.write
        self.close = f.close
        self.f = f
        self.stable = []
        self.sidxmap = {}

    readu8, writeu8 = _bin_rw('B')
    readu16, writeu16 = _bin_rw('H')
    readu32, writeu32 = _bin_rw('I')
    reads8, writes8 = _bin_rw('b')
    reads16, writes16 = _bin_rw('h')
    reads32, writes32 = _bin_rw('i')
    readf, writef = _bin_rw('d')

    def writestring(self, str, usetbl=True):
        if usetbl:
            idx = self.sidxmap.get(str,None)
            if idx is not None:
                self.writeu16(idx)
                return
            if len(self.stable) < 65534:
                self.sidxmap[str] = len(self.stable)
                self.stable.append(str)
                self.writeu16(65535)
            else:
                self.writeu16(65534)
        else:
            self.writeu16(65534)

        self.writeu16(len(str))
        self.write(str)
        
    def readstring(self):
        ln = self.readu16()
        str = self.read(ln)
        return str


class Plane(object):
    def __init__(self, x=0.0, y=0.0, z=0.0, ang=0.0, slope=0.0, tex=None, xpan=0.0, ypan=0.0, xscl=0.0, yscl=0.0, flags=0):
        self.x = x
        self.y = y
        self.z = z
        self.ang = ang
        self.ix = math.cos(ang * math.pi / 180)
        self.iy = math.sin(ang * math.pi / 180)
        self.slope = slope
        self.tex = tex
        self.xpan = xpan
        self.ypan = ypan
        self.xscl = xscl
        self.yscl = yscl
        self.flags = flags

    def copy(self):
        return Plane(self.x, self.y, self.z, self.ang, self.slope, self.tex, 
                     self.xpan, self.ypan, self.xscl, self.yscl, self.flags)

    def writeto(self, f):
        f.write(('%12.4f %12.4f %12.4f %35.30f %35.30f "%s"'+
                '%10.4f %10.4f %10.4f %10.4f %d')%
                (self.x, self.y, self.z, self.ang, self.slope, self.tex or '',
                 self.xpan, self.ypan, self.xscl, self.yscl, self.flags))

    def getz(self, x, y):
        return self.z + ((x - self.x) * self.ix +
                         (y - self.y) * self.iy) * self.slope

class PlaneSpec(object):
    def __init__(self, area, floor):
        self.area = area
        self.floor = floor
        
    def get(self):
        tarea = self.area
        attr = 'ceilreplace'
        if self.floor: attr = 'floorreplace'
        while hasattr(tarea, attr):
            tarea = getattr(tarea, attr)
        return ('c', 'f')[self.floor]+" "+tarea.getreplace().id

        
class WallPart(object):
    def __init__(self, tex=None, flip=0, xpan=0.0, ypan=0.0, xs=0.0, ys=0.0, top=None, bot=None, align=None):
        self.tex = tex
        self.flip = flip
        self.xpan = xpan
        self.ypan = ypan
        self.xs = xs
        self.ys = ys
        self.top = top
        self.bot = bot
        self.align = align

    def copy(self):
        return WallPart(self.tex, self.flip, self.xpan, self.ypan, self.xs, 
                        self.ys, self.top, self.bot, self.align)

    def writeto(self, f):
        f.write(' { "%s" %d %10.4f %10.4f %17.12f %17.12f %6s %6s %6s }'\
                %(self.tex, self.flip, self.xpan, self.ypan, 
                  self.xs, self.ys, self.top.get(), self.bot.get(), 
                  self.align.get()))

        
class Area(object):
    
    def __init__(self, ca=None, id = None):
        self.contours=[]
        self.actors=[]

        self.id = id
        self.valid = False
        
        if ca is not None:
            for k, v in ca.__dict__.iteritems():
                if not k.startswith('_'):
                    setattr(self, k, v);
        
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
        print '%s has no points inside!?'%self.id
        return (0, 0)

            
    
    def getreplace(self):
        area = self
        while hasattr(area, 'replace'):
            if area == area.replace:
                return area
            area = area.replace
        return area
 

class Wall(object):
    def __init__(self, cw=None):
        self.links = {}
        self.parts = []
        self.x = 0.0
        self.y = 0.0
        
        if cw is not None:
            for k, v in cw.__dict__.iteritems():
                if not k.startswith('_'):
                    setattr(self, k, v);

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
            for k, v in cs.__dict__.iteritems():
                if not k.startswith('_'):
                    setattr(self, k, v);

class Map(object):
    def __init__(self):
        self.areas = []
        self.areadict = {}
        
    def getarea(self,id):
        return self.areadict.setdefault(id,Area(id=id));
    
    def __getattr__(self, attr):
        if attr == 'walls':
            walls = []
            for ar in self.areas: walls.extend(ar.walls)
            return walls
        if attr == 'actors':
            actors = []
            for ar in self.areas: actors.extend(ar.actors)
            return actors
        else:
            return object.__getattribute__(self, attr)

def readplane(ds):
    ret = Plane()

    ret.x = ds.readf()
    ret.y = ds.readf()
    ret.z = ds.readf()

    ret.ix = ds.readf()
    ret.iy = ds.readf()
    ret.slope = ds.readf()


    ret.xpan = ds.readf()
    ret.ypan = ds.readf()
    ret.xscl = ds.readf()
    ret.yscl = ds.readf()

    ret.flags = ds.readu16()
    ret.tex = ds.readstring()

    return ret
    
def readplanespec(nmap,ds):
    fc = ds.readu8()
    aid = ds.readu16()
    return PlaneSpec(nmap.areas[aid],bool(fc))

def writenow(msg):
    sys.stdout.write(msg);
    sys.stdout.flush()
    

def readmap(ds):
    map = Map()

    writenow('Reading map...')

    readu8 = ds.readu8
    readu16 = ds.readu16
    readstring = ds.readstring
    readf = ds.readf
    
    readu16()
    nareas = readu16()

    map.areas = [Area() for i in xrange(nareas)]

    for i, ar in enumerate(map.areas):
        ar.idx = i
    
        writenow('.')

        ar.id = readstring();
        ar.ceil = readplane(ds)
        ar.floor = readplane(ds)
        
        ncont = readu8();
        ar.contours = []
        for i in xrange(ncont):
            nwalls = readu8();
            cc = []
            for i in xrange(nwalls):
                w = Wall()
                w.x = readf()
                w.y = readf()
                w.links={}
                nlinks = readu8()
                for i in xrange(nlinks):
                    aid = readu16()
                    fl = readu8()
                    w.links[map.areas[aid]] = fl

                nparts = readu8()
                for i in xrange(nparts):
                    
                    wp = WallPart()
                    wp.tex = readstring()
                    wp.flags = readu8()
                    wp.xpan = readf()
                    wp.ypan = readf()
                    wp.xs = readf()
                    wp.ys = readf()
                    wp.top = readplanespec(map,ds)
                    wp.bot = readplanespec(map,ds)
                    #wp.align = readplanespec(map,ds)
                    
                    w.parts.append(wp)
                
                cc.append(w)
                
            ar.contours.append(cc)

        ar.actors = []

        nactors = readu16()
        for i in xrange(nactors):
            a = Actor()
            a.clas = ds.readstring()
            a.x = readf()
            a.y = readf()
            a.z = readf()
            a.area = ar
            ar.actors.append(a)
            a.ang = readf()

            a.extra = ds.read(readu16())
            
    writenow('\n')
    return map

wid = None

def reload(*args):
    #global lines, w, h
    try:
        map = readmap(datastream(gzip.GzipFile(sys.argv[1])))
    except:
        map = readmap(datastream(file(sys.argv[1],"rb")))
        

    scl = float(sys.argv[2])

    minx, miny = None, None
    maxx, maxy = None, None

    for a in map.areas:
        for c in a.contours:
            for w in c:
                if minx is None or minx > w.x: minx = w.x
                if miny is None or miny > w.y: miny = w.y
                if maxx is None or maxx < w.x: maxx = w.x
                if maxy is None or maxy < w.y: maxy = w.y

    minx *= scl
    miny *= scl
    maxx *= scl
    maxy *= scl

    minx -= 32
    miny -= 32
    maxx += 32
    maxy += 32

    ofsx = int(-minx)
    ofsy = int(maxy)
    w = int(maxx-minx)
    h = int(maxy-miny)

    print w, h

    lines=[]
    for a in map.areas:
            
        for c in a.contours:
            for w1,w2 in zip(c,c[1:]+c[:1]):
                col = 0
                if w1.links:
                    col = 1
                if a.id in overlap:
                    col += 2
                lines.append((int(w1.x*scl+ofsx),int(w1.y*-scl+ofsy),
                              int(w2.x*scl+ofsx),int(w2.y*-scl+ofsy),col))
    lines.sort(key=lambda v: (1, 0, 3, 2)[v[4]])
    wid.lines = lines
    wid.getwidget().set_size(w,h)
    if wid.wid.window:
        update_widget(wid.wid);

def rct(rect):
    return rect.x,rect.y,rect.width,rect.height


class customwidget:
    def __init__(self):
        wid=gtk.Layout()
        

        wid.connect("expose-event",self.raw_expose_event);
        wid.connect("key-press-event",self.raw_key_event);
        wid.connect("key-release-event",self.raw_key_event);
        wid.connect("button-press-event",self.raw_button_event);
        wid.connect("button-release-event",self.raw_button_event);
        wid.connect("motion-notify-event",self.raw_motion_event);
        
        wid.add_events(gdk.EXPOSURE_MASK|gdk.KEY_PRESS_MASK|gdk.KEY_RELEASE_MASK|
                       gdk.BUTTON_PRESS_MASK|gdk.BUTTON_RELEASE_MASK|gdk.POINTER_MOTION_MASK)

        self.keys=None
        self.wid=wid;
    def raw_key_event(self,widget,event):
        if not self.keys: return False
        press=(event.type==gdk.KEY_PRESS)
        if event.keyval in self.keys and press:
            self.keys[event.keyval]()
            return True
        return False
    def invalidate_rect(self,x,y,w,h):
        self.wid.bin_window.invalidate_rect((x,y,w,h),False);
    def draw(self,wid,w,area):
        pass
    def button(self,x,y,b,s):
        pass
    def mouse(self,x,y):
        pass
    def raw_expose_event(self,widget,event):
        self.draw(widget,widget.bin_window,rct(event.area))
    def raw_button_event(self,widget,event):
        x,y=event.get_coords()
        but=event.button
        if but<=3:
            self.button(x,y,but,event.type==gdk.BUTTON_PRESS)
            
    def raw_motion_event(self,widget,event):
        x,y=event.get_coords()
        self.mouse(x,y)
    def getwidget(self):
        return self.wid
            

class linewidget(customwidget):
    def __init__(self,lines,w,h):
        customwidget.__init__(self)
        self.lines = lines
        self.wid.set_size(w,h)
        self.gridlevel=4
        self.keys={
            ks.Up:lambda:self.movey(-16),
            ks.Down:lambda:self.movey(16),
            ks.Left:lambda:self.movex(-16),
            ks.Right:lambda:self.movex(16),
            };
        self.wid.set_flags(gtk.CAN_FOCUS)
        

    def move(self,adj,amt):
        nv=adj.value+amt
        if nv>adj.upper-adj.page_size:
            nv=adj.upper-adj.page_size
        adj.set_value(nv)
        
    def movex(self,amt):
        self.move(self.wid.get_hadjustment(),amt)
    def movey(self,amt):
        self.move(self.wid.get_vadjustment(),amt)
    def draw(self,wid,w,area):
        g = graph(w,wid)
        cols = ("#FFFFFF","#FF9900", "#00FFFF","#FF9900")

        dx,dy,dw,dh=area

        g.set_foreground("#000000")
        g.draw_rectangle(True,dx,dy,dw,dh)

        g.set_foreground("#404040")
        if self.gridlevel:
            skip=(1<<self.gridlevel)
            mask=~(skip-1)
            for gy in xrange(dy&mask,((dy+dh+skip-1)&mask),skip):
                g.draw_line(dx,gy,dx+dw,gy)
            for gx in xrange(dx&mask,((dx+dw+skip-1)&mask),skip):
                g.draw_line(gx,dy,gx,dy+dh)

        setcolor = g.set_foreground
        line = g.draw_line
        lcol = -1
        for x1,y1,x2,y2,col in self.lines:
            if x1<dx and x2<dx: continue
            if x1>dx+dw and x2>dx+dw: continue
            if y1<dy and y2<dy: continue
            if y1>dy+dh and y2>dy+dh: continue
            if lcol != col:
                setcolor(cols[col])
                lcol = col
            line(x1,y1,x2,y2)




def build_ui():
    global wid
    
    box1=gtk.VBox();
    
    menubar=gtk.MenuBar();
    filemenu=gtk.Menu();
    
    item=gtk.MenuItem("File")
    item.set_submenu(filemenu)
    menubar.append(item);
    
    item=gtk.ImageMenuItem("gtk-quit");
    item.connect("activate",destroy);
    filemenu.append(item);

    item=gtk.MenuItem("Reload");
    item.connect("activate",reload);
    filemenu.append(item);

    box1.pack_start(menubar,False,True,0)
    
    swin=gtk.ScrolledWindow()
    

    wid = linewidget(None,0,0)
    swin.add(wid.getwidget());
    swin.set_policy(gtk.POLICY_ALWAYS,gtk.POLICY_ALWAYS);
    box1.pack_start(swin,True,True,0)
    
    return box1


def destroy(wid=None):
    #global _loopfunc
    sys.exit(0);
    #_loopfunc=None

def deleteevent(wid,evt):
    return False

window=gtk.Window()
window.set_title('test');
window.connect("destroy",destroy);
window.connect("delete-event",deleteevent);

window.add(build_ui())
window.set_default_size(800,600);
reload()

window.show_all();

if False:
    img = Image.new("RGB",(w,h))

    draw = ImageDraw.Draw(img)

    draw.rectangle((0,0,w,h),fill=(0,0,0))

    cols = [(255,255,255),(255,0,0)]
    for x1,y1,x2,y2,col in lines:
        draw.line((x1,y1,x2,y2),fill=cols[col])

    #draw.line((0,0,100,100),fill=(1,1,1))


    img.save(sys.argv[3])
    
gtk.main()
