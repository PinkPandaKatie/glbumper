#include "common.h"
#include "graphics.h"
#include "sound.h"
#include "model.h"
#include "engine.h"
#include "profile.h"
#include "tokenizer.h"

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <deque>

int screenw = 0, screenh = 0;
bool screen_fs = false;

// The GLUtesselator used to tesselate areas. Initialized by init_engine(). 
static GLUtesselator* tesselator = NULL;
bool hasgraph = true;

typedef sptr<Appearance> (*CreateFunc)(Actor* act);

static map<string,CreateFunc> classes;

int register_create_func(const char* s, CreateFunc cf) {
    classes.insert(pair<string,CreateFunc>(string(s),cf));
    return 0;
}

#define registerclass(name,clas)                                                \
static sptr<Appearance> Create##clas(Actor* act) {                              \
    return GC::track(new clas(act));                                            \
}                                                                               \
static int __dummy_create_##clas = register_create_func(#name,Create##clas)








class Stencilable {
public:
    virtual ~Stencilable() { }
    virtual void stencil(DrawManager& dm)=0;
};


class DrawManager {
    map<Stencilable*, int> stencils;
    
    int cmode;
    void setmode(int mode) {
        if (mode != cmode) {
            cmode = mode;
            switch(mode) {
            case 1:
                if (stencils.empty()) {
                    glStencilMask(0);
                    glDisable(GL_STENCIL_TEST);
                } else {
                    glStencilMask(0xff);
                    glStencilFunc(GL_ALWAYS, 0, 0xff);
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                    glEnable(GL_STENCIL_TEST);
                }
                glColorMask(1, 1, 1, 1);
                glDepthMask(1);
                glEnable(GL_TEXTURE_2D);
                glEnable(GL_DEPTH_TEST);
                break;
            case 2:
                glStencilMask(0xFF);
                glDisable(GL_TEXTURE_2D);
                glColorMask(0, 0, 0, 0);
                glDepthMask(1);
                glEnable(GL_DEPTH_TEST);
                glEnable(GL_STENCIL_TEST);
                break;
            case 3:
                glStencilMask(0);
                glColorMask(1, 1, 1, 1);
                glDepthMask(0);
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_TEXTURE_2D);
                glEnable(GL_STENCIL_TEST);
                break;
            }
        }
    }

public:

    vect3d viewpt, forward, up, right;
    sptr<Area> viewarea;
    deque< sptr<Appearance> > drawqueue;

    DrawManager() : cmode(0) { }

    void normal() {
        setmode(1);
    }
    void stencil(Stencilable* st) {
        int stval;
        map<Stencilable*, int>::iterator itr = stencils.find(st);
        if (itr == stencils.end()) {
            stval = stencils.size() + 1;
            stencils[st] = stval;
        } else {
            stval = itr->second;
        }
        setmode(2);
        glStencilFunc(GL_ALWAYS, stval, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }
    void dostencils() {
        setmode(3);
        FOREACH(itr, stencils) {
            glStencilFunc(GL_EQUAL, itr->second, 0xff);
            itr->first->stencil(*this);
        }
        setmode(1);
    }

};






double Color::read_color_component(TokenStream& ts) {
    Token t;
    ts.nextToken(t);
    if (t.type == TT_INTEGER)
	return t.ival/255.0;
    if (t.type == TT_FLOAT)
	return t.dval;
    return 0;
}

#define DRAW_FLIPX 1
#define DRAW_FLIPY 2

void _checkerror (const char* file, int line)
{
    GLenum errCode;

    if ((errCode = glGetError ()) != GL_NO_ERROR) {
	const GLubyte *errString;
	errString = gluErrorString (errCode);
	fprintf (stderr, "OpenGL error detected at %s:%d: %s\n", file, line, errString);
	SDL_Quit ();
	exit (1);
    }
}

void WallPart::draw(DrawManager& dm, double x1, double y1, double jx, double jy, double len) {

    /*
    double fza = bot->getz (x1, y1);
    double cza = top->getz (x1, y1);
    double fzb = bot->getz (x1+len*jx, y1+len*jy);
    double czb = top->getz (x1+len*jx, y1+len*jy);
    */

    if (drawmode == DRAWMODE_TEXTURE) {
	if (tex == NULL) return;
	tex->setup(dm);
    }
    if (cza <= fza && czb <= fzb)
	return;


    double isx = 1.0 / scale.x;
    double isy = 1.0 / scale.y;
#define doVertex(wx,wz) ({						\
    glTexCoord2d(((wx * isx + pan.x) * (flags & DRAW_FLIPX ? -1 : 1)),	\
		 ((wz * isy - pan.y) * (flags & DRAW_FLIPY ? 1 : -1)));	\
									\
    glVertex3d(x1 + jx * wx,y1 + jy * wx, wz);				\
})

    if (cza > fza && czb > fzb) {
	glBegin (GL_QUADS);

	doVertex(0,cza);
	doVertex(len,czb);
	doVertex(len,fzb);
	doVertex(0,fza);
	
	glEnd ();

    } else {
	// Special magic to handle intersecting slopes.
	
	double jp = -((len / ((czb - fzb) - (cza - fza))) * (cza - fza));
	//double jxp = x1 + jx * (jp * len);
	//double jyp = y1 + jy * (jp * len);
	double jzp = (jp / len) * (fzb - fza) + fza;
	glBegin(GL_TRIANGLES);

	doVertex(jp,jzp);

	if (cza > fza) {
	    doVertex(0,fza);
	    doVertex(0,cza);
	} else {
	    doVertex(len,czb);
	    doVertex(len,fzb);
	}

	glEnd();
    }
}
#undef doVertex


