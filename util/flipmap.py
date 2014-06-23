import buildmap,sys



map=buildmap.map(sys.argv[1])

for s in map.sprites:
    s.x = -s.x


for i,s in enumerate(map.walls):
    map.walls[s.point2].point0 = i
    s.x = -s.x
    s.ox = s.x
    s.oy = s.y

for w in map.walls:
    w.x = map.walls[w.point2].ox
    w.y = map.walls[w.point2].oy
    w.point2 = w.point0

map.writeto(sys.argv[2])
