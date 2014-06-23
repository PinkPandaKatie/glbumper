from genmap import *
import sys
zz=1024-3648
bridges={
        24: bridge(plane(  448,	    0,		-416,		0,	0,	 	"walnut",	0,	0,''),	
	           plane(  448,	    0,		-416-16,	0,	0,		"ceiltex",	0,	0,'')),

        25: bridge(plane(  448,	    0,		-340,		0,	0,	      	"walnut",	0,	0,''),	
	           plane(  448,	    0,		-340-16,	0,	0,		"ceiltex",	0,	0,'')),
        
       }
print bridges
class mangler:
    def postconvert(self):
        nseekers=0
        for s in self.map.sectors:
            for a in list(s.actors):
                
                if a.clas == 'seeker':
                    if nseekers:
                        nseekers -= 1
                    else:
                        s.actors.remove(a)

        for w in self.map.walls:
            
            if w.osect and (w.hitag == 10 or w.hitag == 12):
                ofz = w.osect.keys()[0].floor.getz(w.x1,w.y1)
                fl = w.sect.floor
                fl.slope = (ofz - fl.z) / ((w.x1 - fl.x)*fl.ix + (w.y1 - fl.y)*fl.iy)
            if w.osect and (w.hitag == 11 or w.hitag == 12):
                ocz = w.osect.keys()[0].ceil.getz(w.x1,w.y1)
                ce = w.sect.ceil
                ce.slope = (ocz - ce.z) / ((w.x1 - ce.x)*ce.ix + (w.y1 - ce.y)*ce.iy)


convert(['pathtest'],'pathtest.world',[bridges],mangler());



