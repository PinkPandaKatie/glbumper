import buildmap,sys



map=buildmap.map(sys.argv[1])

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


map.writeto(sys.argv[2])
