#include "common.h"
#include "object.h"
 
// Based on Python's garbage collector by 
// Neil Schemenauer <nas@arctrix.com>

#define NUM_GENERATIONS 3

class GCgeneration : public GCList {
public:
    GCgeneration(int threshold) : threshold(threshold), count(0) { }

    int threshold;
    int count;
};

static GCgeneration generations[NUM_GENERATIONS] = {
    GCgeneration(700),
    GCgeneration(10),
    GCgeneration(10)
};

bool Mutex::initialized = false;
int Object::num_objects = 0;
bool GC::enabled = true;
bool GC::collecting = false;
int Object::max_trashlevel = 20;
int Object:: trashlevel = 0;
Mutex GC::collect_mutex;
Mutex GC::ref_mutex;
GCList Object::trashcan;
map<int, sptr<Thread> > Thread::running_threads;


// I put all these functions in a class to avoid declaring every
// single function as a friend of Object. 


// set all gc_refs = refcount.  After this, gc_refs is > 0 for all objects
// in containers, and is GC_REACHABLE for all tracked gc objects not in
// containers.

inline void GC::update_refs(GCList* containers) {
    for (GCList *gc = containers->next; gc != containers; gc = gc->next) {
	Object* op = static_cast<Object*>(gc);
	op->gc_refs = op->refcount;
    }
}

// A traversal callback for subtract_refs.
void GC::visit_decref(Object* op, void *data) {
    if (op == NULL) return;
    if (op->gc_refs > 0)
	op->gc_refs--;
}

// Subtract internal references from gc_refs.  After this, gc_refs is >= 0
// for all objects in containers, and is GC_REACHABLE for all tracked gc
// objects not in containers.  The ones with gc_refs > 0 are directly
// reachable from outside containers, and so can't be collected.

inline void GC::subtract_refs(GCList* containers) {
    for (GCList* gc = containers->next; gc != containers; gc=gc->next) {
	Object* op = static_cast<Object*>(gc);
	op->__gc_traverse((visitproc)visit_decref, NULL);
    }
}

// A traversal callback for move_unreachable.
void GC::visit_reachable(Object* op, GCList *reachable) {
    if (op == NULL) return;
    
    const int gc_refs = op->gc_refs;
    
    if (gc_refs == 0) {
	// This is in move_unreachable's 'young' list, but
	// the traversal hasn't yet gotten to it.  All
	// we need to do is tell move_unreachable that it's
	// reachable.
	 
	op->gc_refs = 1;
    } else if (gc_refs == GC_TENTATIVELY_UNREACHABLE) {
	// This had gc_refs = 0 when move_unreachable got
	// to it, but turns out it's reachable after all.
	// move it back to move_unreachable's 'young' list,
	// and move_unreachable will eventually get to it
	// again.

	op->moveto(reachable);
	op->gc_refs = 1;
    }
    // Else there's nothing to do.
    // If gc_refs > 0, it must be in move_unreachable's 'young'
    // list, and move_unreachable will eventually get to it.
    // If gc_refs == GC_REACHABLE, it's either in some other
    // generation so we don't care about it, or move_unreachable
    // already dealt with it.
    // If gc_refs == GC_UNTRACKED, it must be ignored.
}

// move the unreachable objects from young to unreachable.  After this,
// all objects in young have gc_refs = GC_REACHABLE, and all objects in
// unreachable have gc_refs = GC_TENTATIVELY_UNREACHABLE.  All tracked
// gc objects not in young or unreachable still have gc_refs = GC_REACHABLE.
// All objects in young after this are directly or indirectly reachable
// from outside the original young; and all objects in unreachable are
// not.

