#!/usr/local/bin/python

import sys
import os
import re
import traceback

classes = {}

class tokenreader(object):
    """A wrapper for an iterator that allows one object to be pushed back."""
    def __init__(self, lst):
        self.lst = lst
        self.itr = iter(lst)
        self.pb = None
        self.last = None
        
    def __iter__(self):
        return self

    def pushback(self):
        self.pb = self.last

    def next(self):
        if self.pb:
            self.last = self.pb
            self.pb = None
            return self.last
        self.last = self.itr.next()
        return self.last
    
# 
class classdef:
    """Defines how a member is traversed and cleared."""
    def __init__(self, typename = None, types = []):
        if typename == None:
            typename = type(self).__name__
        self.typename = typename
        self.types = types
    
    def visit(self, obj, n=0):
        return ""

    def clear(self, obj, n=0):
        return ""
    
    def isnoop(self):
        for t in self.types:
            if not t.isnoop():
                return False
        return True

    def __str__(self):
        if self.types:
            extra=''
            if self.types[-1].types:
                extra=' '
            return "%s<%s%s>"%(self.typename, ', '.join(str(t) for t in self.types), extra)
        else:
            return self.typename

class sptr(classdef):
    """Defines how to traverse a smart pointer."""
    def visit(self, obj, n=0):
        return '    '*n + "_visit(%s, _data);\n"%obj
    
    def clear(self, obj, n=0):
        return '    '*n + "%s = NULL;\n"%obj
    
    def isnoop(self):
        return False

class container(classdef):
    """Defines how to traverse an STL container (vector, set, etc.)"""
    
    def visit(self, obj, n=0):
        spc = '    '*n
        txt = self.types[0].visit("*_itr%d"%n, n+1)
        return "%(spc)sfor(%(self)s::iterator _itr%(n)d = (%(obj)s).begin(); "\
               "_itr%(n)d != (%(obj)s).end(); ++_itr%(n)d) {\n%(txt)s%(spc)s}\n"%locals()
    
    def clear(self, obj, n=0):
        return '    '*n + "%s.clear();\n"%obj

class map(container):
    def __init__(self, typename, types):
        classdef.__init__(self, typename, types)
        self.types = [pair(typename,self.types)]
        
    def __str__(self):
        return str(self.types[0])

class pair(classdef):
    """Defines how to traverse an STL pair."""

    def visit(self, obj, n=0):
        return self.types[0].visit("(%s).first"%obj, n) + \
               self.types[1].visit("(%s).second"%obj, n)

    def clear(self, obj, n=0):
        return self.types[0].clear("(%s).first"%obj, n) + \
               self.types[1].visit("(%s).second"%obj, n)


class custom_classdef(classdef):
    """
    A class definition that can be defined by putting a special
    command in the comments of the C++ sources.
    """
    def __init__(self, typename, types, visit, clear):
        classdef.__init__(self, typename, types)
        self.visit_f = visit
        self.clear_f = clear
        
    def isnoop(self):
        return False

    def eval(self, txt, obj, n):
        locs = {}
        locs['types'] = self.types
        locs['obj'] = obj
        locs['n'] = n
        ret = eval(txt, globals(), locs)
        return ret
        
    
    def visit(self, obj, n = 0):
        return self.eval(self.visit_f, obj, n)
    
    def clear(self, obj, n = 0):
        return self.eval(self.clear_f, obj, n)

classdeftypes = {
    "sptr" : sptr,
    "pair" : pair,
    "list" : container,
    "set" : container,
    "queue" : container,
    "deque" : container,
    "vector" : container,
    "map" : map,
    "multimap" : map,
};


class cppclass(object):
    """Describes a C++ class and it's members that need to be traversed."""

    def __init__(self, name, header):
        self.name = name
        self.header = header
        self.supers = set()
        self.subs = set()
        self.members = {}

    def getallsubs(self):
        ret = set(self.subs)
        for c in list(ret):
            ret |= c.getallsubs()
        return ret
    
    def getallsupers(self):
        ret = set(self.supers)
        for c in list(ret):
            ret |= c.getallsupers()
        return ret


def def_custom_classdef(type, vis, clr):
    def ret(typename, types):
        return custom_classdef(typename, types, vis, clr)
    classdeftypes[type] = ret
    
    

def get_classdef(type, temptypes):
    return classdeftypes.get(type, classdef)(type, temptypes)
   

def readtype(itr):
    """
    Reads a C++ type from itr and returns a classdef describing how to
    traverse the type.
    """

    basetype = itr.next();
    if basetype == "unsigned":
        basetype += " " + itr.next();

    temptypes = []
    
    t = itr.next();

    # Is it a template type?
    if t == "<":
        while True:
            temptypes.append(readtype(itr))
            t = itr.next();
            if t == ">":
                break
            elif t == ",":
                pass
            else:
                itr.pushback();
    else:
        itr.pushback()
        
    return get_classdef(basetype, temptypes)

    

def readtoend(itr):
    """
    Read until ';' or '}' is seen, allowing nested braces.
    """
    bracelvl = 0;
    while True:
        t = itr.next();
        if t == ";":
            if not bracelvl:
                return
            continue
        if t == "{":
            bracelvl += 1
            continue
        
        if t == "}":
            bracelvl -= 1
            if not bracelvl:
                return
            continue

