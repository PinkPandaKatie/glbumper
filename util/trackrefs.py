import sys

ptrs = set()

def printaddrs():
    print "Addrs:",' '.join(ptrs)
    
while True:
    l = sys.stdin.readline()
    if not l: break
    addr = l[3:-1]
    if l.startswith('+++0'):
        print 'Add:',addr
        ptrs.add(addr)
        printaddrs();
    elif l.startswith('---0'):
        print 'Remove:',addr
        if addr in ptrs:
            ptrs.remove(addr)
        else:
            print 'WTF?????\007',addr
        printaddrs();
            
    else:
        print l[:-1]
