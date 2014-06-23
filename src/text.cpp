
#include "common.h"
#include "text.h"
#include "graphics.h"

#include <fstream>

sptr<Font> Font::default_font;
string Font::loaderclass = "font";

static inline Uint16 utf8_to_uni(const char*& utf8) {
    Uint16 ch = *utf8;
    if (ch == 0) return 0;
    if ( ch >= 0xF0 ) {
	ch  =  (Uint16)(ch&0x07) << 18;
	ch |=  (Uint16)(*++utf8 & 0x3F) << 12;
	ch |=  (Uint16)(*++utf8 & 0x3F) << 6;
	ch |=  (Uint16)(*++utf8 & 0x3F);
    } else if ( ch >= 0xE0 ) {
	ch  =  (Uint16)(ch & 0x1F) << 12;
	ch |=  (Uint16)(*++utf8 & 0x3F) << 6;
	ch |=  (Uint16)(*++utf8 & 0x3F);
    } else if ( ch >= 0xC0 ) {
	ch  =  (Uint16)(ch & 0x3F) << 6;
	ch |=  (Uint16)(*++utf8&0x3F);
    }
    utf8++;
    return ch;
}
/*
static Uint16 *UTF8_to_UNICODE(Uint16 *unicode, const char *utf8, int len) {
    int i, j;
    
    for ( i=0, j=0; i < len; ++i, ++j ) {
	unicode[j] = utf8_to_uni(utf8);
    }
    unicode[j] = 0;
    
    return unicode;
}
*/

#include objdef(Font)

sptr<Resource> FontLoader::load_resource_from_file(string filename) {
    sptr<Font> font = GC::track(new Font());

    ifstream f(filename.c_str(), ios::in);
    if (!f)
	throw ios_base::failure("cannot open font file");

    StreamTokenReader ts(f);

    ts.expectSymbol("font");
    string imgname = join_path(dirname(filename), ts.readString(ST_STRING));
	
    font->ascent = ts.readInt();
    font->descent = ts.readInt();

    while (!ts.isEOF()) {
	string s = ts.readString(ST_SYMBOL);
	vector<Glyph>* glyphs;
	if (s == "glyph")
	    glyphs = &font->glyphs;
	else if (s == "shadow") 
	    glyphs = &font->shadowglyphs;
	else
	    throw GenericTokenError(0, "foo!");
		
	int glyphnum = ts.readInt();
	if (((unsigned)glyphnum) >= glyphs->size())
	    glyphs->resize(glyphnum+1);

	Glyph& g((*glyphs)[glyphnum]);
	g.ox = ts.readInt();
	g.oy = ts.readInt();
	g.adv = ts.readInt();
	g.sx = ts.readInt();
	g.sy = ts.readInt();
	g.sw = ts.readInt();
	g.sh = ts.readInt();
    }
    font->tex = new FileGLTexture(imgname, 0);
    font->tex->revalidate();

    return font;

}

int Font::rendertext(const char* txt, int sx, int sy, bool shadow) const {
    int cx=0;
    
    if (!tex) {
	return 0;
    }
    
    const vector<Glyph>& cglyphs(shadow ? shadowglyphs : glyphs);

    Uint16 c;
    const Glyph* g;

    tex->bind();
    while((c = utf8_to_uni(txt))) {
	if (c >= cglyphs.size()) continue;
	g = &cglyphs[c];
	
	double tcxl,tcyl,tcxh,tcyh;
	tcxl=(double)g->sx / tex->w;
	tcxh=(double)(g->sx + g->sw) / tex->w;
	tcyl=(double)g->sy / tex->h;
	tcyh=(double)(g->sy + g->sh) / tex->h;
	
	glBegin(GL_QUADS);
	
	glTexCoord2d(tcxl,tcyl);
	glVertex2i(cx + sx + g->ox, sy + g->oy);
	
	glTexCoord2d(tcxh,tcyl);
	glVertex2i(cx + sx + g->ox + g->sw, sy + g->oy);
	
	glTexCoord2d(tcxh,tcyh);
	glVertex2i(cx + sx + g->ox + g->sw, sy + g->oy + g->sh);
	
	glTexCoord2d(tcxl,tcyh);
	glVertex2i(cx + sx + g->ox, sy + g->oy + g->sh);
	
	glEnd();

	cx += g->adv;
    }

    return cx;
}

