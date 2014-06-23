import namestruct,buildmap,sys
map=buildmap.map(sys.argv[1])

fact = float(sys.argv[2])

basex = -131072
basey = -131072

for w in map.walls:
    w.x = int((w.x-basex) * fact + basex)
    w.y = int((w.y-basey) * fact + basey)
    #w.xrepeat *= fact
    #if w.xrepeat >= 255:
    #    w.xrepeat = 0

for s in map.sprites:
    s.x = int((w.x-basex) * fact + basex)
    s.y = int((w.y-basey) * fact + basey)

map.writeto(sys.argv[3])
