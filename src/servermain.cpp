/*
 *  Copyright (C) 2005 Jared Stafford
 *
 *  GLBumper: A clone of Micro$oft "Hover!"
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



#include "common.h"
#include "engine.h"
#include "game.h"
#include "profile.h"
#include "server.h"
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <list>

// SDL on Win32 defines main as SDL_main, then links in a library that
// defines the WinMain function. However, the standalone server should
// be a console application on Windows, so we #undef main.

#ifdef main
#undef main
#endif

void usage(char* argv0) {
    printf("usage: %s [-p port] [-fps limit] [world]\n"
	   "\n"
	   "-p        Specify port to listen on. Default is 20200\n"
	   "-fps      set server frames per second. Default is 20.\n"
	   "          High values will flood your network!\n"
	   "world     Specify world to load, minus the .world extension.\n\n"
	   ,
	   argv0);
    exit(0);
}

static volatile bool server_exit = false;

#ifdef WIN32

BOOL console_ctrl_handler(int type) {
    switch(type) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
	server_exit = true;
	return TRUE;
	
    default: 
	return FALSE; 
    }
}
#else
#include <sys/signal.h>

void sigint(int signum) {
    server_exit = true;
}
#endif

void pollsg(SocketGroup* g, double t) {
    try {
	g->poll(t);
    } catch (exception& e) {
	cout << "Got exception ("<<e.what()<<"), continuing" << endl;
    }
}

#define WAIT_FRAC (1.0/2.0)
int main (int argc, char **argv) {
    int targc = argc - 1;
    char **targv = argv + 1;
    char *worldname = "demo";
    const char* datadir = getenv("GLBUMPER_DATA");
    if (!datadir) {
	datadir = ".";
    }

    int fps = 25;

    int port = 20200;

    srand(time(NULL));

    SDL_Init (SDL_INIT_NOPARACHUTE);
    sock_init();

#ifdef WIN32
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)console_ctrl_handler, TRUE);
#else
    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);

#endif

    while (targc) {
	if (**targv == '-') {
	    if (!strcmp (*targv + 1, "p")) {
		targc--;
		port = atoi(*++targv);
	    } else if (!strcmp (*targv + 1, "D")) {
		if (!targc--) usage(argv[0]);
		datadir = *++targv;
	    } else if (!strcmp (*targv + 1, "fps")) {
		targc--;
		fps = atoi(*++targv);
	    } else {
		usage(argv[0]);
	    }
	} else {
	    worldname = *targv;
	}
	targc--;
	targv++;
    }

    FileResourceLoader::set_data_dir(datadir);
    
    int framems = 1000/fps;

    init_engine();

    Socket::init();

    sptr<ServerThread> svr = GC::track(new ServerThread(framems));

    svr->loadworld(worldname);

    svr->listen(port);


    svr->start();

    while (!server_exit && svr->isrunning()) {
	dsleep(0.5);
    };

    cout << "\n\n\nGot signal, exiting" << endl;

    svr->stop();

    svr = NULL;

    cout << "\nBefore collection:\n";
    GC::printstats();
    GC::collect();
    cout << "After collection:\n";
    GC::printstats();
    //GC::printobjects();

    cout<<endl;
    SDL_Quit ();
    return 0;
}
