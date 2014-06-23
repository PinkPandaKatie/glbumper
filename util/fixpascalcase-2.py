import sys,os,re
from pidents import pidents


for i in pidents:
    def lc(m):
        return '_' + m.group(0).lower()
    ri = re.sub('[A-Z]', lc, i)[1:]
    print "'%s': '%s',"%(i,ri)

