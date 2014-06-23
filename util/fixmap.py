import buildmap,sys



map=buildmap.map(sys.argv[1])

for s in map.sectors:
    s.ceilingshade=-127
    s.floorshade=-127

for w in map.walls:
    w.shade=-127
    w.cstat &= ~1

    #if w.picnum == 26 or w.picnum == 27:
    #    w.xrepeat *= 2
    #    w.yrepeat *= 2

#map.sprites = [a for a in map.sprites if a.picnum != 1]


map.writeto(sys.argv[2])