void GC::move_unreachable(GCList* young, GCList* unreachable) {
    GCList* gc = young->next;

    // Invariants:  all objects "to the left" of us in young have gc_refs
    // = GC_REACHABLE, and are indeed reachable (directly or indirectly)
    // from outside the young list as it was at entry.  All other objects
    // from the original young "to the left" of us are in unreachable now,
    // and have gc_refs = GC_TENTATIVELY_UNREACHABLE.  All objects to the
    // left of us in 'young' now have been scanned, and no objects here
    // or to the right have been scanned yet.

    while (gc != young) {
	Object* op = static_cast<Object*>(gc);
	GCList* next;
	
	if (op->gc_refs) {
	    // gc is definitely reachable from outside the
	    // original 'young'.  Mark it as such, and traverse
	    // its pointers to find any other objects that may
	    // be directly reachable from it.  Note that the
	    // call to traverse may append objects to young,
	    // so we have to wait until it returns to determine
	    // the next object to visit.

	    op->gc_refs = GC_REACHABLE;
	    op->__gc_traverse((visitproc)visit_reachable, (void *)young);
	    next = gc->next;
	}
	else {
	    // This *may* be unreachable.  To make progress,
	    // assume it is.  gc isn't directly reachable from
	    // any object we've already traversed, but may be
	    // reachable from an object we haven't gotten to yet.
	    // visit_reachable will eventually move gc back into
	    // young if that's so, and we'll see it again.
	    
	    next = gc->next;
	    gc->moveto(unreachable);
	    op->gc_refs = GC_TENTATIVELY_UNREACHABLE;
	}
	gc = next;
    }
}

// Break reference cycles by clearing the containers involved.	This is
// tricky business as the lists can be changing and we don't know which
// objects may be freed.  It is possible I screwed something up here.

void GC::delete_garbage(GCList* collectable, GCList* old) {
    while (!collectable->empty()) {
	Object *op = static_cast<Object*>(collectable->next);
	
	{ 
	    sptr<Object> obj = op;
	    obj->__gc_clear();
	}

	if (collectable->next == op) {
	    // object is still alive, move it, it may die later
	    op->moveto(old);
	    op->gc_refs = GC_REACHABLE;
	}
    }
}

// This is the main function.  read this to understand how the
// collection process works.

void GC::collect(int generation) {
    int i;
    GCList *young = &generations[generation]; // the generation we are examining
    GCList *old; // next older generation
    GCList unreachable; // unreachable trash 

    collect_mutex.lock();
    
    // update collection and allocation counters
    if (generation+1 < NUM_GENERATIONS)
	generations[generation+1].count += 1;
    
    for (i = 0; i <= generation; i++)
	generations[i].count = 0;

    // merge younger generations with one we are currently collecting
    for (i = 0; i < generation; i++) {
	generations[generation].merge(&generations[i]);
    }

    if (generation < NUM_GENERATIONS-1)
	old = &generations[generation+1];
    else
	old = young;

    // Using refcount and gc_refs, calculate which objects in the
    // container set are reachable from outside the set (i.e., have a
    // refcount greater than 0 when all the references within the
    // set are taken into account).

    update_refs(young);
    subtract_refs(young);

    // Leave everything reachable from outside young in young, and move
    // everything else (in young) to unreachable.
    // NOTE:  This used to move the reachable objects into a reachable
    // set instead.  But most things usually turn out to be reachable,
    // so it's more efficient to move the unreachable things.

    move_unreachable(young, &unreachable);

    // move reachable objects to next generation.
    if (young != old)
	old->merge(young);


    // call clear on objects in the unreachable set.  This will cause
    // the reference cycles to be broken.  It may also cause some objects
    // in finalizers to be freed.
    delete_garbage(&unreachable, old);

    collect_mutex.unlock();
}

void GC::collect_generations() {
    collect_mutex.lock();

    // Find the oldest generation (higest numbered) where the count
    // exceeds the threshold.  Objects in the that generation and
    // generations younger than it will be collected.
    for (int i = NUM_GENERATIONS-1; i >= 0; i--) {
	if (generations[i].count > generations[i].threshold) {
	    collect(i);
	    break;
	}
    }

    collect_mutex.unlock();
}

// run the collector on all generations.
void GC::collect() {
    collect(NUM_GENERATIONS-1);
}

