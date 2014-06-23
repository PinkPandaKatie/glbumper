
import sys
import os
import re

classre = re.compile(r'class\s+\b([A-Z][A-Za-z0-9]+)\b')
pascalre = re.compile(r'\b([A-Z][A-Za-z0-9]*[a-z][A-Za-z0-9]+)\b')

pascalidents=set()
classes=set()

lines = []
for i in sys.argv[1:]:
    data = file(i).read()
    data = re.sub(r'(?s)/\*(.*?)\*/', '', data);
    data = re.sub(r'(?s)\\\n', '', data);
    data = re.sub(r'//(.*?)\n', '', data);
    lines.extend(data.split('\n'))
    pascalidents.update(set(pascalre.findall(data)))
    classes.update(set(classre.findall(data)))
    
idents = pascalidents - classes
for i in sorted(idents):
    l = [j for j in lines if i in j];
    print "'%s', # %s"%(i,''.join(l[:1]))
