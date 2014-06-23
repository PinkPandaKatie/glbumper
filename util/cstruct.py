import re
import struct
from array import array
from cStringIO import StringIO

try:
    from struct import Struct as _Struct
except ImportError:
    class _Struct(object):
        def __init__(self, fmt):
            self.format = format
            self.size = struct.calcsize(format)

        def pack(self, *args):
            return struct.pack(self.format, *args)

        def unpack(self, *args):
            return struct.pack(self.format, *args)

def stringbuf(s):
    return array('B', s)

def nullterm(s):
    idx = s.find('\0')
    if idx != -1:
        return s[:idx]
    return s

class types:
    pass

class Field(object):
    size = 0
    def __init__(self):
        pass

    def get(self, buf, pos=0):
        pass

    def set(self, buf, pos, val):
        pass

    def mkarray(self, size):
        return Array(self, size)
    
    def tostring(self, val):
        buf = stringbuf('\0' * self.size)
        self.set(buf, 0, val)
        return buf.tostring()

class ComplexField(Field):
    def set(self, buf, pos, val):
        nval = self.get(buf, pos)
        nval.update(val)

    def new(self):
        buf = stringbuf('\0' * self.size)
        return self.get(buf, 0)

    def tostring(self, val):
        nval = self.new()
        nval.update(val)
        return nval.tostring()

    def read_from(self, f):
        data = ''
        while len(data) < self.size:
            ndata = f.read(self.size - len(data))
            if not ndata:
                raise EOFError
            data += ndata
        return self.get(data, 0)

    def write_to(self, f, v):
        f.write(self.tostring(v))

class Value(object):
    def __init__(self, type, buf, pos):
        self._type = type
        self._buf = buf
        self._pos = pos
        self._size = type.size

    def get_type(self):
        return self._type

    def copy(self):
        bufcpy = self._buf[self._pos : self._pos + self._size]
        if isinstance(bufcpy, str):
            bufcpy = stringbuf(bufcpy)
        return type(self)(self._type, bufcpy, 0)

    def cast(self, ntype):
        return ntype.get(self._buf, self._pos)

    def tostring(self):
        return str(buffer(self._buf, self._pos, self._size))

class PrimField(_Struct, Field):
    def get(self, buf, pos=0):
        return self.unpack_from(buf, pos)[0]
    
    def set(self, buf, pos, val):
        return self.pack_into(buf, pos, val)

    def tostring(self, val):
        return self.pack(val)

    def __repr__(self):
        return 'PrimField(%r)' % self.format


class StringField(PrimField):
    def __init__(self, size):
        PrimField.__init__(self, str(size) + 's')

    def __repr__(self):
        return 'StringField(%d)' % self.size

class NullStringField(StringField):
    def get(self, buf, pos):
        str = StringField.get(self, buf, pos)
        return nullterm(str)

class CharField(PrimField):
    def __init__(self, aclas):
        PrimField.__init__(self, 'c')
        self.aclas = aclas
        
    def mkarray(self, size):
        return self.aclas(size)

class ArrayVal(Value):
    def __len__(self):
        return self._type.len

    def _getpos(self, i):
        len = self._type.len
        if i < 0:
            i += len

        if not 0 <= i < len:
            raise IndexError
        
        return self._pos + self._type.basesize * i

    def __getitem__(self, i):
        return self._type.base.get(self._buf, self._getpos(i))

    def __setitem__(self, i, v):
        return self._type.base.set(self._buf, self._getpos(i), v)
        
    def __iter__(self):
        base = self._type.base
        base_size = base.size

        cpos = self._pos
        for i in xrange(self._type.len):
            yield base.get(self._buf, cpos)
            cpos += base_size

    def __repr__(self):
        return repr(list(self))

    def cast_length(self, newlen):
        return self.cast(Array(self._type.base, newlen))
    
    def update(self, val):
        for i, v in enumerate(val):
            self[i] = v

class Array(ComplexField):
    def __init__(self, base, len):
        self.base = base
        self.len = len
        self.basesize = base.size
        self.size = len * base.size

    def get_base(self):
        return self.base

    def length(self):
        return self.len
    
    def get(self, buf, pos=0):
        return ArrayVal(self, buf, pos)

    def __repr__(self):
        return 'Array(%r, %d)' % (self.base, self.len)


