// -*- c++ -*-

#ifndef _VECT2D_H_
#define _VECT2D_H_

#ifndef EPSILON
#define EPSILON (1.0/1048576.0)
#endif

#include <ostream>
class vect2d;
class vect3d;

class vect2d { 
public:
    double x, y;
	
    vect2d(double x = 0, double y = 0);
    vect2d(const vect3d& v);

    vect2d& normalize();

    vect2d& operator+=(const vect2d& o);
    vect2d& operator-=(const vect2d& o);

    vect2d& operator=(const vect3d& o);

    vect2d& operator*=(const double& o);
    vect2d& operator/=(const double& o);
};

class vect3d { 
public:
    double x, y, z;
	
    vect3d(double x = 0, double y = 0, double z = 0);
    vect3d(const vect2d& v);

    vect3d& normalize();

    vect3d& operator+=(const vect3d& o);
    vect3d& operator-=(const vect3d& o);

    vect3d& operator=(const vect2d& o);

    vect3d& operator*=(const double& o);
    vect3d& operator/=(const double& o);

};

inline vect2d::vect2d(double x, double y) : x(x), y(y) { }
inline vect2d::vect2d(const vect3d& v) : x(v.x), y(v.y) { }

// vect2d gets promoted to vect3d for these operations.
// The compiler should optimize away the unneeded operation on the z coordinate.

inline vect3d operator+(const vect3d& a,const vect3d& b) { return vect3d(a) += b; }
inline vect3d operator-(const vect3d& a,const vect3d& b) { return vect3d(a) -= b; }

inline double operator*(const vect3d& a,const vect3d& b) {
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
inline vect3d operator*(const vect3d& a,const double o) {
    return vect3d(a)*=o;
}
inline vect3d operator*(const double o, const vect3d& a) {
    return vect3d(a)*=o;
}
inline vect3d operator/(const vect3d& a,const double o) {
    return vect3d(a)/=o;
}

inline vect2d operator-(const vect2d& a) {
    return vect2d(-a.x,-a.y);
}
inline vect2d operator+(const vect2d& a) {
    return a;
}

inline vect3d operator-(const vect3d& a) {
    return vect3d(-a.x,-a.y,-a.z);
}
inline vect3d operator+(const vect3d& a) {
    return a;
}

inline vect3d operator/(const vect3d& a, const vect3d& b) {
    return (a*(a*b));
}

inline bool operator==(const vect3d& a, const vect3d& b) {
    return fabs(a.x-b.x)<EPSILON && fabs(a.y-b.y)<EPSILON && fabs(a.z-b.z)<EPSILON;
}
inline bool operator!=(const vect3d& a, const vect3d& b) {
    return !(a == b);
}
inline vect2d operator~(const vect2d& a) {
    return vect2d(-a.y,a.x);
}
inline vect3d operator^(const vect3d& a, const vect3d& b) {
    return vect3d(a.y*b.z - b.y*a.z, b.x*a.z - a.x*b.z, a.x*b.y - b.x*a.y);
}
inline const double vlen2(const vect3d& v) {
    return v*v;
}

inline const double vlen(const vect3d& v) {
    return sqrt(vlen2(v));
}

inline vect3d vnormal(const vect3d& a) {
    return vect3d(a).normalize();
}

inline double atan2(const vect2d& a) {
    return atan2(a.y,a.x);
}

inline vect2d& vect2d::normalize()  {
    double l=1.0/vlen(*this);
    x *= l;
    y *= l;
    return *this;
}

inline vect2d& vect2d::operator=(const vect3d& o) {
    x = o.x;
    y = o.y;
    return *this;
}
inline vect2d& vect2d::operator+=(const vect2d& o) {
    x += o.x;
    y += o.y;
    return *this;
}
inline vect2d& vect2d::operator-=(const vect2d& o) {
    x -= o.x;
    y -= o.y;
    return *this;
}
inline vect2d& vect2d::operator*=(const double& o) {
    x *= o;
    y *= o;
    return *this;
}
inline vect2d& vect2d::operator/=(const double& o) {
    x /= o;
    y /= o;
    return *this;
}


inline vect3d::vect3d(double x, double y, double z) : x(x), y(y), z(z) { }
inline vect3d::vect3d(const vect2d& v) : x(v.x), y(v.y), z(0) { }

inline vect3d& vect3d::normalize()  {
    double l=1.0/vlen(*this);
    x *= l;
    y *= l;
    z *= l;
    return *this;
}
inline vect3d& vect3d::operator=(const vect2d& o) {
    x = o.x;
    y = o.y;
    z = 0;
    return *this;
}
inline vect3d& vect3d::operator+=(const vect3d& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
}
inline vect3d& vect3d::operator-=(const vect3d& o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
}
inline vect3d& vect3d::operator*=(const double& o) {
    x *= o;
    y *= o;
    z *= o;
    return *this;
}
inline vect3d& vect3d::operator/=(const double& o) {
    x /= o;
    y /= o;
    z /= o;
    return *this;
}




inline std::ostream& operator<<(std::ostream& s, const vect2d& v) {
    s << '<' << v.x << ", " << v.y << '>';
    return s;
}
inline std::ostream& operator<<(std::ostream& s, const vect3d& v) {
    s << '<' << v.x << ", " << v.y << ", " << v.z << '>';
    return s;
}






#endif

