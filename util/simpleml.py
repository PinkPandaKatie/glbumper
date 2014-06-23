from genmap import *
import sys
bridges=[
# 0
	bridge(plane(0,		0,		-160,		0,	0,	 	'dmud.png',	0,	0,1.0,1.0,0),	
	       plane(0,		0,		-200,		0,	0,		'ceiltex.png',	0,	0,1.0,1.0,0)),

	bridge(plane(0,		0,		-100,		0,	0,	 	'dmud.png',	0,	0,1.0,1.0,0),	
	       plane(0,		0,		-128,		0,	0,		'ceiltex.png',	0,	0,1.0,1.0,0)),
]

class mangler(object):
    pass


convert(['simpleml'],'simpleml.world',[dict(enumerate(bridges))],mangler);
