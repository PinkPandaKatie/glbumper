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

/*
 * game.cpp: a simple demonstration game
 *
 * This code is a mess.
 */

#include "common.h"
#include "client.h"
#include "server.h"
#include "graphics.h"
#include "sound.h"
#include "model.h"
#include "text.h"
#include "ui.h"
#include "smdp.h"

void usage(char* argv0) {
    printf("usage: %s [-f] [-g wxh] [-fps limit] host:port \n"
	   "\n"
	   "-f         Use fullscreen mode\n"
	   "-g         set the window size or fullscreen resolution\n"
	   "-fps       Limit frames per second. Default is 80\n"
	   "host:port  Specify server and port to connect to. Default is\n"
	   "           \"localhost:20200\". If this option is used, the port\n"
	   "           MUST be specified!\n\n"
	   ,
	   argv0);
    exit(0);
}


int main (int argc, char **argv) {
    int port = -1;
    int fullscreen = 0;
    int w = 640, h = 480;
    int targc = argc - 1;
    char **targv = argv + 1;
    char *worldname = "demo";
    const char* datadir = getenv("GLBUMPER_DATA");
    if (!datadir) {
	datadir = ".";
    }
    int maxfps = 80;
    int fps = 25;
    srand(time(NULL));
    sock_init();


    while (targc) {
	if (**targv == '-') {
	    if (!strcmp (*targv + 1, "p")) {
		targc--;
		port = atoi(*++targv);
	    } else if (!strcmp (*targv + 1, "f")) {
		fullscreen = 1;
	    } else if (!strcmp (*targv + 1, "D")) {
		if (!targc--) usage(argv[0]);
		datadir = *++targv;
	    } else if (!strcmp (*targv + 1, "nosound")) {
		Sound::enable = false;
	    } else if (!strcmp (*targv + 1, "g")) {
		int ww, hh;
		targc--;
		if (sscanf (*++targv, "%dx%d", &ww, &hh) == 2) {
		    w = ww;
		    h = hh;
		}
	    } else if (!strcmp (*targv + 1, "fps")) {
		targc--;
		maxfps=atoi(*++targv);
	    } else if (!strcmp (*targv + 1, "sfps")) {
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

#ifdef ENABLE_PROFILE
    profile_init();
#endif

    int framems = 1000 / fps;

    int retval = 0;

    FileResourceLoader::set_data_dir(datadir);

    ResourceLoader::registerloader("texture", (new TextureLoader("textures")));
    ResourceLoader::registerloader("model", (new ModelLoader("models")));
    ResourceLoader::registerloader("font", (new FontLoader("fonts")));
    if (Sound::enable)
	ResourceLoader::registerloader("sound", (new SoundLoader("sounds")));


    SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);

    //Socket::init();

    setmode(w, h, fullscreen);

    init_engine();
    Sound::init();


    Font::default_font = Resource::get<Font>("default16");

    sptr<ClientComponent> gcomp = GC::track(new ClientComponent());
    sptr<ServerThread> svr = GC::track(new ServerThread(framems));


    sptr<RootComponent> root = GC::track(new RootComponent());
    

    try {
	svr->loadworld(worldname);
	svr->start();
	sptr<SMDPLoopbackConnection> conn1 = GC::track(new SMDPLoopbackConnection());
	sptr<SMDPLoopbackConnection> conn2 = conn1->getpeer();

	svr->connect_loopback(conn1);
	gcomp->connect_loopback(conn2);
	
	if (port != -1)
	    svr->listen(port);

	
	dsleep(0.001);

	root->add(gcomp);
	gcomp->grabfocus();

	root->run(maxfps);

    } catch (std::exception& te) {
	cout << "\nUnexpected error: " << te.what() << "\n";
    }
    
    //svr->stop();

    //svr = NULL;
    gcomp = NULL;
    root = NULL;

    //GC::collect();
    /*
    cout << "\nBefore collection:\n";
    GC::printstats();
    cout << "After collection:\n";
    GC::printstats();
    */
    
    cout<<endl;
    SDL_Quit ();
    return retval;
}
