/* -*- c++ -*- */

#ifndef _UI_H
#define _UI_H

#include "common.h"
#include "object.h"

#include "graphics.h"
#include "text.h"

class Component : public Object {
public: DECLARE_OBJECT;
    sptr<Component> parent;
    vector< sptr<Component> > children;
    bool canfocus, isfocused;
    int x, y;

    Component() : 
	parent(NULL), canfocus(false), isfocused(false), x(0), y(0) { }

    

    virtual ~Component();
    virtual void add(Component* c, int x=0, int y=0);
    virtual void remove(Component* c);

    void render(int x, int y);
    sptr<Component> nextsibling();
    sptr<Component> nextcomponent();
    
    virtual void setfocus(Component* c) {
	if (parent != NULL) parent->setfocus(c);
    }
    virtual void switchfocus() {
	if (parent != NULL) parent->switchfocus();
    }
    virtual void refresh() {
	if (parent != NULL) parent->refresh();
    }
    void grabfocus() {
	setfocus(this);
    }
    virtual void draw(int x, int y) { }
    virtual bool event(SDL_Event& evt) { return false; }
    virtual void tick(double time) { }

};



class RootComponent : public Component {
public: DECLARE_OBJECT;
    bool dirty;
    sptr<Component> focus;
    
    RootComponent() : dirty(false), focus(NULL) { }



    void setfocus(Component* f);
    void switchfocus();
    void draw(int x, int y) {
	dirty = false;
    }
    void refresh() {
	dirty = true;
    }
    bool event(SDL_Event& evt);
    void tick(double time);
    
    void run(double maxfps);

};

class Label : public Component {
    TextLayout tl;
    Color bg;
public: DECLARE_OBJECT;

    Label(Color bg = color::Clear) : bg(bg) { }
    Label(string txt, Font* font = Font::default_font, Color fg = color::White, Color bg = color::Clear);
    void draw(int x, int y);
    TextLayout& layout() {
	return tl;
    }
};

#endif
