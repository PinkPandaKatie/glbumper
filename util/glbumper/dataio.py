import struct
from cStringIO import StringIO as StringIO


try:
    from struct import Struct
    def _fmtrw(fmt):
        struct = Struct(">" + fmt)
        siz = struct.size
        pack = struct.pack
        unpak = struct.unpack
        def retw(self, *dats):
            for dat in dats:
                self.write(pak(dat))

        def retr(self):
            dat = self.read(siz)
            if len(dat) < siz:
                raise EOFError()
            return upak(dat)[0]
        
        return retw, retr

    def _converter(frm, to):
        frm = Struct('@' + frm)
        to = Struct('@' + to)
        return (lambda v: to.unpack(frm.pack(v))[0]), (lambda v: frm.unpack(to.pack())[0])
    
except ImportError:
    import struct

    def _fmtrw(fmt):
        rfmt = ">" + fmt

        siz = struct.calcsize(rfmt)
        pak = struct.pack
        upak = struct.unpack
        def retw(self, *dats):
            for dat in dats:
                self.write(pak(rfmt, dat))

        def retr(self):
            dat = self.read(siz)
            if len(dat) < siz:
                raise EOFError()
            return upak(rfmt, dat)[0]
        return retw, retr

    def _converter(frm, to):
        upk = struct.unpack
        pk = struct.pack
        frm = "@" + frm
        to = "@" + to
        return (lambda v: upk(to, pk(frm, v))[0]), (lambda v: upk(frm, pk(to, v))[0])
    
u32_to_float, float_to_u32 = _converter("I", "f")
u64_to_double, double_to_u64 = _converter("Q", "d")

class DataIO(object):
    def __init__(self, data_or_stream=None):
        self.type = type
        
        stream = None
        if data_or_stream is None:
            stream = StringIO()
        elif isinstance(data_or_stream, str):
            stream = StringIO(data_or_stream)
        else:
            stream = data_or_stream

        for a in ('read', 'write', 'getvalue', 'seek', 'tell', 'close'):
            if hasattr(stream, a):
                setattr(self, a, getattr(stream, a))

        self.stream = stream

        self.read_nbits = 0
        self.read_bucket = 0
        
        self.write_nbits = 0
        self.write_bucket = 0
    

    def tellbit(self):
        return (self.tell() << 3) - self.read_nbits + self.write_nbits

    def readvar(self):
        ret=0
        while True:
            rd = self.readbyte()
            ret <<= 7
            ret |= rd&127
            if not (rd&128): break
        return ret
    
    def writevar(self, var):
        sh = 28
        f=w=0
        while True:
            w = (var>>sh)&127
            if not sh: break
            sh -= 7
            if w or f:
                f=1
                self.writebyte(w|128)
        self.writebyte(w)

    def readbits(self,nbits):
        bitbucket=self.read_bucket
        bitsinbucket=self.read_nbits
        if nbits<=self.read_nbits:
	    ret = bitbucket>>(bitsinbucket-nbits)
	    bitsinbucket -= nbits
	    self.read_bucket = (bitbucket & (1<<bitsinbucket)-1)
            self.read_nbits = bitsinbucket
            return ret
        ret = bitbucket
        nbits -= bitsinbucket
        bitsinbucket = bitbucket = 0;
        while nbits:
            if nbits>=16:
                ret <<= 16
                ret |= self.readshort()
                nbits -= 16
            elif nbits >= 8:
                ret <<= 8;
                ret |= self.readbyte()
                nbits -= 8
            else:
                ret <<= nbits
                bitbucket = self.readbyte()
                ret |= (bitbucket>>(8-nbits))
                bitsinbucket = 8-nbits
                bitbucket &= (1<<bitsinbucket)-1
                nbits = 0
        self.read_nbits = bitsinbucket
        self.read_bucket = bitbucket
        return ret
    
    def readalign(self):
	self.read_nbits = self.read_bucket = 0

    def writebits(self,val,nbits):
        bitbucket=self.write_bucket
        bitsinbucket=self.write_nbits
        while nbits:
	    b2w = 32-bitsinbucket
            if b2w>nbits: b2w = nbits
	    bitbucket <<= b2w
	    bitsinbucket += b2w
	    bitbucket |= (val>>(nbits-b2w))&((1<<b2w)-1)
	    nbits -= b2w
            if bitsinbucket >= 32:
		self.writeint(bitbucket)
		bitsinbucket = bitbucket = 0;
        self.write_nbits = bitsinbucket
        self.write_bucket = bitbucket


    def writealign(self):
        while self.write_nbits>=16:
	    self.writeshort(self.write_bucket>>(self.write_nbits-16));
	    self.write_nbits -= 16;
        if self.write_nbits>=8:
	    self.writebyte(self.write_bucket>>(self.write_nbits-8));
	    self.write_nbits -= 8;

	if self.write_nbits>0:
	    self.writebyte(self.write_bucket<<(8-self.write_nbits));
	self.write_nbits = self.write_bucket=0;

    
    writebyte,readbyte = _fmtrw("B");
    writeshort,readshort = _fmtrw("H");
    writeint,readint = _fmtrw("I");

    writesbyte,readsbyte = _fmtrw("b");
    writesshort,readsshort = _fmtrw("h");
    writesint,readsint = _fmtrw("i");

    writefloat,readfloat = _fmtrw("f");
    writedouble,readdouble = _fmtrw("d");

    def writestring(self, *strs):
        for str in strs:
            self.writevar(len(str))
            self.write(str)
        
    def readstring(self):
        len = self.readvar()
        return self.read(len)

