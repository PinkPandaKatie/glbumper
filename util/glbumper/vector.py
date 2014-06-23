import math

class vect3d(object):
    def __init__(self, x=0, y=0, z=0):
        if isinstance(x,tuple):
            self.x = float(x[0])
            self.y = float(x[1])
            self.z = float(x[2])
        elif isinstance(x, vect3d):
            self.x = x.x
            self.y = x.y
            self.z = x.z
        else:
            self.x = float(x)
            self.y = float(y)
            self.z = float(z)
            
            
    def __add__(self, o):
        return vect3d(self.x + o.x, self.y + o.y, self.z + o.z)
    
    def __sub__(self, o):
        return vect3d(self.x - o.x, self.y - o.y, self.z - o.z)
    
    def __mul__(self, o):
        if isinstance(o, vect3d):
            return self.dot(o)
        return vect3d(self.x * o, self.y * o, self.z * o)
    
    def __div__(self,s):
        return vect3d(self.x / s, self.y / s, self.z / d)
    
    def __rdiv__(self,s):
        raise ArithmeticError,"cannot divide by vect"
    
    def __neg__(self):
        return vect3d(-self.x, -self.y, -self.z)
    
    def __pos__(self):
        return vect3d(self.x, self.y, self.z)
    
    def len(self):
        return sqrt(self.x * self.x + self.y * self.y + self.z * self.z)
    __abs__=len
    
    def __eq__(self,o):
        return abs(self.x - o.x) < EPSILON and abs(self.y - o.y) < EPSILON and \
               abs(self.z - o.z) < EPSILON
    
    def __ne__(self,o):
        return not self.__eq__(o)
    
    def dist(self, o):
        return abs(self - o)
    
    def len2(self):
        return self.x * self.x + self.y * self.y + self.z * self.z
    
    def dot(self,o):
        return self.x * o.x + self.y * o.y + self.z * o.z
    
    def cross(self, o):
        return vect3d(self.y * o.z - o.y * self.z, o.x * self.z - self.x * o.z, self.x * o.y - o.x * self.y);
    __xor__ = cross
    
    
    def normal(self):
        l=self.len()        
        return vect3d(self.x / l, self.y / l, self.z / l)
    
    def __str__(self):
        return "<%.2f, %.2f, %.2f>"%(self.x, self.y, self.z)
    
    def __repr__(self):
        return "vect3d(%.2f, %.2f, %.2f)"%(self.x, self.y, self.z)