void Wall::draw(DrawManager& dm) {
    //printf("draw %d\n",parts.size());
    if (drawmode != DRAWMODE_TEXTURE && drawmode != DRAWMODE_WIREFRAME) {
	glColor3f(0,0,1);
    }

    if (drawmode != DRAWMODE_WIREFRAME) {
	FOREACH(part,parts) {
	    part->draw(dm,v1.x,v1.y,j.x,j.y,len);
	}
    }
    if (drawmode != DRAWMODE_TEXTURE) {
	glColor3f(1,1,1);
	glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	glLineWidth (2.5);
	FOREACH(part,parts) {
	    part->draw(dm, v1.x,v1.y,j.x,j.y,len);
	}
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    }
}
void Area::draw(DrawManager& dm, bool dwalls) {

    if (drawmode)
	glDisable (GL_TEXTURE_2D);
    glColor3f (1, 1, 1);
    glFrontFace (GL_CW);
    if (!(nowalls & 1) && dwalls) {
	FOREACHW(w,this) {
	    w->draw(dm);
	}
    }
    if (!(nowalls & 2)) {
	if (draweverything || dwalls || dm.viewpt.z > floor->getz(dm.viewpt))
	    floor->draw(dm, this);
	
	
	glFrontFace (GL_CCW);
	if (draweverything || dwalls || dm.viewpt.z < ceil->getz(dm.viewpt))
	    ceil->draw(dm, this);
    }






}

void Plane::draw(DrawManager& dm, Area* area) {
    PROBEGINF;

    if (tex == NULL) return;

    double cz = cpt.z;
    int robustcnt=(flags&16?2:1);
    while (robustcnt--) {
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (matrix)
	    glMultMatrixd(matrix);

	glTranslated(0,0,cz);


	switch (drawmode) {
	case DRAWMODE_TEXTURE:
	    glEnable(GL_TEXTURE_2D);
	    //glColor3d(1,1,1);
	    setup_texture_matrix(tex);
	    tex->setup(dm);

	    break;
	case DRAWMODE_SOLID_OUTLINE_WALLS:
	case DRAWMODE_SOLID_OUTLINE_AREABOUNDS:
	case DRAWMODE_SOLID_OUTLINE_AREATESS:
	    glDisable(GL_TEXTURE_2D);
	    glColor3d(0.5,0.5,0.5);

	    break;
	}

	switch (drawmode) {
	case DRAWMODE_TEXTURE:
	case DRAWMODE_SOLID_OUTLINE_WALLS:
	    if (area->tess) area->tess->call();
	    break;
	case DRAWMODE_SOLID_OUTLINE_AREABOUNDS:
	    if (area->tess) area->tess->call();
	    glColor3d(1,1,1);
	    glBegin(GL_LINES);
	    FOREACHW(ww,area) {
		glVertex2d(ww->v1.x,ww->v1.y);
		glVertex2d(ww->next->v1.x,ww->next->v1.y);
	    }
	    glEnd();

	    break;
	case DRAWMODE_SOLID_OUTLINE_AREATESS:
	    if (area->tess) area->tess->call();
	    glColor3d(1,1,1);
	    glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	    glLineWidth (2.5);
	    if (area->tess) area->tess->call();
	    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	    break;
	case DRAWMODE_WIREFRAME: 
	    glLineWidth (2.5);
	    glColor3d(1,1,1);
	    glBegin(GL_LINES);
	    FOREACHW(ww,area) {
		glVertex2d(ww->v1.x,ww->v1.y);
		glVertex2d(ww->next->v1.x,ww->next->v1.y);
	    }
	    glEnd();
	
	    break;
	}


	glMatrixMode(GL_TEXTURE);
	glLoadIdentity(); // restore texture matrix

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();    // restore modelview matrix

	cz += ceil?0.01:-0.01;
    }
    PROENDF;


}


// Used for sorting Appearances.
inline bool cmp_drawable (const sptr<Appearance>& a, const sptr<Appearance>& b)
{
    return a->sortz > b->sortz;
}

