import buildmap,sys



map=buildmap.map(sys.argv[1])

for s in map.sectors:
    s.ceilingshade=-127
    s.floorshade=-127

    s.floorpicnum = 22
    s.ceilingpicnum = 20

for w in map.walls:
    w.shade=-127
    w.cstat &= ~1

    w.picnum = 23

#map.sprites = [a for a in map.sprites if a.picnum != 1]


map.writeto(sys.argv[2])
