import genmap

class Hooks(object):
    def __init__(self):
        self.map = None

genmap.convert(['tstvis4'],'tstvis4.world',Hooks())