void World::drawview (vect3d pt, Area* area, double ang, double vofs,
		      double xfov, double yfov, ActorFilter& filt)
{
    unsigned char aseen[area_bit_size];

    if (draweverything || area == NULL) {
	memset (aseen,0xff,area_bit_size);
    } else {
	trace (pt, area, ang, (vofs?0:atan2(fabs(xfov), 1.0)), aseen);
    }

    DrawManager dm;

    double s = sin (ang);
    double c = cos (ang);

    dm.viewpt = pt;
    dm.forward = vnormal(vect3d(c, s, vofs));
    dm.right = vnormal(dm.forward ^ vect3d(0, 0, 1));
    dm.up = vnormal(dm.right ^ dm.forward);
    dm.viewarea = area;

    if (drawmode == DRAWMODE_TEXTURE) {
	glDisable (GL_POLYGON_OFFSET_FILL);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
    } else {
	glLineWidth (2.5);
	glEnable (GL_POLYGON_OFFSET_FILL);
	glDisable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
    }
    glDisable(GL_LIGHTING);
    glDepthMask(1);
    
    glPolygonOffset (2, 0);

    glColor3f (1.0, 1.0, 1.0);
    
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    CHECKERROR();
    GLdouble xmin, xmax, ymin, ymax;
    xmax = xfov*2.0 * MASTER_SCALE;
    xmin = -xmax;
    
    ymin = -yfov*2.0 * MASTER_SCALE;
    ymax = -ymin;
    
    glFrustum (xmin, xmax, ymin, ymax, 0.03125, draweverything? 1024.0 : 512.0);
    
    CHECKERROR();
    
    if (xfov*yfov>0)
	glCullFace (GL_BACK);
    else
	glCullFace (GL_FRONT);

    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    gluLookAt (pt.x, pt.y, pt.z, pt.x + c, pt.y + s, pt.z + vofs, 0, 0, 1);
    glColor3f (1, 1, 1);

    int nareas = areas.size();
    for (int i = 0; i < nareas; i++) {
	if (ISSET(aseen, i)) {
	    areas[i]->draw(dm, true);
	}
    }

    dm.dostencils();

    glLineWidth (1);

    multimap<double, sptr<Appearance> > willdraw;

    for (int i = 0; i < 2; i++) {
	vector< sptr<Actor> >& clst(i ? staticactors : actors);
	FOREACH(titr, clst) {
	    Actor* act = *titr;
	    if (filt.filter(act) && act->appearance != NULL) {
		if (act->is_static) {
		    act->appearance->willdraw = ISSET(aseen, act->area->id);
		} else {
		    act->appearance->willdraw = false;
		    FOREACH(itr, act->areas) {
			if (ISSET(aseen,(*itr)->id)) {
			    act->appearance->willdraw = true;
			    break;
			}
		    }
		}
		if (act->appearance->willdraw) {
                    double sortz = (act->loc.x - pt.x) * c + (act->loc.y - pt.y) * s;
                    if (!act->appearance->needsort)
			 sortz = INFINITY;
		    willdraw.insert(pair<double, sptr<Appearance> >(-sortz, act->appearance));
		}
	    }
	}
    }
    FOREACH(itr, willdraw) {
        itr->second->draw(dm);
    }
    while (!dm.drawqueue.empty()) {
        (*dm.drawqueue.begin())->draw(dm);
        dm.drawqueue.pop_front();
    }

    
    
}


// Tesselation callbacks.

// A vertex to pass to gluTessVertex. A four-dimensional vertex may be
// required, so include a dummy coordinate just in case.
struct tess_vertex {
    double x,y,z,dum;
    tess_vertex(double x = 0,double y = 0,double z = 0) : x(x),y(y),z(z),dum(0) { }
};

// On Unix, the OpenGL library is written in C and uses (gasp) the C
// calling convention! Who would've ever guessed? On Win32, for some
// odd reason, OpenGL calls use the Pascal calling convention, so our
// callbacks must use the same convention. But CALLBACK isn't defined
// on most Unix systems, so if it isn't defined, define it to nothing.

#ifndef CALLBACK
#define CALLBACK
#endif

// See the man page for gluTessCallback for details.

static CALLBACK void tesscb_vertex (tess_vertex* vtx) {
    glTexCoord2d(vtx->x,vtx->y);
    glVertex3dv((GLdouble*)vtx);
}

// Creates a new vertex when the tesselator determines an intersection
// has occured. This shouldn't normally be called if you design your
// world correctly.

static CALLBACK void tesscb_combine(GLdouble coords[3], tess_vertex *d[4], GLfloat w[4], tess_vertex **dataOut, vector<tess_vertex> *vlst) {
    fprintf(stderr,"warning: tesselation combine\n");
    vlst->resize(vlst->size()+1);
    *dataOut=&*(vlst->end()-1);
    (*dataOut)->x = coords[0];
    (*dataOut)->y = coords[1];
    (*dataOut)->z = coords[2];
}

// This should probably do more, but if you're getting tesselation
// errors on your world, you probably goofed somewhere.

static void tesscb_error(GLenum errno)  {
    fprintf(stderr,"warning: tesselation error: %s\n",gluErrorString(errno));
}

void AreaTess::call() {
    if (name != 0)
	glCallList(name);
}

void AreaTess::release() {
    if (name != 0)
	glDeleteLists(name, 1);
    name = 0;
}

