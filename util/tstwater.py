import genmap

class Hooks(object):
    def __init__(self):
        self.map = None

genmap.convert(['tstwater'],'tstwater.world',Hooks())
