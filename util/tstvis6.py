import genmap

class Hooks(object):
    def __init__(self):
        self.map = None

genmap.convert(['tstvis6'],'tstvis6.world',Hooks())
