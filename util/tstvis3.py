import genmap

class Hooks(object):
    def __init__(self):
        self.map = None

genmap.convert(['tstvis3'],'tstvis3.world',Hooks())
