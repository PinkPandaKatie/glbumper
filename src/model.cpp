
#include <iostream>
#include <fstream>
#include <map>

#include "common.h"
#include "tokenizer.h"
#include "vector.h"
#include "model.h"



string Model::loaderclass = "model";

#include objdef(Model)
#include objdef(Material)
#include objdef(Mesh)

inline int readLEInt(istream& is) {
    int a = is.get();
    int b = is.get();
    int c = is.get();
    int d = is.get();
    if (d==EOF) return 0; // if any were EOF, d will be EOF too.
    return a | (b<<8) | (c<<16) | (d<<24);
}
inline int readLEShort(istream& is) {
    int a = is.get();
    int b = is.get();
    if (b==EOF) return 0;
    return a | (b<<8);
}
inline float readLEFloat(istream& is) {
    union {
	int integer;
	float fp;
    } u;
    u.integer=readLEInt(is);
    return u.fp;
}

void Light::readFrom(TokenStream& ts) {
    Token t;
    while (1) {
	ts.expectToken(t,TT_SYMBOL);
	if (t.sval=="}") {
	    break;
	} else if (t.sval=="pos") {
	    
	    pos.x = ts.readFloat();
	    pos.y = ts.readFloat();
	    pos.z = ts.readFloat();
	    directional = ts.readInt();
	} else if (t.sval=="ambient") {
	    amb = Color(ts);
	} else if (t.sval=="diffuse") {
	    diff = Color(ts);
	} else if (t.sval=="specular") {
	    spec = Color(ts);
	} else if (t.sval=="attenuation") {
	    qa = ts.readFloat();
	    la = ts.readFloat();
	    ca = ts.readFloat();
	}
    }
}
inline void Light::setup(int lightnum) {
    GLfloat p[4];
    glEnable(GL_LIGHT0+lightnum);
    p[0]=pos.x; p[1]=pos.y; p[2]=pos.z; p[3]=!directional;
    glLightfv(GL_LIGHT0+lightnum,GL_POSITION,p); 
	
    p[0]=amb.r; p[1]=amb.g; p[2]=amb.b; p[3]=1;
    glLightfv(GL_LIGHT0+lightnum,GL_AMBIENT,p); 
	
    p[0]=diff.r; p[1]=diff.g; p[2]=diff.b; p[3]=1;
    glLightfv(GL_LIGHT0+lightnum,GL_DIFFUSE,p); 

    p[0]=spec.r; p[1]=spec.g; p[2]=spec.b; p[3]=1;
    glLightfv(GL_LIGHT0+lightnum,GL_SPECULAR,p); 
	
    glLightf(GL_LIGHT0+lightnum,GL_CONSTANT_ATTENUATION,ca); 
    glLightf(GL_LIGHT0+lightnum,GL_LINEAR_ATTENUATION,la); 
    glLightf(GL_LIGHT0+lightnum,GL_QUADRATIC_ATTENUATION,qa); 
}

void Material::readFrom(TokenStream& ts) {
    Token t;
    while (1) {

	ts.expectToken(t,TT_SYMBOL);
	if (t.sval=="}") {
	    return;
	} else if (t.sval=="image") {
	    string fn = ts.readString(ST_STRING);
	    tex = Resource::get<Texture>(fn);
	} else if (t.sval=="light") {
	    lights.push_back(Light());
	    (lights.end()-1)->readFrom(ts);
	} else if (t.sval=="ambient") {
	    amb = Color(ts);
	} else if (t.sval=="diffuse") {
	    diff = Color(ts);
	} else if (t.sval=="specular") {
	    spec = Color(ts);
	} else if (t.sval=="shine") {
	    shine = ts.readFloat();
	}
    }
}
void Material::setup(DrawManager& dm) {
    GLfloat p[4];
    if (lights.empty()) {
	glDisable(GL_LIGHTING);
    } else {
	glEnable(GL_LIGHTING);
	int i = 0;
	FOREACH(light,lights) {
	    light->setup(i++);
	}
	for (; i < 8; i++) {
	    glDisable(GL_LIGHT0+i);
	}
    }
    if (tex != NULL) {
	glEnable(GL_TEXTURE_2D);
	tex->setup(dm);
    } else {
	glDisable(GL_TEXTURE_2D);
    }

    p[0]=amb.r; p[1]=amb.g; p[2]=amb.b; p[3]=1;
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,p); 
	
    p[0]=diff.r; p[1]=diff.g; p[2]=diff.b; p[3]=1;
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,p); 

    p[0]=spec.r; p[1]=spec.g; p[2]=spec.b; p[3]=1;
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,p); 

    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shine);
	
}