void AreaTess::revalidate() {
    if (!screenw) return;
    if (name != 0) return;
    name = glGenLists(1);
    unsigned int i;

    vector<tess_vertex> vlst;
    
    FOREACHW(w,area) {
	vlst.push_back(tess_vertex(w->v1.x,w->v1.y,0));
    }

    glNewList(name,GL_COMPILE);

    gluTessBeginPolygon(tesselator,&vlst);
    gluTessBeginContour(tesselator);

    i = 0;
    FOREACHW(w,area) {
	gluTessVertex(tesselator,(GLdouble*)&vlst[i],&vlst[i]);
	if (w->nextcontour != NULL && w->next != w->nextcontour) {
	    gluTessEndContour(tesselator);
	    gluTessBeginContour(tesselator);
	}
	i++;
    }

    gluTessEndContour(tesselator);
    gluTessEndPolygon(tesselator);

    glEndList();

    CHECKERROR();
    
}

static double swapmatrix[16] = {
    0.0, 1.0, 0.0, 0.0,
    1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0};

void Plane::setup_texture_matrix(Texture* ti) {
    GLdouble m[16];

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    glTranslated(pan.x,pan.y,0);
    glScaled(1.0 / scale.x, 1.0 / scale.y,0);

    //glScaled(1.0/64.0/MASTER_SCALE, 1.0/64.0/MASTER_SCALE, 1.0);
    
    
    if (flags&1) {
	glScaled (1.0,-1.0,1.0);
    }
    if (flags&2) {
	glScaled (-1.0,1.0,1.0);
    }
    if (flags&4) {
	glMultMatrixd(swapmatrix);
    }
    
    if (flags&8) {
	m[0]  = dir.y;
	m[1]  = -dir.x;
	m[2]  = 0.0;
	m[3]  = 0.0;
	
	m[4]  = -dir.x;
	m[5]  = -dir.y;;
	m[6]  = 0.0;
	m[7]  = 0.0;
	
	m[8]  = 0.0;
	m[9]  = 0.0;
	m[10] = 1.0;
	m[11] = 0.0;
	
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = 0.0;
	m[15] = 1.0;

	glMultMatrixd(m);
	
	glTranslated(-cpt.x,-cpt.y,0);
    }
}




#ifndef GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

static int anisotropic = 1;

static void set_anisotropic (void) {
    CHECKERROR ();

    GLfloat aniso;
    glGetFloatv (GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    if (glGetError () != GL_NO_ERROR) {
	fprintf (stderr,
		 "Warning: anisotropic filtering not supported by driver, using trilinear filtering.\n");
	anisotropic = 0;
    }
}

void GLTexture::uploadsurface(SDL_Surface *surf, int flags) {
    SDL_Surface *conv = NULL;
    SDL_PixelFormat* f = surf->format;
#define pack(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

    Uint32 ssig = pack(f->Ashift, f->Rshift, f->Gshift, f->Bshift);

    int format = 0;
    if (f->BytesPerPixel == 4) {
	
	switch (ssig) {
	case pack(24, 0, 8, 16):
	    format = GL_RGBA;
	    break;
	case pack(24, 16, 8, 0):
	    format = GL_BGRA;
	    break;
	}
    } else if (f->BytesPerPixel == 3) {
	switch (ssig) {
	case pack(0, 0, 8, 16):
	    format = GL_RGB;
	    break;
	case pack(0, 16, 8, 0):
	    format = GL_BGR;
	    break;
	}
    }

    int align = 0;
    int expectpitch = surf->w*f->BytesPerPixel;

    if (expectpitch == surf->pitch) align = 1;
    else if (((expectpitch+1)&~1) == surf->pitch) align = 2;
    else if (((expectpitch+3)&~3) == surf->pitch) align = 4;
    //else if (((expectpitch+7)&~7) == surf->pitch) align = 8;

    //fprintf(stderr,"%d %d %d %d %d\n",align, format, surf->pitch, surf->w, expectpitch);
    if (format == 0 || align == 0) {
	SDL_SetAlpha(surf,0,255);
	conv =
	    SDL_CreateRGBSurface (SDL_SWSURFACE, surf->w, surf->h, 32, 0x000000FF,
				  0x0000FF00, 0x00FF0000, 0xFF000000);
	SDL_BlitSurface (surf, NULL, conv, NULL);
	
	surf = conv;
	format = GL_RGBA;
	align = 1;
    }
    glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei (GL_UNPACK_ALIGNMENT, align);

    CHECKERROR();

    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (flags & FL_REPX) ? GL_REPEAT:GL_CLAMP_TO_EDGE);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (flags & FL_REPY) ? GL_REPEAT:GL_CLAMP_TO_EDGE);
    if (flags & FL_FILTER)
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    else
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    CHECKERROR();
    if (flags & FL_REPLACE) {
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, 
			format, GL_UNSIGNED_BYTE, surf->pixels);
	CHECKERROR();
    } else {
	if (flags & FL_FILTER)
	    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & FL_MIPS)?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
	else
	    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	    
	if (flags & FL_MIPS) {
	    if (anisotropic)
		set_anisotropic ();
	    gluBuild2DMipmaps (GL_TEXTURE_2D, GL_RGBA, surf->w, surf->h,
			       format, GL_UNSIGNED_BYTE, surf->pixels);
	    CHECKERROR();
	} else {
	    if (surf->w == powerof2(surf->w) && surf->h == powerof2(surf->h)) {
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0,
			      format, GL_UNSIGNED_BYTE, surf->pixels);
		CHECKERROR();
	    } else {
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, powerof2(surf->w), 
			      powerof2(surf->h), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surf->w, surf->h, 
				format, GL_UNSIGNED_BYTE, surf->pixels);
		CHECKERROR();
	    }
	}
    }

    if (conv) SDL_FreeSurface (conv);
}

