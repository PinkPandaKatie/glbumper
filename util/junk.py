
#if __name__ == '__main__':
#    import sys,oa
#    from oa.path import basename

#    nmapf=sys.argv[0];
#    base=basename(mapf)
    


#     hook('preconvertbridges')

#     # Step 4: convert bridge sprites to bridge array
    
#     for ar in nmap.areas:
#         for a in ar.actors:
#             if a.picnum == 2:
#                 if a.hitag in bridges[ar.mn]:
#                     if bridges[ar.mn][a.hitag] not in ar.bridges:
#                         ar.bridges.append(bridges[ar.mn][a.hitag]);
#                         ar.defaultcanjoin=True
#                 else:
#                     print "warning: bridge %d,%d not found!"%(ar.mn,a.hitag)
#             if a.picnum == 4:
#                 ar.canjoin=(a.lotag == 0)
                
#     for ar in nmap.areas:
#         if ar.canjoin is None:
#             ar.canjoin = ar.defaultcanjoin

#     hook('postconvertbridges')

#     hook('presplit')
#     # Step 6: split areas at bridges

#     tm.mark()

#     newareas=[]
#     for ar in nmap.areas:
#         newareas.extend(splitbridges(nmap,ar))
#     nmap.areas=newareas
#     tm.report("split bridges")

#     hook('postsplit')



    
    
#     # Step 8: join adjacent areas

#     tm.mark()
#     for ar in nmap.areas:
#         if ar.canjoin and not hasattr(ar,'replace'):
#             while True:
#                 for w in ar.walls:
#                     if w.links and canjoinarea(ar,getreplace(w.links.keys()[0])):
#                         joinareas(ar,w.links.keys()[0])
#                         break
#                 else:
#                     break

#     # Step 9: remove areas that were joined

#     newareas=[]
#     for ar in nmap.areas:
#         if not (hasattr(ar,'replace')):
#             newareas.append(ar)
#     nmap.areas=newareas;
            

#     for ar in nmap.areas:
#         for w in ar.walls:
#             nlinks={}
#             for oa,fl in w.links.items():
#                 nlinks[getreplace(oa)]=fl
#             w.links=nlinks

#     tm.report("join")











GRIDISCL=1.0/4.0
GRIDSCL=4.0

def wallsint(a1, a2):
        
    for w1 in a1.walls:
        for w2 in a2.walls:
            if (w1.x1 == w2.x1 and w1.y1 == w2.y1) or \
               (w1.x2 == w2.x2 and w1.y2 == w2.y2) or \
               (w1.x1 == w2.x2 and w1.y1 == w2.y2) or \
               (w1.x2 == w2.x1 and w1.y2 == w2.y1):
                continue

            v = w1.intersect(w2)
            if not v: continue

            ix, iy = v
            if (ix == w1.x1 and iy == w1.y1) or \
               (ix == w1.x2 and iy == w1.y2) or \
               (ix == w2.x1 and iy == w2.y1) or \
               (ix == w2.x2 and iy == w2.y2):
                continue
            #print w1.x1, w1.y1, w1.x2, w1.y2, w2.x1, w2.y1, w2.x2, w2.y2, ix, iy
            return True


    return False

def planesint(a1,a2):
    for w in a1.walls + a2.walls:
        fz1 = a1.floor.getz(w.x1, w.y1)
        fz2 = a2.floor.getz(w.x1, w.y1)
        cz1 = a1.ceil.getz(w.x1, w.y1)
        cz2 = a2.ceil.getz(w.x1, w.y1)

        if cz1 > fz2 and cz1 < cz2:
            return True
        if fz1 > fz2 and fz1 < cz2:
            return True
        if cz2 > fz1 and cz2 < cz1:
            return True
        if fz2 > fz1 and fz2 < cz1:
            return True
        
    return False
        

def checkoverlap(nmap):
    grid = {}
    timer.begin("check overlap",len(nmap.areas))

    for ar in nmap.areas:
        ar.bbo=set()
        ar.cx, ar.cy = ar.findpt()
    for ar in nmap.areas:
        timer.progress()
        minx = maxx = miny = maxy = None
        for w in ar.walls:
            if minx is None or w.x1 < minx: minx = w.x1
            if miny is None or w.y1 < miny: miny = w.x1
            if maxx is None or w.x1 > maxx: maxx = w.x1
            if maxy is None or w.y1 < maxy: maxy = w.x1

        ar.bbox = minx, miny, maxx, maxy
        gsx = int(math.floor(minx*GRIDISCL))
        gsy = int(math.floor(miny*GRIDISCL))
        gex = int(math.ceil(maxx*GRIDISCL))
        gey = int(math.ceil(maxy*GRIDISCL))


        for gx in xrange(gsx,gex+1):
            for gy in xrange(gsy,gey+1):
                ll = grid.setdefault((gx,gy),[])
                for oa in ll:
                    ar.bbo.add(oa)
                ll.append(ar)

    timer.progress(0)
    timer.msg('overlap = set([')
    for ar in nmap.areas:
        timer.progress()
        #timer.msg("%s %d"%(ar.id,len(ar.bbo)))
        for oa in ar.bbo:
            if ar in oa.bbo:
                oa.bbo.remove(ar)
            timer.progress(timer.i)
            if planesint(ar, oa) and wallsint(ar, oa):
                
                timer.msg('        "%s", "%s", '%(ar.id, oa.id))
    timer.msg('])')
                        
        
    timer.end()











try:
    from overlap import overlap
    overlap.update(dict((v,k) for k,v in overlap.iteritems()))
except:
    overlap={}







