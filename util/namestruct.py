from struct import pack,unpack,calcsize

__all__=['struct','structdef','readarray','writearray']

class struct:
    def __init__(self,_def,data=None):
        self._def=_def
        if data is not None:
            lst=unpack(_def.fmt,data)
            for i,n in enumerate(_def.names):
                setattr(self,n,lst[i])

    def getsize(self):
        return self._def.getsize()
    
    def getdata(self):
        return pack(self._def.fmt,*[getattr(self,n) for n in self._def.names])
        
    def writeto(self,fil):
        fil.write(self.getdata())

    def copy(self):
        ret = struct(self._def)
        for n in self._def.names:
            setattr(ret,n,getattr(self,n))
        return ret

    def __str__(self):
        return '{ '+', '.join(['%s: %s'%(n,getattr(self,n)) for n in self._def.names])+' }'
            
    def __repr__(self):
        return str(self)
    
class structdef:
    def __init__(self,definition):
        self.names=[]
        lins=definition.split('\n')
        self.fmt=lins[0]
        for lin in lins[1:]:
            lst=[x for x in lin.split(' ') if x]
            if not lst: continue
            type=lst[0]
            self.fmt += '%d%s'%(len(lst)-1,type);
            self.names += lst[1:]
        self.size=calcsize(self.fmt)

    def readfrom(self,f):
        data=f.read(self.size)
        return struct(self,data)
            
    def getsize(self):
        return self.size


def readarray(f,sfmt,sdef):
    num,=unpack(sfmt,f.read(calcsize(sfmt)))
    return [sdef.readfrom(f) for i in xrange(num)]

def writearray(f,sfmt,arr):
    f.write(pack(sfmt,len(arr)))
    for s in arr:
        s.writeto(f)

