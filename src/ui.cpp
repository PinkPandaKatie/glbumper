
#include "ui.h"

/*
 *
 * Component
 *
 */

#include objdef(Component)

Component::~Component() {

}

void Component::add(Component* c, int x, int y) {
    if (c->parent != NULL) return;
    children.push_back(c);
    c->x = x;
    c->y = y;
    c->parent = this;
}

void Component::remove(Component* c) {
    if (c->isfocused) switchfocus();
    FOREACH(itr, children) {
	if (*itr == c) {
	    children.erase(itr);
	    c->parent = NULL;
	    break;
	}
    }
}
    
void Component::render(int x, int y) {
    draw(x, y);
    FOREACH(c, children) {
	(*c)->render(x + (*c)->x, y + (*c)->y);
    }
}

sptr<Component> Component::nextsibling() {
    if (parent == NULL) 
	return NULL;
    
    unsigned int i;
    for (i = 0; i < parent->children.size()-1; i++) {
	if (parent->children[i] == this) {
	    return parent->children[i+1];
	}
    }
    return NULL;
}
sptr<Component> Component::nextcomponent() {
    if (!children.empty())
	return children[0];

    if (parent == NULL) 
	return NULL;

    Component* f = this;
    Component* next;

    while (f != NULL && (next = f->nextsibling()) == NULL) f = f->parent;
    return f;
}

/*
 *
 * RootComponent
 *
 */

#include objdef(RootComponent)

void RootComponent::setfocus(Component* f) {
    if (focus != NULL) focus->isfocused = false;
    focus = f;
    if (focus != NULL) focus->isfocused = true;
    refresh();
}
void RootComponent::switchfocus() {
    Component* f = focus;
    if (f == NULL) f = this;
    while (f != NULL && !f->canfocus) f = f->nextcomponent();
    setfocus(f);
}
bool RootComponent::event(SDL_Event& evt) {
    if (focus != NULL && focus->parent == NULL) setfocus(NULL);
    Component* f, *par;
    f = focus;
    while (f != NULL && f != this) {
	par = f->parent;
	if (f->event(evt)) {
	    return true;
	}
	f = par;
    }
    return false;
}
void RootComponent::tick(double time) {
    sptr<Component> f, next;
    f = nextcomponent();
    while (f != NULL) {
	next = f->nextcomponent();
	f->tick(time);
	f = next;
    }

}

void RootComponent::run(double maxfps) {
    SDL_Event evt;

    double minframetime = 1.0 / maxfps;
    double ltime, ctime;
    double mintime = gettime();
    ltime = ctime = gettime();
    double deltatime = 0.0;
    while (!children.empty()) {
	
	while (SDL_PollEvent (&evt)) {
	    event(evt);
	}
	
	tick(deltatime);
	
	glDepthMask(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D (0, screenw, screenh, 0);
	
	render(0, 0);
	CHECKERROR();

	ltime=ctime;
	while ((ctime=gettime())<mintime) {
	    dsleep(mintime - ctime);
	}
	mintime = ctime + minframetime;
	deltatime = ctime - ltime;
	if (deltatime > 0.1) deltatime = 0.1;

	SDL_GL_SwapBuffers();

	// Create and destroy an Object to bump the garbage
	// collector's counter.
	sptr<Object> tmpobject = new Object();
    }
}

/*
 *
 * Label
 *
 */

#include objdef(Label)


Label::Label(string txt, Font* font, Color fg, Color bg) : bg(bg) {
    tl.font(font) << fg << txt;
}

void Label::draw(int x, int y) {
    int tminx, tminy, tmaxx, tmaxy;
    
    tl.measure(&tminx, &tmaxx, &tminy, &tmaxy);
    
    tmaxx -= tminx;
    tmaxy -= tminy;
    

    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    //glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    bg.set();
    drawrect(x, y, x + tmaxx + 10, y + tmaxy + 10);
    glEnable(GL_TEXTURE_2D);
    
    tl.render(x + 5, y + 5);

}