void GC::printstats(ostream& out) {
    collect_mutex.lock();

    out << "GC Stats: \n";
    for (int i = 0; i < NUM_GENERATIONS; i++) {
	GCgeneration* g = &generations[i];
	out << "generation " << i << ": " << g->count << " / "
	     << g->threshold << " (" << g->size() << ") \n";
    }
    collect_mutex.unlock();
}
void GC::printobjects(ostream& out) {
    collect_mutex.lock();

    out << "Tracked objects: \n";
    for (int i = 0; i < NUM_GENERATIONS; i++) {
	out << "  generation " << i << ": \n";
	for (GCList* gc = generations[i].next; gc != &generations[i]; gc=gc->next) {
	    out << "     " << static_cast<Object*>(gc)->str() << "\n";
	}
    }
    collect_mutex.unlock();
}

void Object::track() throw() {
    GC::collect_mutex.lock();

    if ((gc_refs == GC_TRACK_NOT_NEEDED) || !(gc_refs == GC_UNTRACKED || gc_refs == GC_NEW) || refcount == -1) {
	GC::collect_mutex.unlock();
	return;
    }

    generations[0].count++; // number of allocated GC objects
    if (generations[0].count > generations[0].threshold &&
	GC::enabled &&
	!GC::collecting &&
	generations[0].threshold) {
	
	GC::collecting = true;
	GC::collect_generations();
	GC::collecting = false;
    }

    gc_refs = GC_REACHABLE;
    next = &generations[0];
    prev = generations[0].prev;
    prev->next = this;
    generations[0].prev = this;

    GC::collect_mutex.unlock();
}

void Object::untrack() throw() {
    GC::collect_mutex.lock();

    if (gc_refs == GC_TRACK_NOT_NEEDED || gc_refs == GC_UNTRACKED || 
	gc_refs == GC_NEW) {
	GC::collect_mutex.unlock();
	return;
    }
    gc_refs = GC_UNTRACKED;
    prev->next = next;
    next->prev = prev;

    GC::collect_mutex.unlock();
}

void Object::destroy() throw() {

    GC::collect_mutex.lock();

    // If another thread just dereferenced a weak pointer to this
    // object, we might now have references. Now that we have the
    // lock, check the refcount again.

    if (refcount > 0) {
	GC::collect_mutex.unlock();
	return;
    }

    refcount = 0;

    untrack();

    weakptrbase* cur = weak;
    weakptrbase* old;
    while (cur) {
	old = cur;
	cur = cur->next;
	old->set_ref(NULL);
    }

    // Avoid stack overflow.

    if (trashlevel < max_trashlevel) {
	++trashlevel;

	delete this;
	
	if (trashlevel == 1) {
	    while (!trashcan.empty()) {
		Object* op = static_cast<Object*>(trashcan.next);
		op->remove();
		delete op;
	    }
	}

	--trashlevel;
	
    } else {
	moveto(&trashcan);
    }
    GC::collect_mutex.unlock();
}

Object::~Object() {
    --num_objects;

    if (refcount>0)
	cerr << "Foo! deleting object " << str() << " with references!"<<endl;
}

int Thread::run_thread(Thread* t) {
    int ret = 0;

    sptr<Thread> sp = t;
    running_threads[SDL_ThreadID()] = sp;
    try {
	ret = sp->run();
    } catch (exception& ex) {
	cout << "Thread " << SDL_ThreadID() << " caught unexpected exception: " << ex.what() << endl;
    }
    t->lock();
    t->running = false;
    t->unlock();
    running_threads.erase(SDL_ThreadID());
    return ret;
}

int Thread::stop() {
    int ret = 0;
    
    request_stop = true;

    SDL_WaitThread(thr, &ret);
    thr = NULL;

    return ret;
}

void Thread::start() {
    if (thr)
	return;
    
    lock();
    running = true;
    unlock();

    thr = SDL_CreateThread((int(*)(void*))run_thread, this);

}

#include objdef(Thread)


void weakptrbase::set_ref(Object* ref) throw() {
    GC::collect_mutex.lock();

    if (objref) {
	if (this == objref->weak)
	    objref->weak = next;
	if (prev) 
	    prev->next = next;
	if (next)
	    next->prev = prev;

	objref = NULL;
	prev = next = NULL;
    }

    objref = ref;

    if (objref) {
	if (objref->weak) 
	    objref->weak->prev=this;
	prev = NULL;
	next = ref->weak;
	objref->weak = this;
    }
    
    GC::collect_mutex.unlock();
}
