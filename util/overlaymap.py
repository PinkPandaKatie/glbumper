import namestruct,buildmap,sys
import Image
map=buildmap.map(sys.argv[1])

img = Image.open(sys.argv[2])

itr=iter(img.getdata())

nspr = []
for s in map.sprites:
    if s.picnum != 1: nspr.append(s)
map.sprites=nspr
for y in xrange(img.size[1]):
    for x in xrange(img.size[0]):
        r,g,b = itr.next()[:3]

        if r == 255 and g == 255 and b == 255:
            nspr = namestruct.struct(buildmap.spritestruct)
            nspr.x = (x+12) * 640 - 131072
            nspr.y = (y+12) * 640 - 131072
            nspr.z = 0
            nspr.cstat = 0
            nspr.picnum = 1
            nspr.shade = nspr.pal = nspr.clipdist = nspr.filler = 0
            nspr.xrepeat = nspr.yrepeat = 8
            nspr.xoffset = nspr.yoffset = 0
            nspr.sectnum = nspr.statnum = nspr.ang = nspr.owner = 0
            nspr.xvel = nspr.yvel = nspr.zvel = nspr.hitag = nspr.lotag = nspr.extra = 0
            map.sprites.append(nspr)

            

map.writeto(sys.argv[3])
