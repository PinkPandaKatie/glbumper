/* -*- c++ -*- */
#ifndef _MODEL_H_
#define _MODEL_H_

#include "tokenizer.h"
#include "vector.h"

#include <map>
#include <vector>

#include <SDL.h>
#include <SDL_image.h>

#include "graphics.h"
#include "engine.h"
#include "resource.h"

class ModelLoader;

class Light {
public:
    Light() : amb(0), diff(0), spec(0) { 
	directional=true;
	ca=1;la=0;qa=0;
    }
    void readFrom(TokenStream& ts);
    void setup(int lightnum);

    vect3d pos;
    bool directional;

    Color amb;
    Color diff;
    Color spec;
    
    double ca,la,qa;
};

class Material : public Object {
public: DECLARE_OBJECT;
    Material() : spec(0) { 
	tex = NULL;
	amb=Color(0.2,0.2,0.2);
	diff=Color(0.8,0.8,0.8);
	shine=0;
    }
    ~Material() {
    }
    void readFrom(TokenStream& ts);
    void setup(DrawManager& dm);
    sptr<Texture> tex;
    Color amb;
    Color diff;
    Color spec;
    double shine;
    vector<Light> lights;
};

class Vertex {
public:
    Vertex() { }
    Vertex(vect3d loc, vect3d norm, vect2d uv, Color col) : loc(loc), norm(norm), c(col), uv(uv) { }
    vect3d loc,norm;
    Color c;
    vect2d uv;
    ~Vertex() {
	
    }
};

class Face {
public:
    Face() {
    }
    void draw(DrawManager& dm, bool wireframe, float shade);
    vector<Vertex> vtxv;
};

class Mesh : public Object {
public: DECLARE_OBJECT;
    Mesh() {
    }
    void draw(DrawManager& dm, bool wireframe, float shade);
    sptr<Material> material;
    vector<Face> facev;
    
};

#define NMODCACHE 12
struct CacheRec {
    CacheRec() : shade(INFINITY), wireframe(false) { }
    float shade;
    bool wireframe;
};

class ModelDisplayList : public GLResource {
public:
    CacheRec cache[NMODCACHE];
    int cachehead;
    GLuint base;

    ModelDisplayList() : cachehead(0), base(0) { }
    void release() {
	if (base != 0)
	    glDeleteLists(base, NMODCACHE);
	base = 0;
	for (int i = 0; i < NMODCACHE; i++) {
	    cache[i].shade = INFINITY;
	}
    }
    void alloc() {
	release();
	base = glGenLists(NMODCACHE);
    }
    void compile(int n) {
	glNewList(base + n, GL_COMPILE);
    }
    void call(int n) {
	glCallList(base + n);
    }
    void revalidate() {
	if (!screenw) return;
	alloc();
    }
};

class Model : public Resource {
    ModelDisplayList dlist;
    vector<sptr<Mesh> > meshes;
    vector<sptr<Material> > materials;

    friend class ModelLoader;

    Model() { }

public: DECLARE_OBJECT;
    void draw(DrawManager& dm, bool wireframe=false, float shade=1.0);
    ~Model() {
	cout << "delete model "<<this<<endl;
    }
    
    static string loaderclass;
};

class ModelLoader : public FileResourceLoader {
public:
    ModelLoader(string dirname) : FileResourceLoader(dirname, "") { }
    sptr<Resource> load_resource_from_file(string filename);
};

#endif
