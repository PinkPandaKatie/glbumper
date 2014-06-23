/*
 *  Copyright (C) 2003  JSPenguin (Jared Stafford)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "tokenizer.h"
#include <ctype.h>
#include <sstream>
#include <iostream>

#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t' || (c) == ' ' || (c) == '\n')
#define IS_WHITESPACE_EOF(c) (IS_WHITESPACE(c) || (c) == EOF)
#define CHECK_EOF(c) if ((c)== EOF) throw TokenEOFError(lineno)
#define NEXTC c = getChar()


string TokenEOFError::getMessage() {
    return string("unexpected EOF");
}


static char* types[] = {
    "symbol", 
    "string", 
    "integer", 
    "float",
    "invalid"};

TokenExpectError::TokenExpectError(TokenType typeexpect, Token _got) : TokenError(_got.lineno), 
								 got(_got) 
{
    expect = types[(typeexpect>=0 && typeexpect<TT_INVALID ? typeexpect : TT_INVALID)];
}
string TokenExpectError::getMessage() {
    ostringstream err;
    err << "expected " << expect << ", got " << got.toString();
    return err.str();
};
void VectorTokenStream::nextToken(Token& t) {
    if (cur == end) throw TokenEOFError(begin==end ? 0 : (end-1)->lineno);
    t = *cur++;
}
void VectorTokenStream::pushBack() {
    if (cur != begin)
	cur--;
}
bool VectorTokenStream::isEOF() {
    return cur == end;
}

TokenReader::TokenReader() {
    pushedback=false;
    eof = false;
    pbc=-1;
    lineno = 1;
}

void TokenReader::skipWhitespace() {
    int c;
    if (pbc != -1) {
	c = pbc;
	pbc = -1;
    } else {
	NEXTC;
    }

again:
    while (IS_WHITESPACE(c)) {
        NEXTC;
    }

    if (c == '#') {
	do {
	    NEXTC;
	} while (c != EOF && c != '\n');
	if (c != EOF)
	    goto again;
    }

    pbc = c;

    if (c == -1) {
	eof = true;
    }
}
bool TokenReader::isEOF() {
    skipWhitespace();
    return eof;
}
void TokenReader::nextToken(Token& t) {
    int c;
    if (pushedback) {
	pushedback = false;
	t = last;
	//cout << "popped " << t.toString() << "\n";
	return;
    }
    skipWhitespace();

    t.sval.clear();
    t.lineno = lineno;
    
    // skipWhitespace always sets pbc
    CHECK_EOF(c = pbc);

    pbc = -1;

    long long ival, divisor;
    t.type = TT_INVALID;
    if (c == '"') {
	CHECK_EOF(NEXTC); // skip quote char
	while (c != '"') {
	    if (c=='\\') {
		CHECK_EOF(NEXTC);
		switch (c) {
		case 'n':
		    c='\n';
		    break;
		case 't':
		    c='\t';
		    break;
		case 'b':
		    c='\b';
		    break;
		}
	    }
	    t.sval += c;
	    CHECK_EOF(NEXTC);
	}
	t.type = TT_STRING;
    } else if ((c >= '0' && c <= '9') || c == '-') {
	int radix = 10;
	bool flt=false;
	bool minus=false;
	divisor=1;
	

	if (c == '0') {	
	    t.sval += c;
	    NEXTC; // won't hurt...
	    if (c == 'x' || c == 'X') {
		t.sval += c;
		radix=16;
		NEXTC;
	    } else if (IS_WHITESPACE_EOF(c)) {
		t.ival = 0;
		t.dval = 0;
		t.type=TT_INTEGER;

		last = t;
		//cout << "read " << t.toString() << "\n";
		return;
	    }
	}
    
	if (c == '-') {
	    minus=true;
	    t.sval += c;
	    CHECK_EOF(NEXTC);
	}
	ival = 0;
	while (c >= '0' && c <= '9' || c == '.' || c == 'e' || (radix == 16 && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))) {
	    minus=false;
	    t.sval += c;
	    if (c == 'e' && (radix != 16)) {
		flt = true;
	    }
	    if (c == '.') {
		if (flt) {
		    break;
		}
		flt=true;
	    }
	    NEXTC;
	}
	pbc=c;
	if (minus) {
	    t.type = TT_SYMBOL;
	    t.sval = "-";
	} else if (flt) {
	    t.type = TT_FLOAT;
	    t.dval = strtod(t.sval.c_str(),NULL);
	    t.ival = (int)t.dval;
	} else {
	    t.type = TT_INTEGER;
	    t.ival = strtol(t.sval.c_str(),NULL,0);
	    t.dval = t.ival;
	}
	
    } else if (c=='_' || isalpha(c)) {
	do {
	    t.sval += c;
	    NEXTC;
	} while (c=='_' || isalnum(c));
	
	t.type = TT_SYMBOL;
    } else {
	t.type = TT_SYMBOL;
	t.sval = c;
    }
    //cout << "read " << t.toString() << "\n";
    last = t;
    return;
}

void TokenReader::pushBack() {
    pushedback = true;
}


void TokenStream::expectToken(Token& t, TokenType type) {
    nextToken(t);
    if (t.type != type) {
	throw TokenExpectError(type,t);
    }
}

void TokenStream::expectSymbol(const char* symbol) {
    Token t;
    nextToken(t);
    if (t.type != TT_SYMBOL || t.sval != symbol) {
	ostringstream err;
	err << "'" << symbol << "'";
	throw TokenExpectError(err.str(),t);
    }
}
double TokenStream::readFloat() {
    Token t;
    nextToken(t);
    if (t.type != TT_FLOAT && t.type != TT_INTEGER)
	throw TokenExpectError(TT_FLOAT,t);
    
    return t.dval;
    
}
double TokenStream::readRational() {
    Token t;

    double val=readFloat();
    if (!isEOF()) {
	nextToken(t);
	if (t.type == TT_SYMBOL && t.sval[0] == '/') {
	    double div=readFloat();
	    val /= div;
	} else {
	    pushBack();
	}
    }
    return val;

}

int TokenStream::readInt() {
    Token t;
    expectToken(t,TT_INTEGER);
    return t.ival;
}

string TokenStream::readString(StringTypeEnum type) {
    Token t;
    nextToken(t);
    switch (type) {
    case ST_SYMBOL:
	if (t.type != TT_SYMBOL)
	    throw TokenExpectError(TT_SYMBOL,t);
	break;
    case ST_STRING:
	if (t.type != TT_STRING)
	    throw TokenExpectError(TT_STRING,t);
	break;
    case ST_SYMBOL_OR_STRING:
	if (t.type != TT_STRING && t.type != TT_SYMBOL)
	    throw TokenExpectError(string("symbol or string"),t);
	break;
    default:
	throw TokenExpectError(TT_INVALID,t);
    }
    return t.sval;
}







FileTokenReader::FileTokenReader(FILE* _f) : TokenReader() {
    f = _f;
}

int FileTokenReader::getChar() {
    int c = fgetc(f);
    if (c == '\n') lineno++;
    return c;
}

StringTokenReader::StringTokenReader(char* _str) : TokenReader (){
    str = _str;
}

int StringTokenReader::getChar() {
    if (!*str) return EOF;
    if (*str == '\n') lineno++;
    return *str++;
}

int StreamTokenReader::getChar() {
    char ch;
    if (!in.get(ch)) return EOF;
    if (ch == '\n') lineno++;
    return ch;
}

string Token::toString() {
    ostringstream str;
    switch (type) {
    case TT_SYMBOL:
	str << "<symbol> " << sval;
	break;
    case TT_INTEGER:
	str << "<int> " << ival;
	break;
    case TT_FLOAT:
	str << "<float> " << dval;
	break;
    case TT_STRING:
	str << "<str> \"" << sval << "\"";
	break;
    default:
	str << "<invalid>";
    }
    return str.str();
}