class StructVal(Value):
    def __getitem__(self, item):
        type, pos = self._type.fields[item]
        return type.get(self._buf, self._pos + pos)
        
    def __setitem__(self, item, val):
        type, pos = self._type.fields[item]
        return type.set(self._buf, self._pos + pos, val)

    def __iter__(self):
        for k, type, pos in self._type.field_order:
            yield k, type.get(self._buf, self._pos + pos)

    def __getattr__(self, attr):
        try:
            return self.__getitem__(attr)
        except KeyError:
            raise AttributeError
            
    def __setattr__(self, attr, val):
        if attr in ('_type', '_buf', '_pos'):
            return object.__setattr__(self, attr, val)

        try:
            return self.__setitem__(attr, val)
        except KeyError:
            return object.__setattr__(self, attr, val)

    def __repr__(self):
        return '{%s}' % ', '.join('%r: %r' % (k, v) for k, v in self)

    def update(self, val):
        if hasattr(val, 'iteritems'):
            itr = val.iteritems()
        else:
            itr = iter(val)

        for k, v in itr:
            self[k] = v

class Struct(ComplexField):
    is_union = False

    def __init__(self, *fields):
        cpos = 0
        size = 0
        field_order = self.field_order = []

        self.fields = fdict = {}
        for name, type in fields:
            if name is None:
                if not hasattr(type, 'field_order'):
                    raise ValueError('only structs and unions can be anonymous')
                for name, type, pos in type.field_order:
                    field_order.append((name, type, pos + cpos))
                    fdict[name] = type, pos + cpos
            else:
                field_order.append((name, type, cpos))
                fdict[name] = type, cpos

            if self.is_union:
                if type.size > size:
                    size = type.size
            else:
                cpos += type.size

        if self.is_union:
            self.size = size
        else:
            self.size = cpos
        
    def get(self, buf, pos=0):
        return StructVal(self, buf, pos)

    def __repr__(self):
        return 'Struct(%r)' % (self.field_order,)

class Union(Struct):
    is_union = True
    def __repr__(self):
        return 'Union(%r)' % (self.field_order,)

#
# Parser
#

class Token(object):
    def __init__(self, pat):
        self.rx = re.compile(pat, re.S)
        
    def match(self, txt, pos):
        m = self.rx.match(txt, pos)
        if m:
            return self.getval(m), m.end()
        return None, None

    def getval(self, m):
        grp = m.groups()
        if grp:
            return grp[0]

    def __repr__(self):
        return repr(self.rx.pattern)

class TokenLst(Token):
    def getval(self, m):
        return m.groups()

tkwhitespace = Token(r'\s+')
rx_continue_comment = re.compile(r'\\\n|//.*?(\n|$)|/\*.*?\*/', re.S)


tkdefine = TokenLst(r'\#define\s+(\w+)\s+(.*?)(?:\n|$)')

tkocbkt = Token(r'(\{)')
tkccbkt = Token(r'(\})')
tksemi = Token(r'(;)')
tkcomma = Token(r'(,)')


tkword = Token(r'(\w+)')
tkarray = Token(r'\[(.*?)\]')
tkchar = Token(r'(.)')


class ParserBase(object):
    def __init__(self, txt, modprefix=None):
        txt = rx_continue_comment.sub('', txt)

        self.txt = txt
        self.pos = 0
        
    def _read(self, types):
        spos = self.pos
        for tk in types:
            v, e = tk.match(self.txt, spos)
            if e is not None:
                self.pos = e
                return tk, v, spos
        return None, None, spos

    def read(self, *types):
        rval = self._read(types)
        while not self.isend():
            itk, iv, ipos = self._read((tkwhitespace,))
            if not itk:
                break
        return rval

    def isend(self):
        return self.pos >= len(self.txt)

    def parse_varlist(self):
        btype = self.parse_type()
        tk, name, pos = self.read(tksemi)
        if tk:
            yield None, btype
            return

        while True:
            tk, name, pos = self.read(tkword)
            if not tk:
                raise ValueError("expected word")
            
            array_sizes = []
            while True:
                tk, size, pos = self.read(tkarray)
                if not tk:
                    break
                array_sizes.append(size)

            ctype = btype
            for size in reversed(array_sizes):
                ctype = self.make_array(ctype, size)
            yield name, ctype
                
            tk, c, pos = self.read(tksemi, tkcomma)
            if not tk:
                raise ValueError("expected ',' or ';' after list")
            if tk == tksemi:
                break

    def parse_type(self):
        tk, name, pos = self.read(tkword)
        if not tk:
            raise ValueError("expected type name")
            
        if name == 'struct' or name == 'union':
            tk, c, pos = self.read(tkocbkt, tkword)
            strname = None
            if tk == tkword:
                strname = c
                tk, c, pos = self.read(tkocbkt)

            if c != '{':
                if not strname:
                    raise ValueError("expected '{' after 'struct'")
                return self.get_struct(strname)

            fields = []
            while True:
                tk, c, pos = self.read(tkccbkt)
                if tk:
                    break
                
                fields.extend(self.parse_varlist())

            return self.make_struct(name == 'union', strname, fields)
        else:
            return self.get_simple_type(name)

    def parse_typedef(self):
        for name, tp in self.parse_varlist():
            if name is None:
                raise ValueError('empty typedef');
            self.define_type(name, tp)
    
    def parse(self):
        while not self.isend():
            tk, val, pos = self.read(tkword, tkdefine)
            if tk == tkword:
                if val == 'typedef':
                    self.parse_typedef()
                    self.clear()
                elif val == 'struct':
                    self.pos = pos
                    self.parse_type()
                    tk, val, pos = self.read(tksemi)
                    if not tk:
                        raise ValueError("expected ';' after struct")

                else:
                    raise ValueError('expected struct or typedef')

            elif tk == tkdefine:
                self.define(val[0], val[1])
            else:
                raise ValueError('unknown token near %r' % (self.txt[self.pos : self.pos + 15]))
                #raise ValueError('unknown token near %r' % (self.txt[max(0, self.pos - 5) : self.pos + 5]))


