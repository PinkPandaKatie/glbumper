/* for Emacs: -*-c++-*-*/

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

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include <SDL_types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <istream>
#include <vector>
#include <exception>

using namespace std;

typedef enum { TT_SYMBOL, TT_STRING, TT_INTEGER, TT_FLOAT, TT_INVALID } TokenType;
class Tokenizer;

class Token {
public:
    Token() { }
    Token(string str) : sval(str) {
	type = TT_SYMBOL;
    }
    Token(double dval) : ival((int)dval), dval(dval)  {
	type = TT_FLOAT;
    }
    Token(int ival) : ival(ival), dval(ival)  {
	type = TT_INTEGER;
    }
    TokenType type;

    string sval;
    int ival;
    double dval;

    int lineno;

    string toString();
};

class TokenError : public exception {
public:
    TokenError(int lineno) : lineno(lineno) { }
    virtual string getMessage()=0;
    virtual ~TokenError() throw() { }
    int lineno;
};
class GenericTokenError : public TokenError {
public:
    GenericTokenError(int lineno,string reason) : TokenError(lineno),reason(reason) { }
    virtual string getMessage() { return reason; }
    virtual ~GenericTokenError() throw() { }
    string reason;
};

class TokenEOFError : public TokenError {
public:
    TokenEOFError(int lineno) : TokenError(lineno) { }
    ~TokenEOFError() throw() { }
    virtual string getMessage();
};

class TokenExpectError : public TokenError {
public:
    TokenExpectError(string _expect, Token _got) : TokenError(_got.lineno), 
							       expect(_expect), got(_got) { }
    TokenExpectError(const char* _expect, Token _got) : TokenError(_got.lineno), 
							       expect(_expect), got(_got) { }
    ~TokenExpectError() throw() { }
    TokenExpectError(TokenType expect, Token _got);
    virtual string getMessage();
    string expect;
    Token got;
};

typedef enum { ST_STRING, ST_SYMBOL, ST_SYMBOL_OR_STRING } StringTypeEnum;

class TokenStream {
public:
    TokenStream() {  }
    virtual bool isEOF()=0;
    virtual void nextToken(Token& t)=0;
    virtual void pushBack()=0;

    void expectToken(Token& t, TokenType type);
    void expectSymbol(const char* symbol);

    double readFloat();
    double readRational();
    int readInt();
    string readString(StringTypeEnum type);

    virtual ~TokenStream() { }

};

class VectorTokenStream : public TokenStream {
public:
    VectorTokenStream(vector<Token>::iterator _begin, 
		      vector<Token>::iterator _end) : begin(_begin), end(_end), cur(_begin) { };

    virtual void nextToken(Token& t);
    virtual void pushBack();
    virtual bool isEOF();
    
protected:
    vector<Token>::iterator begin;
    vector<Token>::iterator end;
    vector<Token>::iterator cur;

};

class TokenReader : public TokenStream {
public:
    TokenReader();
    virtual void nextToken(Token& t);
    virtual void pushBack();
    virtual bool isEOF();
    virtual int getChar()=0;
    
protected:
    int lineno;
    int pbc;
    bool pushedback;
    bool eof;
    Token last;

    void skipWhitespace();
};


class StringTokenReader : public TokenReader {
public:
    StringTokenReader(char* str);

protected:
    int getChar();

private:
    char* str;
};

class FileTokenReader : public TokenReader {
public:
    FileTokenReader(FILE* f);

protected:
    int getChar();

private:
    FILE* f;
};

class StreamTokenReader : public TokenReader {
public:
    StreamTokenReader(istream& f) : in(f) { };

protected:
    int getChar();

private:
    istream& in;
};

#endif
