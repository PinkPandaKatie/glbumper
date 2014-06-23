// -*- c++ -*-

#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <SDL.h>
#include <SDL_thread.h>
#include <SDL_image.h>

enum {
    GC_NEW = -3,
    GC_UNTRACKED = -4,
    GC_REACHABLE = -5,
    GC_TENTATIVELY_UNREACHABLE = -6,
    GC_TRACK_NOT_NEEDED = -7
};

// Forward declaration of classes
class Object;
class GCList;
class GC;
class weakptrbase;
template<typename Tp> class sptr;

typedef void (*visitproc)(Object*, void*);

// Put this in all Object classes.
#define DECLARE_OBJECT                          \
protected:                                      \
void __gc_traverse_members(visitproc, void*);   \
void __gc_clear_members() throw();              \
virtual void __gc_traverse(visitproc, void*);   \
virtual void __gc_clear() throw();              \
virtual bool __gc_needtrack() throw();          \
public:                                         \
virtual string classname() const throw()

#define objdef(clas) <objdef/clas.impl>

// from Linux asm/atomic.h

#ifdef __GNUC__
#  if defined(__amd64__) || defined(__i386__) 
// mnemonics are the same on ia32 and amd64
#define atomic_increment(var) __asm__ volatile ("lock incl %0" : "+m" (var))
#define atomic_decrement(var, is_zero)                                  \
    __asm__ volatile ("lock decl %0\n\tsete %1" : "+m" (var), "=r" (is_zero));

#endif
#endif

#ifndef atomic_increment

//#warning "no atomic increment/decrement available, using slower mutex version instead"

#define atomic_increment(var)			\
    GC::ref_mutex.lock();			\
    ++(var);					\
    GC::ref_mutex.unlock()

#define atomic_decrement(var, is_zero)		\
    GC::ref_mutex.lock();			\
    is_zero = --(var) == 0);                    \
    GC::ref_mutex.unlock()


#endif

class Mutex {
protected:
    static bool initialized;
    SDL_mutex* mtx;

public:
    Mutex() throw() {
	if (!initialized) {
	    SDL_Init(SDL_INIT_NOPARACHUTE);
	    initialized = true;
	}
	mtx = SDL_CreateMutex();
    }

    ~Mutex() {
	SDL_DestroyMutex(mtx);
    }

    void lock() throw() {
	SDL_LockMutex(mtx);
    }

    void unlock() throw() {
	SDL_UnlockMutex(mtx);
    }
};

class Condition : public Mutex {
    SDL_cond* cond;

public:
    Condition() throw() {
	cond = SDL_CreateCond();
    }

    ~Condition() {
	SDL_DestroyCond(cond);
    }

    void broadcast() throw() {
	lock();
	SDL_CondBroadcast(cond);
	unlock();
    }

    void signal() throw() {
	lock();
	SDL_CondSignal(cond);
	unlock();
    }

    void wait() throw() {
	lock();
	SDL_CondWait(cond, mtx);
	unlock();
    }

    
};

class Lock {
    Mutex* mut;

public:
    Lock(Mutex* mut) throw() : mut(mut) {
	mut->lock();
    }
    ~Lock() {
	mut->unlock();
    }
};

class GC {
    friend class Object;

    static bool collecting;
    static bool enabled;

    static inline void update_refs(GCList* containers);

    static inline void visit_decref(Object*, void*);
    static void visit_reachable(Object*, GCList*);

    static void subtract_refs(GCList* containers);
    static void move_unreachable(GCList* young, GCList* unreachable);

    static void delete_garbage(GCList* collectable, GCList* old);

public:

    // This is the main function. Read this to understand how the
    // collection process works.
    static void collect(int generation);
    static void collect_generations();
    static void collect();

    static void enable(bool en) { enabled = en; }
    static void printstats(ostream& out = cout);
    static void printobjects(ostream& out = cout);

    template<typename ObjTp>
    static sptr<ObjTp> track(ObjTp* o);

    static Mutex collect_mutex;
    static Mutex ref_mutex;

};

// A linked list for keeping track of all Objects.

