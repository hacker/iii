#include <syslog.h>
#include <getopt.h>
#include <sys/stat.h>
#include <glob.h>
#include <iostream>
#include <cassert>
#include <stdexcept>
#include "eyetil.h"
#include "eyefiworker.h"

#include "config.h"

#include "eyefi.nsmap"

#define PHEADER \
	PACKAGE " Version " VERSION "\n" \
	"Copyright (c) 2009 Klever Group"

int main(int argc,char **argv) try {

    int port = 59278;

    while(true) {
	static struct option opts[] = {
	    { "help", no_argument, 0, 'h' },
	    { "usage", no_argument, 0, 'h' },
	    { "version", no_argument, 0, 'V' },
	    { "license", no_argument, 0, 'L' },
	    { "port", required_argument, 0, 'p' },
	    { NULL, 0, 0, 0 }
	};
	int c = getopt_long(argc,argv,"hVLp:",opts,NULL);
	if(c==-1) break;
	switch(c) {
	    case 'h':
		std::cerr << PHEADER << std::endl << std::endl
		    << " " << argv[0] << " [options]" << std::endl
		    << std::endl <<
		    " -h, --help,\n"
		    " --usage                    display this text\n"
		    " -V, --version              display version information\n"
		    " -L, --license              show license\n"
		    " -p <port>, --port=<port>   port to listen to\n"
		    "           (you're not likely to ever need it)\n"
		    << std::endl << std::endl;
		exit(0);
		break;
	    case 'V':
		std::cerr << VERSION << std::endl;
		exit(0);
		break;
	    case 'L':
		extern const char *COPYING;
		std::cerr << COPYING << std::endl;
		exit(0);
		break;
	    case 'p':
		port = 0xffff&strtol(optarg,0,0);
		if(errno) {
		    std::cerr << "Failed to parse port number" << std::endl;
		    exit(1);
		}
		break;
	    default:
		std::cerr << "Huh?" << std::endl;
		exit(1);
		break;
	}
    }

    const char *ident = rindex(*argv,'/');
    if(ident)
	++ident;
    else
	ident = *argv;
    openlog(ident,LOG_PERROR|LOG_PID,LOG_DAEMON);
    syslog(LOG_INFO,"Starting iii eye-fi manager");

    struct stat st;
    if(stat(EYEKIN_CONF_DIR,&st) || !S_ISDIR(st.st_mode))
	syslog(LOG_WARNING,"configuration directory '%s' does not exist or is not a directory",EYEKIN_CONF_DIR);
    glob_t g; int rg = glob(EYEKIN_CONF_DIR"/????????????.conf",GLOB_NOSORT,NULL,&g);
    if(rg || !g.gl_pathc)
	syslog(LOG_WARNING,"I see nothing resembling a card config in '%s'",EYEKIN_CONF_DIR);
    else
	globfree(&g);

    eyefiworker().run(port);

    closelog();
    return 0;
} catch(std::exception& e) {
    syslog(LOG_CRIT,"Exiting iii daemon, because of error condition");
    syslog(LOG_CRIT,"Exception: %s",e.what());
    return 1;
}

