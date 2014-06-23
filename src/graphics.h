/* -*- c++ -*- */

#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <GL/gl.h>
#include <GL/glu.h>
#include <string>

#include "common.h"
#include "object.h"

#include "engine.h"
#include "vector.h"

#include "resource.h"
#include "tokenizer.h"

#define FL_REPX 1
#define FL_REPY 2
#define FL_MIPS 4
#define FL_FILTER 16
#define FL_REPLACE 8

extern int screenw, screenh;
extern bool screen_fs;

struct Color {
    
    double r, g, b, a;
    Color(double r=1.0,double g=1.0,double b=1.0,double a=1.0) : r(r), g(g), b(b), a(a) { }
    Color(int color) : 
	r(((color >> 16) & 255) / (double)255.0),
	g(((color >> 8) & 255) / (double)255.0),
	b(((color) & 255) / (double)255.0),
	a(((color >> 24) & 255) / (double)255.0) { }

    static double read_color_component(TokenStream& ts);
    Color(TokenStream& ts) {
	r=read_color_component(ts);
	g=read_color_component(ts);
	b=read_color_component(ts);
	a = 1.0;
    }

    void set() const {
	glColor4d(r, g, b, a);
    }
};

namespace color {
    static Color Black(0, 0, 0);
    static Color Red(1, 0, 0);
    static Color Green(0, 1, 0);
    static Color Yellow(1, 1, 0);
    static Color Blue(0, 0, 1);
    static Color Magenta(1, 0, 1);
    static Color Cyan(0, 1, 1);
    static Color White(1, 1, 1);

    static Color LtBlue(.3, .3, 1);

    static Color Clear(0, 0, 0, 0);
};

class GLTexture : public GLResource {
protected:
    GLuint name;

public:
    int w, h;

    GLTexture() : name(0), w(0), h(0) { }

    void alloc() {
	release();
	glGenTextures(1,(GLuint*)&name);
	glBindTexture(GL_TEXTURE_2D, name);
    }
    void release() {
	if (name != 0)
	    glDeleteTextures(1,(GLuint*)&name);
	name = 0;
    }
    void bind() {
	if (name != 0)
	    glBindTexture(GL_TEXTURE_2D, name);
    }

    static void uploadsurface (SDL_Surface * surf, int flags);
};

class FileGLTexture : public GLTexture {
    string filename;
    int flags;

public:
    FileGLTexture(const string& filename, int flags) : filename(filename), flags(flags) { }
    virtual void revalidate();
};

class TextureLoader : public FileResourceLoader {
public:
    TextureLoader(string dirname) : FileResourceLoader(dirname, "") { }
    
    virtual sptr<Resource> load_resource_from_file(string filename);
};

extern void _checkerror (const char* file, int lineno);

#define CHECKERROR() _checkerror(__FILE__, __LINE__);

void setmode(int w, int h, bool fullscreen);

void set_appearance_class(Actor* a, string clas);

void drawrect(double x1, double y1, double x2, double y2);

inline void glVertex(vect3d pt) {
    glVertex3d(pt.x,pt.y,pt.z);
}

class DecorationLoader : public ActorLoader {
    ActorLoader& passthrough;
public:
    DecorationLoader(ActorLoader& passthrough) : passthrough(passthrough) { }
    sptr<Actor> load(string& clas, vect3d loc, Area* area, double ang, DataInput& ds);
};

#endif