int Font::measuretext(const char* txt, int* pminx, int* pmaxx, int* pminy, int* pmaxy) const {
    int cx=0;
    int minx=0;
    int maxx=0;
    int miny=0;
    int maxy=0;
    int f=1;
    Uint16 c;
    int cminx,cminy,cmaxx,cmaxy;
    const Glyph* g;

    while((c = utf8_to_uni(txt))) {
	if (c >= glyphs.size()) continue;

	g = &glyphs[c];

	cminx=cx + g->ox;
	cminy=g->oy;
	cmaxx=cminx + g->sw;
	cmaxy=cminy + g->sh;
	if (f || minx > cminx) minx = cminx;
	if (f || maxx < cmaxx) maxx = cmaxx;
	if (f || miny > cminy) miny = cminy;
	if (f || maxy < cmaxy) maxy = cmaxy;
	f=0;

	cx += g->adv;
    }
    if (pminx) *pminx = minx;
    if (pmaxx) *pmaxx = maxx;
    if (pminy) *pminy = miny;
    if (pmaxy) *pmaxy = maxy;

    return cx;
}

void TextLayout::checktext() {
    splitlines(strm.str(), cfont, ccolor, cscolor);
    strm.str("");
}

void TextLayout::renderpart(int bx, int by, bool shadow) {
    int cx, sy;
    
    checktext();

    sy = by;// - lines[0].ascent;

    FOREACH(cline, lines) {
	sy += cline->ascent;
	cx = 0;
	FOREACH(cchunk, cline->chunks) {
	    if (cx < cchunk->tab) cx = cchunk->tab;
	    const Color& cc(shadow ? cchunk->shadow : cchunk->color);
	    if (cc.a) {
		cc.set();
		cx += cchunk->font->rendertext(cchunk->txt.c_str(), cx + bx, sy, shadow);
	    } else {
		cx += cchunk->font->measuretext(cchunk->txt.c_str(), NULL, NULL, NULL, NULL);
	    }
	}

	sy += cline->descent;
    }
}
void TextLayout::render(int bx, int by) {
    renderpart(bx, by, true);
    renderpart(bx, by, false);
}
void TextLayout::measure(int* pminx, int* pmaxx, int* pminy, int* pmaxy) {
    int minx = 0, maxx = 0, miny = 0, maxy = 0;
    int tminx, tmaxx, tminy, tmaxy;
    int sx, sy;
    bool f = true;

    checktext();

    //sy = -lines[0].ascent;
    sy = 0;
    FOREACH(cline, lines) {
	sx = 0;
	sy += cline->ascent;
	FOREACH(cchunk, cline->chunks) {
	    if (sx < cchunk->tab) sx = cchunk->tab;
	    int adv = cchunk->font->measuretext(cchunk->txt.c_str(), 
						&tminx, &tmaxx, &tminy, &tmaxy);

	    if (f || minx > (tminx + sx)) minx = (tminx + sx);
	    if (f || maxx < (tmaxx + sx)) maxx = (tmaxx + sx);
	    if (f || miny > (tminy + sy)) miny = (tminy + sy);
	    if (f || maxy < (tmaxy + sy)) maxy = (tmaxy + sy);
	    f = false;
	    
	    sx += adv;
	}
	
	sy += cline->descent;

    }

    if (pminx) *pminx = minx;
    if (pmaxx) *pmaxx = maxx;
    if (pminy) *pminy = miny;
    if (pmaxy) *pmaxy = maxy;
}

void TextLayout::addchunk(const string& txt, Font* font, Color color, Color scolor) {
    if (txt.empty() && !horiz_tab) return;
    if (font == NULL) return;

    TextLine& cline(*(lines.end()-1));
    if (cline.chunks.empty())
	cline.ascent = cline.descent = 0; // clear newline font size

    cline.chunks.push_back(TextChunk(txt, font, color, scolor, horiz_tab));

    horiz_tab = 0;

    if (cline.ascent < font->ascent) cline.ascent = font->ascent;
    if (cline.descent < font->descent) cline.descent = font->descent;
}

void TextLayout::splitlines(const string& text, Font* font, Color color, Color scolor) {
    int p, lp;
    if (font == NULL) return;
    if (text.empty()) return;
    lp = 0;
    while ((unsigned)(p = text.find('\n', lp)) != string::npos) {
	addchunk(text.substr(lp, p - lp), font, color, scolor);
	lp = p + 1;
	
	TextLine& cline(*(lines.end()-1));

	if (cline.chunks.empty()) {
	    cline.descent = font->descent;
	    cline.ascent = font->ascent;
	}

	lines.resize(lines.size()+1);

	TextLine& nline(*(lines.end()-1));

	nline.descent = font->descent;
	nline.ascent = font->ascent;
    }

    addchunk(text.substr(lp), font, color, scolor);

}

TextLayout& TextLayout::clear(){
    strm.str("");
    lines.erase(lines.begin(), lines.end());
    lines.resize(lines.size()+1);
    return *this;
}
TextLayout::TextLayout(Font* font, Color color, Color scolor) : 
    cfont(font), ccolor(color), cscolor(scolor) { 
    lines.resize(lines.size()+1);
}