void FileGLTexture::revalidate() {
    if (!screenw) return;

    alloc();
    SDL_Surface *surf = IMG_Load (filename.c_str());
    if (!surf)
	throw ios_base::failure(string("cannot open file: ") + filename);
    if (flags & FL_MIPS) {
	w = surf->w;
	h = surf->h;
    } else { 
	w = powerof2(surf->w);
	h = powerof2(surf->h);
    }
    uploadsurface(surf, flags);
    SDL_FreeSurface(surf);
}



class StaticTexture : public Texture {
public:
    GLTexture* tex;

    StaticTexture(GLTexture* tex) : tex(tex) {
	w = tex->w;
	h = tex->h;
    }
    void setup(DrawManager& dm) {
        dm.normal();
	glEnable(GL_TEXTURE_2D);
	tex->bind();
    }
    ~StaticTexture() {
	delete tex;
    }
    DECLARE_OBJECT;
};
#include objdef(StaticTexture)

class TestStencil : public Texture, public Stencilable {
public:
    Color col[4];

    TestStencil(TokenStream& ts) {
        for (int i = 0; i < 4; i++) {
            col[i] = Color(ts);
        }
    }
    void setup(DrawManager& dm) {
        dm.stencil(this);
    }
    void stencil(DrawManager& dm) {
        
	//return;
	bool cull = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);

	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D (0, 1, 1, 0);

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
        col[0].set();
	glVertex2d(0,0); 
        col[1].set();
        glVertex2d(0,1);
        col[2].set();
	glVertex2d(1,1); 
        col[3].set();
        glVertex2d(1,0);
	glEnd();

	glPopMatrix();

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	
	if (cull) glEnable(GL_CULL_FACE);
        glEnable(GL_TEXTURE_2D);
    }
    ~TestStencil() {
    }
    DECLARE_OBJECT;
};
#include objdef(TestStencil)


#define skh 32.0
#define skl -32.0

#define sklfd  { skl, skl, skl }
#define sklbd  { skl, skl, skh }
#define sklfu  { skl, skh, skl }
#define sklbu  { skl, skh, skh }
#define skrfd  { skh, skl, skl }
#define skrbd  { skh, skl, skh }
#define skrfu  { skh, skh, skl }
#define skrbu  { skh, skh, skh }

static GLdouble skywind[6][4][3] = {
    {sklfu, skrfu, skrfd, sklfd},
    {skrfu, skrbu, skrbd, skrfd},
    {skrbu, sklbu, sklbd, skrbd},
    {sklbu, sklfu, sklfd, sklbd},
    {sklbu, skrbu, skrfu, sklfu},
    {sklfd, skrfd, skrbd, sklbd}
};
static GLdouble skytexwind[4][2] = {
    {0.0, 0.0},
    {1.0, 0.0},
    {1.0, 1.0},
    {0.0, 1.0}
};

class SkyTexture : public Texture, public Stencilable {
public:
    GLTexture* tex[6];
    int rotate[6];
    SkyTexture(string dirname, TokenStream& ts) {
        Token t;
        for (int i = 0; i < 6; i++) {
            string fn = ts.readString(ST_STRING);
            rotate[i] = ts.readInt();
            string nfn = join_path(dirname, fn);
            tex[i] = new FileGLTexture(nfn, FL_FILTER | FL_MIPS);
            tex[i]->revalidate();
        }
    }
    void setup(DrawManager& dm) {
        dm.stencil(this);
    }
    void stencil(DrawManager& dm) {
        
        glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_CULL_FACE);
       


	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
        gluLookAt(0, 0, 0, 
                  dm.forward.x, dm.forward.y, dm.forward.z,
                  dm.up.x, dm.up.y, dm.up.z);
        glRotated(90, 1, 0, 0);

	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	GLdouble xmin, xmax, ymin, ymax;
	xmax = 1.0;
	xmin = -xmax;
	
	ymin = -xmax*0.75;
	ymax = -ymin;
        

	glFrustum (xmin, xmax, ymin, ymax, 1.0, 128.0);

        int txn, vn, cv;
        glDepthMask (0);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glColor4d (1.0, 1.0, 1.0, 1.0);
        for (txn = 0; txn < 6; txn++) {
            tex[txn]->bind();
            glBegin (GL_QUADS);
            for (vn = 0; vn < 4; vn++) {
                glTexCoord2dv (skytexwind[vn]);
                cv=rotate[txn]&4 ? 3-vn : vn;
                glVertex3dv (skywind[txn][(cv + rotate[txn])&3]);
            }
            glEnd ();
            
        }


	glPopMatrix();

	glMatrixMode (GL_MODELVIEW);
	glPopMatrix();
	
        glPopAttrib();
    }
    ~SkyTexture() {
    }
    DECLARE_OBJECT;
};
#include objdef(SkyTexture)

