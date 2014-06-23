
import sys,os,gtk,gobject;

from gtk import gdk
from math import *

import gdkkeysyms as ks

class vect(object):
    def __init__(self,x=0,y=0):
        self.x=x; self.y=y;

    def add(self,o):
        return vect(self.x+o.x,self.y+o.y)
    
    def sub(self,o):
        return vect(self.x-o.x,self.y-o.y)
    
    def mul(self,s):
        return vect(self.x*s,self.y*s)

    def div(self,s):
        return vect(self.x/s,self.y/s)

    def len(self):
        return sqrt(self.x*self.x+self.y*self.y)
    
    def len2(self):
        return self.x*self.x+self.y*self.y

    def dot(self,o):
        return self.x*o.x+self.y*o.y

    def cross(self):
        return vect(self.y,-self.x)

    def normal(self):
        ret=vect(self.x,self.y)
        ret.normalize();
        return ret
    
    def normalize(self):
        len=self.len()
        self.x/=len
        self.y/=len
        
    def __str__(self):
        return "<%f,%f>"%(self.x,self.y);
    


class line(object):
    def __init__(self,v1,v2):
        self.pt=v1;
        rel=v2.sub(v1);
        len=rel.len();
        jvct=vect(rel.x/len,rel.y/len)
        ivct=vect(-jvct.y,jvct.x)

        iivct=vect(-ivct.x,ivct.y)
        ijvct=vect(jvct.x,-jvct.y)
        
        self.len=len
        self.ivct=ivct; self.jvct=jvct;
        self.iivct=iivct; self.ijvct=ijvct;
        self.v1=v1
        self.v2=v2

    def draw(self,w,gc):
        w.draw_line(gc,int(self.v1.x),int(self.v1.y),int(self.v2.x),int(self.v2.y))

    def pointin(self,v,rad):
        tv=v.sub(self.pt)
        tpnt=self.iivct.mul(tv.y).add(self.ijvct.mul(tv.x))
        retpt=None
        retd=0
        if tpnt.x>=0 and tpnt.x<=self.len and tpnt.y<=rad and tpnt.y>=-rad:
            if tpnt.y>0:
                retpt=vect(tpnt.x,rad)
                retd=tpnt.y+rad
            else:
                retpt=vect(tpnt.x,-rad)
                retd=-tpnt.y+rad
        else:
            ln=tpnt.len2();
            r2=rad*rad
            if ln<r2:
                retd=tpnt.len();
                retpt=tpnt.div(retd).mul(rad);
            tpnt.x-=self.len
            ln=tpnt.len2();
            if ln<r2:
                retd=tpnt.len();
                retpt=tpnt.div(retd).mul(rad);
                retpt.x+=self.len
        if retpt:
            return (self.ivct.mul(retpt.y).add(self.jvct.mul(retpt.x)).add(self.pt),retd)
        return (None,0)
        #print tpnt
        #return (self.vct.x*(v.x-self.pt.x)+vct.y-(v.y-self.pt.y))>=0


lines=[line(vect(x1,y1),vect(x2,y2)) for x1,y1,x2,y2 in
            (
    (100,100,150,200),
    (150,200,225,210),
    (225,210,300,200),
    (300,200,200,230),
    (300,300,400,215),
    (400,215,150,20),
    (150,20,100,100),
    (100,400,200,400),
    (150,392,250,392),
    )]

sw=500
sh=500

FWVEL=4
TURNVEL=pi/28.0

def update_widget(wid):
    win=wid.window;
    w,h=win.get_size();
    win.invalidate_rect((0,0,w,h),True);
    
def recursive_clip(pt,rad,move,lin,sl=None):
    lln=100000
    lvct=None
    if not lin:
        return move
    if len(lin)==1:
        v,d=lin[0].pointin(move)
        if v:
            return v
        return move
    i=0;
    for l in lin:
        if l==sl: continue
        v,d=l.pointin(move,rad);
        if v and (v.x!=move.x or v.y!=move.y):
            r=list(lin)
            del r[i]
            vct=recursive_clip(pt,rad,v,r,l)
            tln=vct.sub(pt).len2()
            print tln
            if tln<lln:
                lln=tln
                lvct=vct
        i+=1
    if lvct:
        return lvct
    return move