class GCList {

public:
    GCList *next, *prev;
    
    GCList() {
	next = prev = this;
    }
    // list functions

    bool empty() throw() {
	return (next == this);
    }

    // remove `node` from the gc list it's currently in.
    void remove() throw() {
	prev->next = next;
	next->prev = prev;
	prev = next = this;
    }

    // move node from the gc list it's currently in (which is not explicitly
    // named here) to the end of `l`.

    inline void moveto(GCList* l) throw() {
	// Unlink from current list.
	prev->next = next;
	next->prev = prev;

	// Relink at end of new list.
	prev = l->prev;
	prev->next = l->prev = this;
	next = l;
    }

    // append list `from` onto this; `from` becomes an empty list
    inline void merge(GCList *from) throw() {
	GCList *tail;
	if (!from->empty()) {
	    tail = prev;
	    tail->next = from->next;
	    tail->next->prev = tail;
	    prev = from->prev;
	    prev->next = this;
	}
	from->next = from->prev = from;
    }

    inline long size() throw() {
	long n = 0;
	for (GCList* gc = next; gc != this; gc = gc->next) {
	    n++;
	}
	return n;
    }

    // end of list stuff

};


class Object : private GCList {
    friend class GC;
    friend class weakptrbase;
    template<typename Tp> friend class sptr;
    

    // gc_refs can have one of the following values when the garbage
    // collector is *NOT* running:
    //
    // GC_NEW:        
    // Indicates that the Object has just been allocated and has never
    // been referenced. 
    //
    // GC_UNTRACKED:  
    // Indicates that the Object has been referenced, but is not being
    // tracked by the garbage collector. 
    //
    // GC_REACHABLE:
    // Indicates that the Object is being tracked by the garbage
    // collector.
    //
    // When the garbage collector *IS* running, gc_refs may have one
    // of the following values:
    //
    // >= 0:
    // Indicates that the collector has not examined the Object yet.
    // In this case, gc_refs is the number of references to the Object
    // from outside of the set of objects the garbage collector is
    // examining. 
    //
    // GC_TENTATIVELY_UNREACHABLE:
    // Indicates that the Object has no references outside the current
    // set, and that the garbage collector has examined it and moved
    // it to the list of tentativly unreachable objects.
    //
    int gc_refs;


    // The number of smart pointers that refer to this object. It is
    // declared volatile to avoid locking a mutex just to reference
    // and dereference the object.
    volatile int refcount;

    // A linked list of all weak pointers to this Object. Weak
    // pointers are *NOT* counted in refcount.
    weakptrbase* weak;

    static int max_trashlevel;
    static int trashlevel;
    static GCList trashcan;

    // Destroy the Object and clear all weak references.
    void destroy() throw();

    
protected:

    // Track the object with the garbage collector.
    void track() throw();

    // Stop tracking the object with the garbage collector.
    void untrack() throw();

    // Traverse this Object, calling visit on every Object that this
    // Object refers to directly.
    virtual void __gc_traverse(visitproc visit, void* data) { }

    // Clear all references to other Objects. This is called by the
    // garbage collector to break cyclic references.
    virtual void __gc_clear() throw() { }

    virtual bool __gc_needtrack() throw() { return true; }


public:
    Object() throw() : gc_refs(GC_NEW), refcount(0), 
	       weak(NULL) { ++num_objects; }
    
    virtual ~Object();

    // A global count of objects. Use this to check for memory leaks.
    static int num_objects;

    virtual string classname() const throw() { return "Object"; }

    // Return some information about this Object for debugging purposes.
    virtual string str() const throw() { return classname(); }
};

template<typename ObjTp>
sptr<ObjTp> GC::track(ObjTp* o) {
    sptr<ObjTp> ret(o);
    if (o && o->gc_refs == GC_NEW) {
        if (static_cast<Object*>(o)->__gc_needtrack()) {
            o->track();
        } else {
            o->gc_refs = GC_TRACK_NOT_NEEDED;
        }
    }
    return ret;
}

// A "smart" pointer that automatically references and dereferences
// the Objects that it refers to.