sptr<Resource> TextureLoader::load_resource_from_file(string filename) {
    string dn = dirname(filename);
    ifstream fis(filename.c_str(), ios::in);
    if (!fis)
	return NULL;

    Token t;
    StreamTokenReader ts(fis);

    ts.expectToken(t, TT_SYMBOL);
    string clas = t.sval;
    if (clas == "static") {
        ts.expectToken(t, TT_STRING);
	GLTexture* gtex = new FileGLTexture(join_path(dn, t.sval), FL_REPX | FL_REPY | FL_FILTER | FL_MIPS);
	gtex->revalidate();
	return GC::track(new StaticTexture(gtex));
    } else if (clas == "teststencil") {
	return GC::track(new TestStencil(ts));
    } else if (clas == "sky") {
	return GC::track(new SkyTexture(dn, ts));

    }
    
    return NULL;
}

void drawrect(double x1, double y1, double x2, double y2) {
    glBegin(GL_QUADS);
    glVertex2d(x1,y1); glVertex2d(x1,y2);
    glVertex2d(x2,y2); glVertex2d(x2,y1);
    glEnd();
    CHECKERROR();
}

/*
void TextureManager::BeginDraw() {
    stencilmap.clear();
    stencil = 1;
}

int TextureManager::GetStencil(Texture* tex) {
    int val = stencilmap[tex];
    if (!val) {
	val = stencil++;
	stencilmap[tex] = val;
    }
    glStencilFunc(GL_ALWAYS, val, (unsigned)-1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}

void TextureManager::EndDraw() {
    stencilmap.clear();
    stencil = 0;
}

TextureManager::~TextureManager() {
    FOREACH(itr,cache) {
	itr->second->Deref();
    }
    cache.clear();
}

*/
class Decoration : public Actor {
public: DECLARE_OBJECT;
    Decoration() { is_static = true; }

    
};

#include objdef(Decoration)

sptr<Actor> DecorationLoader::load(string& clas, vect3d loc, 
				   Area* area, double ang, DataInput& ds) {

    if (clas != "decoration") return passthrough.load(clas, loc, area, ang, ds);
    sptr<Decoration> d = GC::track(new Decoration());
    
    d->loc = loc;
    d->ang = ang;
    d->area = area;

    string dclass = ds.readstring();
    set_appearance_class(d, dclass);

    if (d->appearance == NULL) return NULL;

    while (!ds.isEOF()) {
	int num = ds.readu8();
	int type = ds.readu8();
	Parameter& p(d->appearance->params[num]);
	switch (type) {
	case PT_STRING:
	    p.sval = ds.readstring();
	    break;
	case PT_INT:
	case PT_ID:
	    p.ival = ds.readu32();
	    p.dval = (double)p.ival;
	    break;
	case PT_FLOAT:
	case PT_FLOAT_NOLERP:
	    p.dval = ds.readf();
	    p.ival = (int)p.dval;
	    break;
	}
	d->appearance->paramchanged(num);
    }
    return d;
}

/**************************************************\
*                                                  *
*                  Appearances                     *
*                                                  *
\**************************************************/


class BaseAppearance : public Appearance {
public: DECLARE_OBJECT;
    BaseAppearance(Actor* a) : Appearance(a), asnd(NULL) { }

    void runsound() {
	if (asnd != NULL) {
	    asnd->setpos(act->loc);
	    asnd->setvol(params[31].dval);
	}
    }
    void paramchanged(int i) {
	if (i == 30) {
	    asnd = NULL;
	    sptr<Sound> s = Resource::get<Sound>(params[30].sval.c_str());
	    if (s != NULL) {
		asnd = GC::track(new AmbientSound(s));
		asnd->start();
	    }
	}
    }
    ~BaseAppearance() { }
    sptr<AmbientSound> asnd;

};

void set_appearance_class(Actor* a, string clas) {
    map<string,CreateFunc>::iterator itr = classes.find(clas);
    if (itr==classes.end())
	a->appearance = NULL;
    else
	a->appearance = (itr->second)(a);
}

#define SF_FLIPX 1
#define SF_FLIPY 2
#define SF_ROTOSCIL 4
#define SF_ANIMOSCIL 8
#define SF_ANIMFLIPOSCIL 16
#define SF_VERTICAL 32

class SpriteAppearance : public BaseAppearance {
public: DECLARE_OBJECT;
    sptr<Texture> tex;

    SpriteAppearance(Actor* act) : BaseAppearance(act) {
	tex = NULL;

	needsort=true;
	


    }



    virtual void paramchanged(int num) {
	if (num == 0) {
	    tex = Resource::get<Texture>(params[0].sval);
	}
	BaseAppearance::paramchanged(num);
    }