def iter_clip(pt,rad,move,lin):
    cont=True
    while cont:
        cont=False
        sd=0
        sv=move
        for l in lines:
            v,d=l.pointin(move,rad);
            if v and not (close(v.x,move.x) and close(v.y,move.y)): #(v.x!=newpt.x or v.y!=newpt.y):
                #print v,newpt,close(v.x,newpt.x),close(v.y,newpt.y)
                #print "mode 0",v,d
                cont=True
                if d>sd:
                    sv=v
                    sd=d
                #break
        #else:
        #
        move=sv
    return move
    
def cm(press):
    pass
def close(x,y):
    return fabs(x-y)<0.00001

class player(object):
    def __init__(self,fw,turn):
        self.fw=fw;
        self.turn=turn;
        self.x=150;
        self.y=100;
        self.ang=0;
        self.rad=15;
        
    def draw(self,w,gc):
        
        s=sin(self.ang);
        c=cos(self.ang);
        wh=self.rad*2
        tx=int(self.x-self.rad)
        ty=int(self.y-self.rad)
        w.draw_arc(gc,False,tx,ty,wh,wh,0,23040);
        tx2=int(self.x+c*self.rad)
        ty2=int(self.y+s*self.rad)
        w.draw_line(gc,int(self.x),int(self.y),tx2,ty2)
        #for l in lines:
        #    v=l.pointin(vect(self.x,self.y),self.rad)
        #    if v:
        #        tx=int(v.x-6)
        #        ty=int(v.y-6)
        #        w.draw_arc(gc,False,tx,ty,12,12,0,23040);

    def tick(self):
        if self.fw.val:
            s=sin(self.ang)*self.fw.val*FWVEL;
            c=cos(self.ang)*self.fw.val*FWVEL;
            #print s,c,self.ang
            pt=vect(self.x,self.y);
            newpt=vect(self.x+c,self.y+s);
            newpt=iter_clip(pt,self.rad,newpt,lines)
            self.x=newpt.x;
            self.y=newpt.y;
            update_widget(window)
                    
        if self.turn.val:
            self.ang+=self.turn.val*TURNVEL;
            update_widget(window)
    
        
        
def drawscene(widget,event):
    w=widget.window
    cmap=widget.get_colormap()
    colors=[cmap.alloc_color(i) for i in ("#000000","#FFFFFF")]
    #for c in colors: cmap.alloc_color(c);
    #print "%08X"%colors[0].pixel

    gc=w.new_gc(foreground=colors[0])
    w.draw_rectangle(gc,True,0,0,sw,sh)

    gc.set_foreground(colors[1])
    
    plr.draw(w,gc);

    for l in lines:
        l.draw(w,gc);
    return True



class vctrl(object):
    def __init__(self):
        self.press=[0,0]
        self.val=0;
    def getfunc(self,val):
        def ret(press):
            self.press[val]=press
            self.val=self.press[1]-self.press[0]
        return ret

fw=vctrl();
turn=vctrl();

plr=player(fw,turn);

keybind={
    ks.Up:fw.getfunc(1),
    ks.Down:fw.getfunc(0),
    ks.Left:turn.getfunc(0),
    ks.Right:turn.getfunc(1),
    ks.space:cm,
}

def keyevt(widget,event):
    press=(event.type==gdk.KEY_PRESS)
    if event.keyval in keybind:
        keybind[event.keyval](press)
    return True

def create_widget():
    ret=gtk.DrawingArea()
    ret.connect("expose-event",drawscene);
    ret.connect("key-press-event",keyevt);
    ret.connect("key-release-event",keyevt);
    ret.set_flags(gtk.CAN_FOCUS)
    ret.add_events(gdk.EXPOSURE_MASK|gdk.KEY_PRESS_MASK|gdk.KEY_RELEASE_MASK)
    ret.set_size_request(sw,sh);
    return ret



def destroy(wid=None):
    gtk.main_quit();

def build_ui():
    box1=gtk.VBox();
    
    menubar=gtk.MenuBar();
    filemenu=gtk.Menu();
    
    item=gtk.MenuItem("File")
    item.set_submenu(filemenu)
    menubar.append(item);
    
    item=gtk.ImageMenuItem("gtk-quit");
    item.connect("activate",destroy);
    filemenu.append(item);
    
    box1.pack_start(menubar,False,True,0)
    
    wid=create_widget();
    
    box1.pack_start(wid,True,True,0)
    
    return box1


def timer():
    #print fw.val,turn.val
    plr.tick()
    return True

gobject.timeout_add(20,timer);

window=gtk.Window()
window.set_title('test');
window.connect("destroy",destroy);

window.add(build_ui())
window.set_default_size(300,400);

window.show_all();

build_ui()
gtk.main();
