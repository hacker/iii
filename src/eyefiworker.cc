#include <signal.h>
#include <stdexcept>
#include "eyefiworker.h"

eyefiworker::eyefiworker()
    : eyefiService(SOAP_IO_STORE|SOAP_IO_KEEPALIVE) {
	bind_flags = SO_REUSEADDR; max_keep_alive = 0;
	socket_flags =
#if defined(MSG_NOSIGNAL)
	    MSG_NOSIGNAL
#elif defined(SO_NOSIGPIPE)
	    SO_NOSIGPIPE
#else
#error Something is wrong with sigpipe prevention on the platform
#endif
	    ;
    }

int eyefiworker::run(int port) {
    if(!soap_valid_socket(bind(0,port,64)))
	throw std::runtime_error("failed to bind()");
    signal(SIGCHLD,SIG_IGN);
    while(true) {
	if(!soap_valid_socket(accept()))
	    throw std::runtime_error("failed to accept()");
	pid_t p = fork();
	if(p<0) throw std::runtime_error("failed to fork()");
	if(!p) {
	    recv_timeout = 600; send_timeout = 120;
	    (void)serve();
	    soap_destroy(this); soap_end(this); soap_done(this);
	    _exit(0);
	}
	close(socket); socket = SOAP_INVALID_SOCKET;
    }
}