    virtual void draw(DrawManager& dm) {

	int flags = params[1].ival;
	double scale = params[2].dval;
	vect2d hotspot(params[3].ival, params[4].ival);

	if (tex == NULL) return;

	vect2d tdiff(act->loc - dm.viewpt);
        vect3d up = dm.up;
	if (flags&SF_VERTICAL) {
	    up = vect3d(0,0,1);
	}

	bool flipx=flags&SF_FLIPX;
	bool flipy=flags&SF_FLIPY;

	tex->setup(dm);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor3d(1,1,1);
	glDepthMask(0);

	vect3d cpt(act->loc);

	vect3d lf(dm.right*(-hotspot.x*scale));
	vect3d rt(dm.right*((tex->w-hotspot.x)*scale));
	vect3d bt(up*((hotspot.y-tex->h)*scale));
	vect3d tp(up*(hotspot.y*scale));

	glBegin(GL_QUADS);

	glTexCoord2d(flipx?0:1,flipy?0:1);
	glVertex(cpt + rt + bt);

	glTexCoord2d(flipx?0:1,flipy?1:0);
	glVertex(cpt + rt + tp);

	glTexCoord2d(flipx?1:0,flipy?1:0);
	glVertex(cpt + lf + tp);

	glTexCoord2d(flipx?1:0,flipy?0:1);
	glVertex(cpt + lf + bt);

	glEnd();

	glDepthMask(1);
    }    
};

registerclass(sprite,SpriteAppearance);


class FlashAppearance : public Appearance {
public: DECLARE_OBJECT;

    double r,g,b,a;

    FlashAppearance(Actor* act, double r, double g, double b, double a) : Appearance(act), r(r),g(g),b(b),a(a) {

    }



    virtual void draw(DrawManager& dm) {
	//cout << "flash draw!"<<endl;
	//return;
	bool cull = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);

	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D (0, 1, 1, 0);

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glColor4d(r, g, b, a);
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
	glVertex2d(0,0); glVertex2d(0,1);
	glVertex2d(1,1); glVertex2d(1,0);
	glEnd();

	glPopMatrix();

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	
	if (cull) glEnable(GL_CULL_FACE);
    }

};


class WaterAppearance : public BaseAppearance {
public: DECLARE_OBJECT;
    sptr<Plane> pl;


    WaterAppearance(Actor* act) : BaseAppearance(act) {
	pl = GC::track(new Plane(true, NULL));
	needsort=true;

    }



    static void setup_clip_plane(GLint cpl, Plane* p) {
	GLdouble pln[4];

	vect3d tpt(p->dir);
	
	tpt.z = p->slope;
	
	vect3d norm = vnormal(vect3d(~p->dir)^tpt);
	
	pln[0] = norm.x;
	pln[1] = norm.y;
	pln[2] = norm.z;
	pln[3] = -(norm * p->cpt);

	if (!p->ceil) {
	    pln[0] = -pln[0];
	    pln[1] = -pln[1];
	    pln[2] = -pln[2];
	    pln[3] = -pln[3];
	}
	

	glClipPlane(cpl,pln);

    }
    virtual void paramchanged(int num) {
	if (num == 0) {
	    pl->tex = Resource::get<Texture>(params[0].sval + ".tex");
	}
	BaseAppearance::paramchanged(num);
    }
    virtual void draw(DrawManager& dm) {
	pl->cpt = act->loc;

	if (dm.viewarea == act->area && dm.viewpt.z < pl->cpt.z) {
	    //cout << "flash!! "<< pt << ' ' << pl->cpt <<endl;
            dm.drawqueue.push_back(GC::track(new FlashAppearance(act, 0, 0.4, 1.0, 0.5)));
	}

	if (pl->tex == NULL) return;
	double alpha = params[1].dval;


	if (drawmode == DRAWMODE_WIREFRAME) return;

	bool cull = glIsEnabled(GL_CULL_FACE);
	glDisable(GL_CULL_FACE);



	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);

	setup_clip_plane(GL_CLIP_PLANE0,act->area->floor);
	setup_clip_plane(GL_CLIP_PLANE1,act->area->ceil);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glColor4d(1,1,1,alpha);
	//cout << "alpha: "<<alpha<<endl;;
	glDepthMask(0);

	pl->draw(dm, act->area);

	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDepthMask(1);
	if (cull) glEnable(GL_CULL_FACE);
    }    
};

registerclass(water,WaterAppearance);


class AreaAppearance : public BaseAppearance {
public: DECLARE_OBJECT;
    AreaAppearance(Actor* act) : BaseAppearance(act) {
	needsort=false;
    }


    
    void draw(DrawManager& dm) {
	
	int groupnum = params[0].ival;
	if (groupnum <= 0) return;
	if (((unsigned)groupnum-1) >= act->world->extragroups.size())
	    return;
	vector<sptr<Area> >& areas(act->world->extragroups.at(groupnum - 1));

	glDepthMask(1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glEnable(GL_NORMALIZE);
	glTranslated(act->loc.x,act->loc.y,act->loc.z);
	
        dm.viewpt -= act->loc;

	FOREACH(area,areas) {
	    (*area)->draw(dm);
	}
        dm.viewpt += act->loc;

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
    }

    ~AreaAppearance() {
    }

};

registerclass(areagroup,AreaAppearance);


class ModelAppearance : public BaseAppearance {
public: DECLARE_OBJECT;
    //ModelAppearance(Actor* act,string src, vect3d ofs=vect3d(), double angofs=0.0, double scale=1.0);
    ModelAppearance(Actor* act);
    void draw(DrawManager& dm);

    