def readmembers(classname, itr):
    """
    Read the traversable members for a class from itr. Also makes sure
    the DECLARE_OBJECT macro is present.
    """
    members = {}
    hasmacro = False
    while True:
        t = itr.next();
        if t == "public" or t == "protected" or t == "private":
            itr.next(); #skip the colon
        
        elif (t == "virtual" or t == "static" or t == "typedef" or 
              t == "class" or t == "struct" or t == "~" or t == classname or 
              t == "void"):
            # Method or static member
            readtoend(itr);

        elif t == '}':
            # End of class definition
            break

        elif t == 'DECLARE_OBJECT':
            # Object declaration macro
            hasmacro = True
        elif t != ';': # Empty declaration

            # Read the type.
            itr.pushback()
            typ = readtype(itr);

            if typ.isnoop():
                readtoend(itr);
            else:

                tmembers={}
                while True:
                    mname = itr.next();
                    while mname in '*&,':
                        mname = itr.next();
                    if mname == ';':
                        break
                    if mname == '(':
                        readtoend(itr)
                        tmembers={}
                        break
                    tmembers[mname] = typ;
                members.update(tmembers);
    return hasmacro, members
                
            
        
        
def readclass(itr, header):
    classname = itr.next();
    bracelvl = 0;
    templvl = 0;

    ret = cppclass(classname, header);
    while True:
        t = itr.next();
        if t == "{":
            ret.hasmacro, ret.members = readmembers(classname, itr)
            return ret
        if t == ";":
            return
        if t == "public":
            ret.supers.add(itr.next())
            continue

def parse_c_file(data, included, defined):
    if included is None:
        included = set()

    def incsub(m):
        h = m.group(1)
        if h in included:
            return ''
        included.add(h)

        try:
            data = parse_c_file(file(h).read(), included, defined)
            return data
        except:
            #traceback.print_exc()
            return ''

    def def_custom_sub(m):
        type = m.group(1).strip();
        vis = m.group(2).strip();
        clr = m.group(3).strip();
        def_custom_classdef(type, vis, clr)
        return ''

    def def_object_sub(m):
        defined.add(m.group(1))
        return ''
    
    data = re.sub(r'(?s)/\*(.*?)\*/', '', data);
    data = re.sub(r'(?s)\\\n', '', data);
    data = re.sub(r'(?s)//\s*@OBJDEF (.*?)\n\s*//(.*?)\n//(.*?)\n', def_custom_sub, data);
    data = re.sub(r'//(.*?)\n', '', data);
    data = re.sub(r'\#include\s+objdef\((.*?)\)\s*\n', def_object_sub, data);
    data = re.sub(r'\#include\s+"(.*?)"\n', incsub, data);
    return data

def read_header(header, included, defined):
    
    data = file(header).read()
    
    f=[True]
        
    data = parse_c_file(data, included, defined)

    data = re.sub(r'\#.*?\n', '', data);
    data = re.sub(r'([{}<>*&\[\]();])', r' \1 ', data);
    data = re.sub(r'\b', ' ', data);
    tokens = [i for i in re.split(r'\s+', data) if i];

    
    itr = tokenreader(tokens)
    for t in itr:
        if t == 'class':
            c = readclass(itr, header)
            if c:
                classes[c.name] = c

included=set()
defined=set()
for i in sys.argv[1:]:
    read_header(i, included, defined)
    
for name, c in classes.iteritems():
    c.supers = set([classes[sc] for sc in c.supers if sc in classes])

for name, c in classes.iteritems():
    for sc in c.supers:
        sc.subs.add(c)

objclasses = classes['Object'].getallsubs();

f = sys.stdout
#f.write("")
#f.write('/*\n\n'r'\b\('+'\\|'.join(objclasses)+r'\)\b\( *\)\*'+'\n\n*/\n\n\n')

classesinheader={}

for c in objclasses:
    if not c.name in defined:
        continue

    print "Creating definition for %s"%c.name
    f = file("objdef/%s.impl"%c.name, "w")
    
    if not c.hasmacro:
        f.write('/******** DECLARATION MACRO NOT FOUND!!! *******/\n')

    allclasses = [c] + [sc for sc in c.getallsupers() if sc.hasmacro]

    
    f.write("void %s::__gc_traverse_members(visitproc _visit, void* _data) {\n"%c.name)
    for mname, typ in c.members.iteritems():
        f.write(typ.visit(mname,1))
    f.write("}\n\n")

    f.write("void %s::__gc_clear_members() throw() {\n"%c.name)
    #f.write("    cout << \"DEBUG: Clear members for '%s'\" << endl;\n"%c.name)
    for mname, typ in c.members.iteritems():
        f.write(typ.clear(mname,1))
    f.write("}\n\n")


    needtrack = False

    for sc in allclasses:
        if sc.members:
            needtrack = True
            break

    f.write("void %s::__gc_traverse(visitproc _visit, void* _data) {\n"%c.name)
    for sc in allclasses:
        f.write("    %s::__gc_traverse_members(_visit, _data);\n"%sc.name)
    f.write("}\n\n")

    f.write("void %s::__gc_clear() throw() {\n"%c.name)
    for sc in allclasses:
        f.write("    %s::__gc_clear_members();\n"%sc.name)
    f.write("}\n\n")

    f.write("bool %s::__gc_needtrack() throw() {\n"%c.name)
    if needtrack:
        f.write("    return true;\n");
    else:
        f.write("    return false;\n");
    f.write("}\n\n")

    f.write("string %s::classname() const throw() { return \"%s\"; }\n\n"%(c.name,c.name))
                  