template<typename Tp>
class sptr {
protected:
    Tp* objref;

    inline void set_ref(Tp* obj) throw() {
	Tp* old = objref;
	objref = obj;
	if (obj) {
	    atomic_increment(obj->refcount); 
	}
	if (old) {
            unsigned char is_zero;
	    atomic_decrement(old->refcount, is_zero); 
            if (is_zero) old->destroy();
	}
    }


public:
    sptr(Tp* ref = NULL) throw() : objref(NULL) { *this = ref; }
    sptr(const sptr& ref) throw() : objref(NULL) { *this = ref; }

    sptr& operator=(Tp* ref) throw() { set_ref(ref); return *this; }
    sptr& operator=(const sptr& ref) throw() { set_ref(ref.objref); return *this; }

    ~sptr() { set_ref(NULL); }

    // Allow assignment to sptr<BaseClass> from sptr<SubClass>.
    
    template<typename newtp>
    sptr(const sptr<newtp>& ref) throw() : objref(NULL) { *this = &*ref; }

    template<typename newtp>
    sptr& operator=(const sptr<newtp>& ref) throw() { return (*this = &*ref); }
    

    // Dynamically or statically cast the pointer to another
    // type. call as ptr.[stat]cast<NewType>() 

    template<typename newtp>
    sptr<newtp> statcast() const throw() {
	return static_cast<newtp*>(objref);
    }

    template<typename newtp>
    sptr<newtp> cast() const throw() {
	return dynamic_cast<newtp*>(objref);
    }

    // get the object that this pointer refers to.
    Tp& operator*() const throw() { return *objref; }
    Tp* operator->() const throw() { return objref; }
    operator Tp*() const throw() { return objref; }
};

// Base class for weak pointer template.
class weakptrbase {
    friend class Object;

    weakptrbase* next;
    weakptrbase* prev;

protected:
    Object* objref;

    // Change the Object that this pointer refers to. If this pointer
    // previously referred to another Object, remove the pointer from
    // the Object's weak pointer list.
    void set_ref(Object* rref) throw();
    
    weakptrbase() throw() : next(NULL), prev(NULL), objref(NULL) { }

    ~weakptrbase() {
	set_ref(NULL);
    }
};

// A "weak" pointer that does not hold a reference to the
// Object. Unlike dumb pointers, this pointer will be cleared when the 
// Object is destroyed.

template<typename Tp>
class weakptr : public weakptrbase {

public:
    weakptr(Tp* ref = NULL) throw() { setref(ref); }
    weakptr(const weakptr& ref) throw() : weakptrbase() { 
        sptr<Tp> sp = ref.getref();
        setref(sp);
    }

    weakptr& operator=(const weakptr& ref) {
        sptr<Tp> sp = ref.getref();
        setref(sp);
        return *this;
    }

    

    void setref(Tp* t) throw() {
	set_ref(&*t);
    }

    sptr<Tp> getref() const throw();
};

template<typename Tp>
sptr<Tp> weakptr<Tp>::getref() const throw() {
    GC::collect_mutex.lock();

    sptr<Tp> ret = dynamic_cast<Tp*>(objref);

    GC::collect_mutex.unlock();

    return ret;
}

// A wrapper class for an SDL_Thread object.
class Thread : virtual public Object, public Mutex {
    SDL_Thread* thr;
    volatile bool request_stop;
    bool running;
    friend int run_thread(Thread* t);
    static map<int, sptr<Thread> > running_threads;

protected:
    // Returns true if the thread should stop.
    inline bool should_stop() {
	return request_stop;
    }
    static int run_thread(Thread* t);

DECLARE_OBJECT;
public: 
    Thread() : thr(NULL), request_stop(false) { }
    virtual ~Thread() {
    }

    static sptr<Thread> current_thread() {
	return running_threads[SDL_ThreadID()];
    }

    // get this thread's unique identifier.
    int get_id() {
	if (thr) return SDL_GetThreadID(thr);
	return 0;
    }
    int stop();
    void start();
    virtual int run() { return 0; }
    bool isrunning() {
	Lock(this);
	return running;
    }
};


#endif

