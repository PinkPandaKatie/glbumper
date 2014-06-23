import buildmap,sys



map=buildmap.map(sys.argv[1])

for w in map.walls:
    if w.picnum == 0:
        w.picnum = 23

for s in map.sectors:
    if s.floorpicnum == 0:
        s.floorpicnum = 23
    if s.ceilingpicnum == 0:
        s.ceilingpicnum = 23
    
map.writeto(sys.argv[2])
