

#include "common.h"

#include "resource.h"

#include <set>

string FileResourceLoader::datadir;
map<string, ResourceLoader*> resource_loaders;
list<GLResource*> GLResource::glresources;
static set< pair<string, string> > failure_warn;


#include objdef(Resource)

ResourceLoader::ResourceLoader() { }
ResourceLoader::~ResourceLoader() { }


void GLResource::releaseall() {
    FOREACH(res, glresources) {
	(*res)->release();
    }
}
void GLResource::revalidateall() {
    FOREACH(res, glresources) {
	(*res)->revalidate();
    }
}

void ResourceLoader::registerloader(string name, ResourceLoader* ldr) {
    resource_loaders[name] = ldr;
}

sptr<Resource> Resource::getresource(string type, string name) {
    map<string, ResourceLoader*>::iterator itr = resource_loaders.find(type);
    if (itr == resource_loaders.end()) 
	return NULL;
    ResourceLoader* ldr = itr->second;
    try {
	return ldr->getresource(name);
    } catch (exception& ex) {
	pair<string, string> p(type, name);
	if (failure_warn.find(p) != failure_warn.end()) {
	    cerr << "Warning: loading '" << type << "' resource '" << name 
		 << " failed: " << ex.what() << endl;
	    failure_warn.insert(p);
	}
	return NULL;
    }

}

sptr<Resource> ResourceLoader::getresource(string name) {
    map<string, sptr<Resource> >::iterator itr = cache.find(name);
    if (itr != cache.end()) 
	return itr->second;

    sptr<Resource> r = loadresource(name);
    if (r != NULL) {
	cache[name] = r;
	r->ldr = this;
	r->name = name;
    }

    return r;
}

sptr<Resource> FileResourceLoader::loadresource(string name) {
    unsigned int pos = name.find_last_of("/\\:");
    if (pos != string::npos) name = name.substr(pos+1);
    string filename = join_path(datadir, resource_dir_name, name + extension);

    return load_resource_from_file(filename);
}


#ifdef WIN32

// Based on Python ntpath.join

static inline bool isslash(char c) {
    return c == '/' || c == '\\';
}

static bool isabs(string path) {
    if (path.size() >= 2 && path[1] == ':') {
	path = path.substr(2);
    }
    return (!path.empty() && isslash(path[0]));
}

string dirname(string filename) {
    int fs_pos = filename.find_last_of('/');
    int bs_pos = filename.find_last_of('\\');
    if (fs_pos == string::npos || (bs_pos > fs_pos))
	fs_pos = bs_pos;
    if (fs_pos == string::npos)
	return "";
    return filename.substr(0,fs_pos);
}

string join_path(string path, string b) {
    bool b_wins = false; // set to 1 iff b makes path irrelevant
    if (path == "")
	b_wins = true;
    else if (isabs(b)) {
	/*
             This probably wipes out path so far.  However, it's more
             complicated if path begins with a drive letter:
                 1. join('c:', '/a') == 'c:/a'
                 2. join('c:/', '/a') == 'c:/a'
             But
                 3. join('c:/a', '/b') == '/b'
                 4. join('c:', 'd:/') = 'd:/'
                 5. join('c:/', 'd:/') = 'd:/'
	*/
	if ((path.size() >= 2 && path[1] != ':') || (b.size() >= 2 && b[1] == ':'))
	    // Path doesn't start with a drive letter, or cases 4 and 5.
	    b_wins = true;

	// Else path has a drive letter, and b doesn't but is absolute.
	else if (path.size() > 3 || (path.size() == 3 && !isslash(path[2])))
	    // case 3
	    b_wins = true;
    }
    if (b_wins) {
	path = b;
    } else {
	// Join, and ensure there's a separator.
	if (isslash(path[path.size()-1])) {
	    if(!b.empty() && isslash(b[0]))
		path += b.substr(1);
	    else
		path += b;
	} else if (path[path.size()-1] == ':') {
	    path += b;
	} else if (!b.empty()) {
	    if (isslash(b[0]))
		path += b;
	    else
		path += "\\" + b;
	} else {
	    /*   path is not empty and does not end with a backslash,
                 but b is empty; since, e.g., split('a/') produces
                 ('a', ''), it's best if join() adds a backslash in
                 this case.
	    */
	    path += "\\";
	}
    }
    return path;
}

#else

// And now, the much simpler *nix version.

string dirname(string filename) {
    unsigned int pos = filename.find_last_of('/');
    if (pos == string::npos)
	return "";
    return filename.substr(0,pos);
}

string join_path(string path1, string path2) {
    if (path2[0] == '/')
	return path2;
    if (path1.empty() || path1[path1.size()-1] == '/')
	return path1 + path2;
    return path1 + "/" + path2;
}

#endif
