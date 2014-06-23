import genmap

class Hooks(object):
    def __init__(self):
        self.map = None

genmap.convert(['nodetest'],'nodetest.world',Hooks())
