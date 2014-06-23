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

#ifndef _DATASTREAM_H_
#define _DATASTREAM_H_

#include <SDL_types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <istream>
#include <vector>
#include <stdexcept>
#include <sstream>

#include <zlib.h>

using namespace std;

class DataInput {

    //vector<string> stable;
    bool eof;
public:
    DataInput() : eof(false) { }

    virtual ~DataInput() { }

    virtual int read(void* data, int len) = 0;
    
    virtual bool isEOF() = 0;

    virtual size_t tell() = 0;

    inline Uint8 readu8() {
	Uint8 ret = 0;
	if (read(&ret,1)<1) eof = true;
	return ret;
    }
    virtual Uint16 readu16() {
	Uint16 ret = 0;
	if (read(&ret,2)<2) eof = true;
	return SwapBig16(ret);
    }
    virtual Uint32 readu32() {
	Uint32 ret = 0;
	if (read(&ret,4)<4) eof = true;
	return SwapBig32(ret);
    }
    virtual Uint64 readu64() {
	Uint64 ret = 0;
	if (read(&ret,8)<8) eof = true;
	return SwapBig64(ret);
    }

    inline Sint8 reads8() { 
	return (signed)readu8();
    }
    inline Sint16 reads16() { 
	return (signed)readu16();
    }
    inline Sint32 reads32() { 
	return (signed)readu32();
    }
    inline Sint64 reads64() { 
	return (signed)readu64();
    }

    inline double readf() {
	union {
	    Uint64 u64;
	    double d;
	} ret;

	ret.u64 = readu64();
	return ret.d;
    }

    string readstring() {
	Uint16 len = readu16();
	char dat[len];
	read(dat,len);
	return string(dat,len);
    }

};

class MemoryDataInput : public DataInput {
public:
    Uint8* buffer;
    int pos;
    int size;
    bool autofree;

    MemoryDataInput(Uint8* buffer, int size, bool autofree=true) : buffer(buffer), pos(0), 
								   size(size), autofree(autofree) { }
    ~MemoryDataInput() {
	if (buffer && autofree) {
	    delete[] buffer;
	}
    }

    int read(void* b, int l) {
	if (l + pos > size) l = size - pos;
	if (l)
	    memcpy(b, &buffer[pos], l);
	pos += l;
	return l;
    }

    size_t tell() {
	return pos;
    }

    bool isEOF() {
	return pos >= size;
    }
};

class FileDataInput : public DataInput {
    FILE* f;
    bool autoclose;

public:
    FileDataInput(FILE* f, bool autoclose=true) : f(f), autoclose(autoclose) { }
    ~FileDataInput() {
	if (f && autoclose) {
	    fclose(f);
	}
    }

    int read(void* b, int l) {
	return fread(b, 1, l, f);
    }
    bool isEOF() {
	return feof(f);
    }

    size_t tell() {
	return ftell(f);
    }
};

class GzipFileDataInput : public DataInput {
    gzFile f;
    bool autoclose;

public:
    GzipFileDataInput(gzFile f, bool autoclose=true) : f(f), autoclose(autoclose) { }
    ~GzipFileDataInput() {
	if (f && autoclose) {
	    gzclose(f);
	}
    }

    int read(void* b, int l) {
	return gzread(f, b, l);
    }
    bool isEOF() {
	return gzeof(f);
    }

    size_t tell() {
	return gztell(f);
    }
};

#endif
