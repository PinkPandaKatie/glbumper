import namestruct,buildmap,sys
map=buildmap.map(sys.argv[1])

fact = float(sys.argv[2])

basex = map.x #-131072
basey = map.y #-131072

for s in map.sectors:
    s.ceilingz = int (s.ceilingz * fact)
    s.floorz = int (s.floorz * fact)

for w in map.walls:
    w.x = int((w.x-basex) * fact + basex)
    w.y = int((w.y-basey) * fact + basey)
    w.xrepeat *= fact
    if w.xrepeat >= 255:
        w.xrepeat = 0

for s in map.sprites:
    s.x = int((w.x-basex) * fact + basex)
    s.y = int((w.y-basey) * fact + basey)

map.x = int((map.x-basex) * fact + basex)
map.y = int((map.y-basey) * fact + basey)
map.z = int (map.z * fact)


map.writeto(sys.argv[3])
