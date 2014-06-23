import buildmap,sys



map=buildmap.map(sys.argv[1])

for w in map.walls:
    if w.nextsector == 65535:
        w.cstat &= ~4
    else:
        w.cstat |= 4
    
map.writeto(sys.argv[2])
