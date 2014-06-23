import buildmap,sys



map=buildmap.map(sys.argv[1])
toadd = int(sys.argv[2])

for s in map.sectors:
    s.ceilingz += toadd
    s.floorz += toadd

for s in map.sprites:
    s.z += toadd

map.writeto(sys.argv[3])
