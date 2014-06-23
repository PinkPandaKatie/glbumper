/* -*- c++ -*- */

#ifndef _RESOURCE_H
#define _RESOURCE_H

#include "common.h"
#include "object.h"

#include <list>
#include <map>

class GLResource;
class Resource;
class ResourceLoader;

class ResourceLoader {
protected:
    friend class Resource;
    map<string, sptr<Resource> > cache;

public:
    ResourceLoader();
    virtual ~ResourceLoader();
    virtual sptr<Resource> loadresource(string name) = 0;
    virtual sptr<Resource> getresource(string name);

    static void registerloader(string type, ResourceLoader* ldr);

};

class Resource : public Object {
protected:
    friend class ResourceLoader;

    ResourceLoader* ldr;
    string name;

public: DECLARE_OBJECT;

    Resource() : ldr(NULL) { }
    
    void forget() {
	if (ldr) {
	    ldr->cache.erase(name);
	    ldr = NULL;
	}
    }
    virtual ~Resource() {
	forget();
    }

    static sptr<Resource> getresource(string type, string name);

    template<class Rclass>
    static sptr<Rclass> get(string name) {
	sptr<Resource> res = getresource(Rclass::loaderclass, name);
	sptr<Rclass> ret = dynamic_cast<Rclass*>(&*res);
	return ret;
    }

    virtual string str() { return "Resource"; }

};

class FileResourceLoader : public ResourceLoader {
    static string datadir;
    string resource_dir_name, extension;

public:
    FileResourceLoader(string dirname="", string extension="") :
	resource_dir_name(dirname), extension(extension) { }

    static void set_data_dir(string dir) { datadir = dir; }
    static string get_data_dir() { return datadir; }

    virtual sptr<Resource> loadresource(string name);
    virtual sptr<Resource> load_resource_from_file(string filename) = 0;
};


class GLResource {
    static list<GLResource*> glresources;

public:
    GLResource() {
	glresources.push_back(this);
    }
    virtual ~GLResource() {
	release();
	glresources.remove(this);
    }
    virtual void release() { };
    virtual void revalidate() = 0;

    static void releaseall();
    static void revalidateall();
};

string dirname(string filename);
string join_path(string f1, string f2);
static inline string join_path(string f1, string f2, string f3) {
    return join_path(join_path(f1, f2), f3);
}
static inline string join_path(string f1, string f2, string f3, string f4) {
    return join_path(join_path(f1, f2), join_path(f3, f4));
}

#endif
