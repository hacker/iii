#include <sys/wait.h>
#include <stdexcept>
#include "eyefiworker.h"

eyefiworker::eyefiworker()
    : eyefiService(SOAP_IO_STORE|SOAP_IO_KEEPALIVE) {
	bind_flags = SO_REUSEADDR; max_keep_alive = 0;
    }

int eyefiworker::run(int port) {
    if(!soap_valid_socket(bind(0,port,5)))
	throw std::runtime_error("failed to bind()");
    while(true) {
	while(waitpid(-1,0,WNOHANG)>0);
	if(!soap_valid_socket(accept()))
	    throw std::runtime_error("failed to accept()");
	pid_t p = fork();
	if(p<0) throw std::runtime_error("failed to fork()");
	if(!p) {
	    (void)serve();
	    soap_destroy(this); soap_end(this); soap_done(this);
	    _exit(0);
	}
	close(socket); socket = SOAP_INVALID_SOCKET;
    }
}