    ~ModelAppearance() {
    }
    void paramchanged(int num);
    vect3d ofs;
    double angofs;
    double scale;
    sptr<Model> mod;
};

registerclass(model,ModelAppearance);

class CylinderAppearance : public BaseAppearance {
public:
    GLUquadric* quad;
    CylinderAppearance(Actor* act) : BaseAppearance(act) {
	needsort=false;
	quad = gluNewQuadric();
    }
    ~CylinderAppearance() {
	gluDeleteQuadric(quad);
    }
    static void dolight(int light, double x, double y, double z, double r=1.0, double g=1.0, double b=1.0) {
        GLfloat p[4];
        glEnable(GL_LIGHT0+light);
        p[0]=x; p[1]=y; p[2]=z; p[3]=1;
        glLightfv(GL_LIGHT0+light,GL_POSITION,p);
        
        p[0]=0.4*r; p[1]=0.4*g; p[2]=0.4*b; p[3]=1;
        glLightfv(GL_LIGHT0+light,GL_AMBIENT,p);
        
        p[0]=0.7*r; p[1]=0.7*g; p[2]=0.7*b; p[3]=1;
        glLightfv(GL_LIGHT0+light,GL_DIFFUSE,p);
    }

    void draw(DrawManager& dm) {
        double rad = params[0].dval;
        double height = params[1].dval;
        double r = params[2].dval;
        double g = params[3].dval;
        double b = params[4].dval;

	bool cull = glIsEnabled(GL_CULL_FACE);
	glEnable(GL_CULL_FACE);
	glFrontFace (GL_CCW);
	glDisable(GL_TEXTURE_2D);


        glDepthMask(1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslated(act->loc.x,act->loc.y,act->loc.z-height);

	glEnable(GL_LIGHTING);
	dolight(0,0,0,height*3,r,g,b);
	dolight(1,0,0,-height*3,r,g,b);
	dolight(2,-rad*8,0,height,r,g,b);
	dolight(3,rad*8,0,height,r,g,b);
	//do_light(4,0,rad*3,height);
	//do_light(5,0,-rad*3,height);

	gluCylinder(quad,rad,rad,height*2,(int)(rad*M_PI*20),2);
	glScaled(-1,1,-1);
	gluDisk(quad,0,rad,(int)(rad*M_PI*20),1);
	glScaled(-1,1,-1);
	glTranslated(0,0,height*2);
	gluDisk(quad,0,rad,(int)(rad*M_PI*20),1);

	glDisable(GL_LIGHTING);
	glPopMatrix();
	if (!cull) glDisable(GL_CULL_FACE);

    }
};

registerclass(cylinder, CylinderAppearance);

ModelAppearance::ModelAppearance(Actor* act) : BaseAppearance(act), mod(NULL) {
    needsort=false;
    
}
void ModelAppearance::paramchanged(int num) {
    if (num == 0) {
	mod = Resource::get<Model>(params[0].sval);
    }
    BaseAppearance::paramchanged(num);
}
extern void dump_gl_state(ostream& o);

void ModelAppearance::draw(DrawManager& dm) {
    if (mod == NULL) return;

    vect3d ofs(params[1].dval, params[2].dval, params[3].dval);
    double angofs = params[4].dval;
    double scale = params[5].dval;

    glDepthMask(1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glEnable(GL_NORMALIZE);
    glTranslated(act->loc.x,act->loc.y,act->loc.z);
    glRotated((act->ang+angofs)*180/M_PI,0,0,1);
    glScaled(scale,scale,scale);
    glTranslated(ofs.x,ofs.y,ofs.z);
    glDisable(GL_CULL_FACE);

    //dump_gl_state(cout);
    mod->draw(dm, drawmode!=DRAWMODE_TEXTURE, 1.0);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();

}



#if 0


#endif

void setmode(int w, int h, bool fullscreen) {


    GLResource::releaseall();

    if (tesselator)
	gluDeleteTess(tesselator);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE, 0);

    SDL_SetVideoMode(w, h, 32, SDL_OPENGL|(fullscreen?SDL_FULLSCREEN:0));
    SDL_WM_SetCaption ("GLBumper", "GLBumper");

    SDL_ShowCursor(0);

    screenw=w; screenh=h;
    screen_fs = fullscreen;

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_POLYGON_SMOOTH);
    glEnable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_FOG);

    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
    SDL_GL_SwapBuffers();

    SDL_ShowCursor (0);

    CHECKERROR();

    tesselator = gluNewTess();
    gluTessCallback(tesselator,GLU_TESS_BEGIN,(GLvoid(*)())glBegin);
    gluTessCallback(tesselator,GLU_TESS_END,(GLvoid(*)())glEnd);
    gluTessCallback(tesselator,GLU_TESS_VERTEX,(GLvoid(*)())tesscb_vertex);
    gluTessCallback(tesselator,GLU_TESS_COMBINE_DATA,(GLvoid(*)())tesscb_combine);
    gluTessCallback(tesselator,GLU_TESS_ERROR,(GLvoid(*)())tesscb_error);

    GLResource::revalidateall();

    CHECKERROR();

}

#include objdef(ModelAppearance)
#include objdef(WaterAppearance)
#include objdef(AreaAppearance)
#include objdef(BaseAppearance)
#include objdef(FlashAppearance)
#include objdef(SpriteAppearance)