inline void Face::draw(DrawManager& dm, bool wireframe, float shade) {
    glBegin(wireframe?GL_LINE_LOOP:GL_POLYGON);
    FOREACH(v,vtxv) {
	if (!wireframe) {
	    glColor3f(v->c.r*shade,v->c.g*shade,v->c.b*shade);
	    glTexCoord2f(v->uv.x,v->uv.y);
	    glNormal3f(v->norm.x,v->norm.y,v->norm.z);
	}
	glVertex3f(v->loc.x,v->loc.y,v->loc.z);
    }
    glEnd();
}

inline void Mesh::draw(DrawManager& dm, bool wireframe, float shade) {
    if (!wireframe) material->setup(dm);
    FOREACH(face,facev) {
	face->draw(dm, wireframe,shade);
    }
}


void Model::draw(DrawManager& dm, bool wireframe, float shade) {
    int i;
    for (i = 0; i < NMODCACHE; i++) {
	if (wireframe == dlist.cache[i].wireframe && shade == dlist.cache[i].shade)
	    break;
    }
    if (i == NMODCACHE) {
	i=dlist.cachehead;
	dlist.cache[i].shade = shade;
	dlist.cache[i].wireframe = wireframe;
	dlist.compile(i);
	dlist.cachehead++;
	dlist.cachehead %= NMODCACHE;
	if (wireframe) {
	    glDisable(GL_COLOR_MATERIAL);
	    glDisable(GL_LIGHTING);
	    glDisable(GL_TEXTURE_2D);
	    glLineWidth (1);
	    glColor3d(1,1,1);
	} else {
	    glEnable(GL_COLOR_MATERIAL);
	}

	FOREACH(mesh,meshes) {
	    (*mesh)->draw(dm, wireframe, shade);
	}
	glEndList();
    }
    dlist.call(i);
}

sptr<Resource> ModelLoader::load_resource_from_file(string filename) {
    ifstream fis(filename.c_str(), ios::in);
    if (!fis)
	return NULL;

    StreamTokenReader ts(fis);

    Token t;
    map<string,int> matmap;
    vector<Color> colmap;

    sptr<Model> ret = GC::track(new Model());

    ret->dlist.revalidate();

    ret->materials.push_back(new Material());
    while (!ts.isEOF()) {
	ts.expectToken(t,TT_SYMBOL);
	if (t.sval=="material") {
	    Material* cmat = new Material();
	    while (1) {
		string str = ts.readString(ST_SYMBOL_OR_STRING);
		if (str=="{")
		    break;
		matmap.insert(pair<string,int>(str,ret->materials.size()));
	    }
	    cmat->readFrom(ts);
	    ret->materials.push_back(cmat);
	} else if (t.sval == "colors") {
	    ts.expectSymbol("{");
	    while (1) {
		ts.nextToken(t);
		if (t.type==TT_SYMBOL && t.sval=="}") break;
		ts.pushBack();
		int col = ts.readInt();
		colmap.push_back(Color(((col>>16)&255)/255.0,
				       ((col>> 8)&255)/255.0,
				       ((col>> 0)&255)/255.0));
	    }
	} else if (t.sval == "meshfile") {
	    string name = ts.readString(ST_SYMBOL_OR_STRING);
	    string mfilname = join_path(dirname(filename),name);
	    //cout << mfilname << endl;
	    ifstream is(mfilname.c_str(), ios::in|ios::binary);
	    if (!is)
		throw ios_base::failure("cannot open mesh file");

	    char mn[257];
	    while (1) {
		int nc = is.get();
		if (nc==EOF) break;
		Mesh *cmesh = new Mesh();
		ret->meshes.push_back(cmesh);
		is.get(mn,nc+1);
		mn[nc]=0;
		map<string,int>::iterator itr = matmap.find(string(mn));
		cmesh->material=ret->materials[0];
		if (itr!=matmap.end()) cmesh->material=ret->materials[itr->second];
		int nface = readLEInt(is);
		cmesh->facev.resize(nface);
		for (int i = 0; i < nface; i++) {
		    Face* cface = &cmesh->facev[i];
		    int vflag = is.get();
		    int nvert = vflag&15;
		    cface->vtxv.resize(nvert);
		    for (int j = 0; j < nvert; j++) {
			Vertex* v= &cface->vtxv[j];
			if (vflag&16) {
			    int index = readLEShort(is);
			    v->c = colmap[index];
			} else {
			    v->c = colmap[0];
			}
			if (vflag&32) {
			    float tu = readLEFloat(is);
			    float tv = readLEFloat(is);
			    v->uv = vect2d(tu,1.0-tv);
			} else {
			    v->uv = vect2d(0,0);
			}
			float nx = readLEFloat(is);
			float ny = readLEFloat(is);
			float nz = readLEFloat(is);
			v->norm = vect3d(nx,ny,nz);
			float x = readLEFloat(is);
			float y = readLEFloat(is);
			float z = readLEFloat(is);
			v->loc = vect3d(x,y,z);
		    }
		}
	    }
	}
    }
    return ret;
}
