/* -*- c++ -*- */

#ifndef _TEXT_H
#define _TEXT_H

#include "common.h"
#include "resource.h"
#include "graphics.h"

#include <map>

#define GLYPH_VALID 1

struct Glyph {
    short ox, oy;
    short adv;
    int sx, sy;
    short sw, sh;

    Glyph() : ox(0), oy(0), adv(0), sx(0), sy(0), sw(0), sh(0) { }
};

class Font : public Resource {
    Font() : tex(NULL), ascent(0), descent(0) { }
    friend class FontLoader;

public: DECLARE_OBJECT;
    GLTexture* tex;
    int ascent, descent;

    vector<Glyph> glyphs;
    vector<Glyph> shadowglyphs;


    int rendertext(const char* txt, int sx, int sy, bool shadow = false) const;
    int measuretext(const char* txt, int* pminx, int* pmaxx, int* pminy, int* pmaxy) const;

    ~Font() {
	if (tex)
	    delete tex;
    }

    static sptr<Font> default_font;
    static string loaderclass;

};

class FontLoader : public FileResourceLoader {
public:
    FontLoader(string dirname) : FileResourceLoader(dirname, ".fdesc") { }
    sptr<Resource> load_resource_from_file(string filename);
};

struct TextChunk {
    int tab;
    string txt;
    sptr<Font> font;
    Color color, shadow;
    
    TextChunk(string txt, Font* font, Color color, 
	      Color shadow, int tab) : 
	tab(tab), txt(txt), font(font), color(color), shadow(shadow) { }
};

struct TextLine {
    vector<TextChunk> chunks;
    int ascent, descent;

    TextLine() : ascent(0), descent(0) { }
};

class TextLayout {
    vector<TextLine> lines;

    sptr<Font> cfont;
    Color ccolor;
    Color cscolor;

    ostringstream strm;

    int horiz_tab;

    void addchunk(const string& txt, Font* font, Color color, Color shadow);
    void splitlines(const string& txt, Font* font, Color color, Color shadow);
    void checktext();
    void renderpart(int bx, int by, bool shadow);

public:
    TextLayout(Font* font=Font::default_font, Color color=Color(), Color shadow=color::Clear);
    
    void render(int bx, int by);
    void measure(int* pminx, int* pmaxx, int* pminy, int* pmaxy);

    TextLayout& add(const string& txt, const sptr<Font> font, Color color) {
	checktext();
	splitlines(txt, cfont, color, cscolor);
	return *this;
    }
    TextLayout& add(const string& txt, Color color) {
	return add(txt, cfont, color);
    }
    TextLayout& add(const string& txt, const sptr<Font> font) {
	return add(txt, font, ccolor);
    }
    TextLayout& add(const string& txt) {
	return add(txt, cfont, ccolor);
    }
    const sptr<Font> font() {
	return cfont;
    }
    Color color() {
	return ccolor;
    }
    TextLayout& color(const Color color) {
	checktext();
	ccolor = color;
	return *this;
    }
    TextLayout& color(double r, double g, double b, double a=1.0) {
	checktext();
	ccolor = Color(r, g, b, a);
	return *this;
    }
    TextLayout& color(int color) {
	checktext();
	ccolor = Color(color);
	return *this;
    }

    Color shadow() {
	return cscolor;
    }
    TextLayout& shadow(const Color color) {
	checktext();
	cscolor = color;
	return *this;
    }
    TextLayout& shadow(double r, double g, double b, double a=1.0) {
	checktext();
	cscolor = Color(r, g, b, a);
	return *this;
    }
    TextLayout& shadow(int color) {
	checktext();
	cscolor = Color(color);
	return *this;
    }

    TextLayout& font(Font* font) {
	checktext();
	cfont = font;
	return *this;
    }

    TextLayout& settab(int tab) {
	checktext();
	horiz_tab = tab;
	return *this;
    }

    TextLayout& clear();

    ostream& stream() {
	return strm;
    }
};

struct htab {
    int tab;
    htab(int tab) : tab(tab) { }
};

template<typename T>
inline TextLayout& operator<<(TextLayout& tl, const T& v) {
    tl.stream() << v;
    return tl;
}

inline TextLayout& operator<<(TextLayout& tl, ostream& (*pf)(ostream&)) {
    pf(tl.stream());
    return tl;
}

template<>
inline TextLayout& operator<<(TextLayout& tl, const Color& col) {
    return tl.color(col);
}

inline TextLayout& operator<<(TextLayout& tl, Font* font) {
    return tl.font(font);
}

inline TextLayout& operator<<(TextLayout& tl, const htab t) {
    return tl.settab(t.tab);
}


#endif
