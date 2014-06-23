import sys
from genmap import *
import mkbridge
bridges= [
# 0
	bridge(plane(0,		0,		160,		0,	0,	 	"walnut.png",	0,	0,''),	
	       plane(0,		0,		128,		0,	0,		"ceiltex.png",	0,	0,'')),
# 1
	bridge(plane(0,		0,		256,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		248,		0,	0,		"ceiltex.png",	0,	0,'')),
# 2
	bridge(plane(0,		0,		256,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		128,		0,	0,		"ceiltex.png",	0,	0,'')),
# 3
	bridge(plane(0,		0,		232,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		256,		0,	0,		"ceiltex.png",	0,	0,'')),
# 4
	bridge(plane(0,		0,		128,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		176,		0,	0,		"ceiltex.png",	0,	0,'')),
# 5
	bridge(plane(0,		0,		356,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		340,		0,	0,		"ceiltex.png",	0,	0,'')),
# 6
	bridge(plane(0,		0,		120,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		68,		0,	0,		"ceiltex.png",	0,	0,'')),
# 7
	bridge(plane(0,		0,		292,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		76,		0,	0,		"ceiltex.png",	0,	0,'')),
# 8
	bridge(plane(0,		0,		32,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		16,		0,	0,		"ceiltex.png",	0,	0,'')),
# 9
	bridge(plane(0,		0,		356,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		68,		0,	0,		"ceiltex.png",	0,	0,'')),
# 10
	bridge(plane(0,		0,		-32,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		-48,		0,	0,		"ceiltex.png",	0,	0,'')),
# 11
	bridge(plane(0,		0,		160,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		128,		0,	0,		"ceiltex.png",	0,	0,'')),
# 12
	bridge(plane(640,		-7360,		356,		-90,	388.0/1152,	"walnut.png",	0,	0,''),	
	       plane(640,		-7360,		340,		-90,	388.0/1152,	"ceiltex.png",	0,	0,'')),
# 13
	bridge(plane(640,		-7360,		356,		-90,	388.0/1152,	"walnut.png",	0,	0,''),	
	       plane(640,		-7360,		-32,		-90,	0,		"ceiltex.png",	0,	0,'')),
#14          
	bridge(plane(0,		0,		928,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		928-16,		0,	0,		"ceiltex.png",	0,	0,'')),
#15
	bridge(plane(0,		0,		16,		0,	0,		"walnut.png",	0,	0,''),	
	       plane(0,		0,		-8,		0,	0,		"ceiltex.png",	0,	0,'')),

       ]

bridgemap=dict(enumerate(bridges))

class mangler:
    
    def postconvert(self):
        for w in self.map.walls:
            if (w.hitag == 10 or w.hitag == 12) and w.osect:
                ofz = w.osect.keys()[0].floor.getz(w.x1,w.y1)
                fl = w.sect.floor
                fl.slope = (ofz - fl.z) / ((w.x1 - fl.x)*fl.ix + (w.y1 - fl.y)*fl.iy)
            if (w.hitag == 11 or w.hitag == 12) and w.osect:
                ocz = w.osect.keys()[0].ceil.getz(w.x1,w.y1)
                ce = w.sect.ceil
                ce.slope = (ocz - ce.z) / ((w.x1 - ce.x)*ce.ix + (w.y1 - ce.y)*ce.iy)

    def presplit(self):
        for s in self.map.sectors:
            if bridges[15] in s.bridges:
                s.bridges.append(bridges[10])
        print 'generating bridges'
        mkbridge.go(self.map)
        print 'done!'
        


convert(['hovertest'],'hovertest.world',[bridgemap],mangler())

if False:
    tsect = map.sectors[56]
    for w in tsect.walls:
        if w.osect == 54:
            break
    cxpan = 0.0
    while not hasattr(w,'scaleset'):
        w.scaleset=True
        w.parts[0].xs = 1.0
        w.parts[0].xpan = cxpan
        #w.parts[0].flip |= 4

        cxpan += (w.len)
        w = tsect.walls[w.nextwall]

    save()
