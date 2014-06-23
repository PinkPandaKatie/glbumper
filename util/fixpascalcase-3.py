import sys,os,re
from pidents2 import pidents

for i in sys.argv[1:]:
    data = file("backup/%s"%i).read()

    for j, k in pidents.iteritems():
        data = re.sub(r'\b' + j + r'\b', k, data)
    
    file(i,'w').write(data)