#
# Types for code generator
#

class TypeSpec(object):
    def __init__(self, txt):
        self.txt = txt

    def get(self, modprefix):
        return self.txt

    def mkarray(self, size):
        return ArrayTypeSpec(self, size)

class BuiltinTypeSpec(TypeSpec):
    def get(self, modprefix):
        return modprefix + self.txt

class CharTypeSpec(BuiltinTypeSpec):
    def __init__(self, txt, aclas):
        BuiltinTypeSpec.__init__(self, txt)
        self.aclas = aclas
        
    def mkarray(self, size):
        return StringTypeSpec(size, self.aclas)

class ArrayTypeSpec(TypeSpec):
    def __init__(self, base, size):
        self.base = base
        self.size = size

    def get(self, modprefix):
        return '%sArray(%s, %s)' % (modprefix, self.base.get(modprefix), self.size)

class StringTypeSpec(TypeSpec):
    def __init__(self, size, aclas):
        self.size = size
        self.aclas = aclas
        
    def get(self, modprefix):
        return '%s%s(%s)' % (modprefix, self.aclas, self.size)

class CodeGenerator(ParserBase):
    def __init__(self, txt, modprefix=None):
        ParserBase.__init__(self, txt)

        if modprefix is None:
            modprefix = __name__ + '.'

        self.modprefix = modprefix
        self.anon_structs = []
        self.output = StringIO()

    def define(self, name, val):
        self.output.write('%s = %s\n' % (name, val))

    def define_type(self, name, tp):
        tptxt = tp.get(self.modprefix)
        if name != tptxt: # e.g. typedef struct foo { ... } foo;
            self.output.write('%s = %s\n' % (name, tptxt))

    def make_array(self, base, size):
        return base.mkarray(size)

    def get_simple_type(self, name):
        if name == 'char':
            return CharTypeSpec('types.' + name, 'NullStringField')
        elif name == 'bchar':
            return CharTypeSpec('types.' + name, 'StringField')
        elif hasattr(types, name):
            return BuiltinTypeSpec('types.' + name)
        else:
            return TypeSpec(name)

    def get_struct(self, name):
        return TypeSpec(name)

    def make_struct(self, union, strname, fields):
        if union:
            clas = 'Union'
        else:
            clas = 'Struct'

        if strname is None:
           strname = '_anonstruct_%d' % len(self.anon_structs)
           self.anon_structs.append(strname)

        self.output.write('%s = %s%s(\n' % (strname, clas, self.modprefix))
        for name, tp in fields:
            self.output.write('    (%r, %s),\n' % (name, tp.get(self.modprefix)))

        self.output.write(')\n')
        return TypeSpec(strname)

    def clear(self):
        if self.anon_structs:
            self.output.write ('del %s\n' % ', '.join(self.anon_structs))
            self.anon_structs[:] = []

