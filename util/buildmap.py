import sys
#import struct as ostruct
#from namestruct import *
from cstruct import addto, Array, types

S_PARALLAX = 1
S_GROUDRAW = 2
S_SWAPXY = 4
S_SMALL = 8
S_XFLIP = 16
S_YFLIP = 32
S_ALIGN = 64

W_BLOCK = 1
W_SWAP = 2
W_ALIGN = 4
W_XFLIP = 8
W_MASK = 16
W_1WAY = 32
W_HITSCAN = 64
W_TRANS = 128
W_YFLIP = 256
W_TRANSREV = 512


SP_BLOCK = 1
SP_TRANS = 2
SP_XFLIP = 4
SP_YFLIP = 8
SP_WALL = 16
SP_FLOOR = 32
SP_1SIDE = 64
SP_CENTER = 128
SP_HITSCAN = 256
SP_TRANSREV = 512
SP_INVIS = 32768


addto(globals(), '''

struct sectortype {
    s16le wallptr, wallnum;
    s32le ceilingz, floorz;
    s16le ceilingstat, floorstat;
    s16le ceilingpicnum, ceilingheinum;
    s8    ceilingshade;
    u8    ceilingpal, ceilingxpanning, ceilingypanning;
    s16le floorpicnum, floorheinum;
    s8 floorshade;
    u8    floorpal, floorxpanning, floorypanning;
    u8    visibility, filler;
    s16le lotag, hitag, extra;
};

struct walltype {
    s32le x, y;
    s16le point2, nextwall, nextsector, cstat;
    s16le picnum, overpicnum;
    s8    shade;
    s8    pal, xrepeat, yrepeat, xpanning, ypanning;
    s16le lotag, hitag, extra;
};

struct spritetype {
    s32le x, y, z;
    s16le cstat, picnum;
    s8    shade;
    u8    pal, clipdist, filler;
    u8    xrepeat, yrepeat;
    s8    xoffset, yoffset;
    s16le sectnum, statnum;
    s16le ang, owner, xvel, yvel, zvel;
    s16le lotag, hitag, extra;
};

struct map_header {
    s32le vers, x, y, z;
    u16le ang, sectnum;
};

''')

def getbuf(data, pos, type):
    val = type.get(data, pos)
    return val, pos + type.size

class BuildMap(object):
    @classmethod
    def from_buffer(cls, data, pos):
        self = cls()

        hdr, pos = getbuf(data, pos, map_header)
        self.x, self.y, self.z, self.ang = hdr.x, hdr.y, hdr.z, hdr.ang
        self.sectnum = hdr.sectnum
        

        numsectors, pos = getbuf(data, pos, types.u16le)
        self.sectors, pos = getbuf(data, pos, Array(sectortype, numsectors))
        
        numwalls, pos = getbuf(data, pos, types.u16le)
        self.walls, pos = getbuf(data, pos, Array(walltype, numwalls))

        numsprites, pos = getbuf(data, pos, types.u16le)
        self.sprites, pos = getbuf(data, pos, Array(spritetype, numsprites))

        return self

    @classmethod
    def from_stream(cls, stream):
        data = stream.read()
        return cls.from_buffer(data, 0)
        
    @classmethod
    def from_file(cls, name):
        f = open(name, 'rb')
        ret = cls.from_stream(f)
        f.close()
        return ret
        
    def __init__(self): 
        self.x = 0
        self.y = 0
        self.z = 0
        self.ang = 0
        self.sectnum = 0
        self.sectors = []
        self.walls = []
        self.sprites = []

    def copy(self):
        nself = type(self)()
        nself.x = self.x
        nself.y = self.y
        nself.z = self.z
        nself.ang = self.ang
        nself.sectnum = self.sectnum
        nself.sectors = self.sectors.copy()
        nself.copy = self.walls.copy()
        nself.sprites = self.sprites.copy()
        
    def writeto(self, file_or_stream):
        if hasattr(file_or_stream,'write'):
            stream = file_or_stream
        else:
            stream = file(file_or_stream,'wb')

        hdr = map_header.new()
        hdr.vers = 7
        hdr.x, hdr.y, hdr.z, hdr.ang, hdr.sectnum = \
               self.x, self.y, self.sectnum, self.ang, self.sectnum
        hdr.write_to(stream)

        stream.write(types.u32le.tostring(len(self.sectors)))
        self.sectors.write_to(stream)

        stream.write(types.u32le.tostring(len(self.walls)))
        self.walls.write_to(stream)

        stream.write(types.u32le.tostring(len(self.sprites)))
        self.sprites.write_to(stream)

map = BuildMap