class Parser(ParserBase):
    def __init__(self, txt, defines=None):
        ParserBase.__init__(self, txt)
        if defines is None:
            defines = {}
        self.defines = defines

    def _get_int_val(self, txt):
        txt = txt.strip()
        try:
            return int(txt, 0)
        except ValueError:
            pass

        val = self.defines[txt]
        try:
            return int(val)
        except ValueError:
            pass
            
        raise AttributeError("can't parse %s" % txt)

    def clear(self):
        pass

    def define(self, name, val):
        self.defines[name] = self._get_int_val(val)

    def define_type(self, name, tp):
        self.defines[name] = tp
        
    def make_array(self, base, size):
        return base.mkarray(self._get_int_val(size))

    def get_simple_type(self, name):
        type = self.defines.get(name)
        if isinstance(type, Field):
            return type

        if hasattr(types, name):
            return getattr(types, name)

        raise ValueError('invalid type: %s' % name)

    def get_struct(self, name):
        type = self.defines.get(name)
        if isinstance(type, Struct):
            return type

        raise ValueError('invalid type: %s' % name)

    def make_struct(self, union, strname, fields):
        if union:
            struc = Union(*fields)
        else:
            struc = Struct(*fields)
        if strname is not None:
            self.defines[strname] = struc
        return struc

def addto(dct, txt):
    tt = Parser(txt, dct)
    tt.read()
    try:
        tt.parse()
    except ValueError, e:
        msg = e.args[0]
        msg += ' (near %r)' % tt.txt[max(0, tt.pos - 10) : tt.pos + 10]
        e.args = (msg,)
        raise
    return dct

def parse(txt, **kw):
    return addto(kw, txt)
    
def gencode(txt, modprefix = None):
    tt = CodeGenerator(txt, modprefix)
    tt.read()
    tt.parse()
    return tt.output.getvalue()

basetypes = types.__dict__

def _mkprim(chr, name):
    basetypes[name] = PrimField(chr)
    basetypes[name + 'be'] = PrimField('>' + chr)
    basetypes[name + 'le'] = PrimField('<' + chr)

def _mkprimus(chr, n):
    _mkprim(chr, 's' + n)
    _mkprim(chr.upper(), 'u' + n)


basetypes['bchar'] = CharField(StringField)
basetypes['char'] = CharField(NullStringField)
_mkprimus('b', '8')
_mkprimus('h', '16')
_mkprimus('i', '32')
_mkprimus('1', '64')
_mkprim('f', 'float')
_mkprim('d', 'double')

del _mkprim, _mkprimus, basetypes

def test():
    testcode = '''

// test comment 1
/* test
   comment
   2
*/

#define STR_SIZE 15

struct test1 {
    u8 byte1, byte_array[10];
    char c1, string1[STR_SIZE], c2;
    bchar binstring1[STR_SIZE];
    union { char uchar; u8 ubyte; };
    u16le le_short;
    u16be be_short;
    u8 two_d_array[4][3];
    struct {
        u32be i1;
        u16be i2;
    } head, rest[15];
};

typedef struct test1 test1_array[100];

typedef struct {
    u8 byte2;
    test1 head, another_array[3];
} test2;

typedef u32le grub_uint32_t;
typedef u16le grub_uint16_t;
typedef u8 grub_uint8_t;


'''


    txt = gencode(testcode, '')

    print txt

    #exec txt

    addto(globals(), testcode)

    print 'test1 = %r' % test1
    print 'size = %d' % test1.size

    tv1 = test1.new()
    tv1.byte1 = 42
    tv1.byte_array = [1, 2, 3]
    tv1.byte_array[6] = 50

    tv1.c1 = '!'
    tv1.string1 = 'test!!'
    tv1.binstring1 = 'test_bin_string'
    tv1.c2 = 'x'
    tv1.uchar = 'A'
    tv1.le_short = 9000
    tv1.be_short = 9000
    tv1.two_d_array = [[10, 11, 12], [13, 14, 15]]
    tv1.two_d_array[2] = [16, 17, 18]
    tv1.two_d_array[3][0] = 19
    tv1.two_d_array[3][1] = 20
    tv1.two_d_array[3][2] = 21

    tv1.head.i1 = 0xdeadbeef
    tv1.head.i2 = 0xf00d

    for i in xrange(15):
        tv1.rest[i].i1 = i + 1000000;
        tv1.rest[i].i2 = i + 1000;

    #tv1.
    data = tv1.tostring()
    print repr(data)

    tv2 = test1.get(data)
    print tv2

if __name__ == '__main__':
    import sys
    
    if len(sys.argv) < 2:
        test()
    else:
        input = sys.argv[1]
        if input == '-':
            infil = sys.stdin
        else:
            infil = open(input, 'rb')

        code = infil.read()

        print gencode(code, '')
