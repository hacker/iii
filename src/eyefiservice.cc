#include <cassert>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <iterator>
#include <syslog.h>
#include <sys/wait.h>
#include <autosprintf.h>
#include "eyekinfig.h"
#include "eyetil.h"
#include "soapeyefiService.h"

static bool detached_child() {
    pid_t p = fork();
    if(p<0) throw std::runtime_error("failed to fork()");
    if(!p) {
	p = fork();
	if(p<0) {
	    syslog(LOG_ERR,"Failed to re-fork child process");
	    _exit(-1);
	}
	if(!p) {
	    setsid();
	    for(int i=getdtablesize();i>=0;--i) close(i);
	    int i=open("/dev/null",O_RDWR); assert(i==0);
	    i = dup(i); assert(i==1);
	    i = dup(i); assert(i==2);
	    return true;
	}
	_exit(0);
    }
    int rc;
    if(waitpid(p,&rc,0)<0) throw std::runtime_error("failed to waitpid()");
    if(!WIFEXITED(rc)) throw std::runtime_error("error in forked process");
    if(WEXITSTATUS(rc)) throw std::runtime_error("forked process signalled error");
    return false;
}

int eyefiService::StartSession(
	std::string macaddress,std::string cnonce,
	int transfermode,long transfermodetimestamp,
	struct rns__StartSessionResponse &r ) {
#ifndef NDEBUG
    syslog(LOG_DEBUG,
	    "StartSession request from %s with cnonce=%s, transfermode=%d, transfermodetimestamp=%ld",
	    macaddress.c_str(), cnonce.c_str(), transfermode, transfermodetimestamp );
#endif
    r.credential = binary_t(macaddress+cnonce+eyekinfig_t(macaddress).get_upload_key()).md5().hex();
    /* TODO: better nonce generator */
    time_t t = time(0);
    r.snonce = binary_t(&t,sizeof(t)).md5().hex();
    r.transfermode=transfermode;
    r.transfermodetimestamp=transfermodetimestamp;
    r.upsyncallowed=false;

    std::string cmd = eyekinfig_t(macaddress).get_on_start_session();
    if(!cmd.empty()) {
	if(detached_child()) {
	    putenv( gnu::autosprintf("EYEFI_MACADDRESS=%s",macaddress.c_str()) );
	    putenv( gnu::autosprintf("EYEFI_TRANSFERMODE=%d",transfermode) );
	    putenv( gnu::autosprintf("EYEFI_TRANSFERMODETIMESTAMP=%ld",transfermodetimestamp) );
	    char *argv[] = { (char*)"/bin/sh", (char*)"-c", (char*)cmd.c_str(), 0 };
	    execv("/bin/sh",argv);
	    syslog(LOG_ERR,"Failed to execute '%s'",cmd.c_str());
	    _exit(-1);
	}
    }
    return SOAP_OK;
}

int eyefiService::GetPhotoStatus(
	std::string credential, std::string macaddress,
	std::string filename, long filesize, std::string filesignature,
	struct rns__GetPhotoStatusResponse &r ) {
#ifndef NDEBUG
    syslog(LOG_DEBUG,
	    "GetPhotoStatus request from %s with credential=%s, filename=%s, filesize=%ld, filesignature=%s",
	    macaddress.c_str(), credential.c_str(), filename.c_str(), filesize, filesignature.c_str() );
#endif
    r.fileid = 1; r.offset = 0;
    return SOAP_OK;
}

int eyefiService::MarkLastPhotoInRoll(
	std::string macaddress, int mergedelta,
	struct rns__MarkLastPhotoInRollResponse &r ) {
#ifndef NDEBUG
    syslog(LOG_DEBUG,
	    "MarkLastPhotoInRoll request from %s with mergedelta=%d",
	    macaddress.c_str(), mergedelta );
#endif
    std::string cmd = eyekinfig_t(macaddress).get_on_mark_last_photo_in_roll();
    if(!cmd.empty()) {
	if(detached_child()) {
	    putenv( gnu::autosprintf("EYEFI_MACADDRESS=%s",macaddress.c_str()) );
	    putenv( gnu::autosprintf("EYEFI_MERGEDELTA=%d",mergedelta) );
	    char *argv[] = { (char*)"/bin/sh", (char*)"-c", (char*)cmd.c_str(), 0 };
	    execv("/bin/sh",argv);
	    syslog(LOG_ERR,"Failed to execute '%s'",cmd.c_str());
	    _exit(-1);
	}
    }
    return SOAP_OK;
}

int eyefiService::UploadPhoto(
	int fileid, std::string macaddress,
	std::string filename, long filesize, std::string filesignature,
	std::string encryption, int flags,
	struct rns__UploadPhotoResponse& r ) {
#ifndef NDEBUG
    syslog(LOG_DEBUG,
	    "UploadPhoto request from %s with fileid=%d, filename=%s, filesize=%ld,"
	    " filesignature=%s, encryption=%s, flags=%04X",
	    macaddress.c_str(), fileid, filename.c_str(), filesize,
	    filesignature.c_str(), encryption.c_str(), flags );
#endif
    eyekinfig_t eyekinfig(macaddress);

    umask(eyekinfig.get_umask());

    std::string td = eyekinfig.get_targetdir();
    tmpdir_t indir(td+"/.incoming.XXXXXX");

    std::string jf;
    binary_t digest, idigest;

    for(soap_multipart::iterator i=mime.begin(),ie=mime.end();i!=ie;++i) {
#ifndef NDEBUG
	syslog(LOG_DEBUG,
		" MIME attachment with id=%s, type=%s, size=%ld",
		(*i).id, (*i).type, (long)(*i).size );
#endif

	if((*i).id && !strcmp((*i).id,"INTEGRITYDIGEST")) {
	    std::string idigestr((*i).ptr,(*i).size);
#ifndef NDEBUG
	    syslog(LOG_DEBUG, " INTEGRITYDIGEST=%s", idigestr.c_str());
#endif
	    idigest.from_hex(idigestr);
	}
	if( (*i).id && !strcmp((*i).id,"FILENAME") ) {
	    assert( (*i).type && !strcmp((*i).type,"application/x-tar") );
#ifdef III_SAVE_TARS
	    std::string tarfile = indir.get_file(filename);
	    {
		std::ofstream(tarfile.c_str(),std::ios::out|std::ios::binary).write((*i).ptr,(*i).size);
	    }
#endif

	    if(!jf.empty()) throw std::runtime_error("already seen tarball");
	    if(!digest.empty()) throw std::runtime_error("already have integrity digest");
	    digest = integrity_digest((*i).ptr,(*i).size,eyekinfig.get_upload_key());
#ifndef NDEBUG
	    syslog(LOG_DEBUG," computed integrity digest=%s", digest.hex().c_str());
#endif

	    tarchive_t a((*i).ptr,(*i).size);
	    if(!a.read_next_header())
		throw std::runtime_error("failed to tarchive_t::read_next_header())");
	    jf = indir.get_file(a.entry_pathname());
	    int fd=open(jf.c_str(),O_CREAT|O_WRONLY,0666);
	    assert(fd>0);
	    a.read_data_into_fd(fd);
	    close(fd);
	}
    }

    if(jf.empty()) throw std::runtime_error("haven't seen jpeg file");
    if(digest!=idigest) throw std::runtime_error("integrity digest verification failed");

    std::string::size_type ls = jf.rfind('/');
    std::string jbn = (ls==std::string::npos)?jf:jf.substr(ls+1);
    std::string tf = td+'/'+jbn;
    bool success = false;
    if(!link(jf.c_str(), tf.c_str())) {
	unlink(jf.c_str()); success = true;
    }else{
	for(int i=1;i<32767;++i) {
	    tf = (const char*)gnu::autosprintf( "%s/(%05d)%s",
		    td.c_str(), i, jbn.c_str() );
	    if(!link(jf.c_str(), tf.c_str())) {
		unlink(jf.c_str()); success = true;
		break;
	    }
	}
    }
    std::string cmd = eyekinfig.get_on_upload_photo();
    if(success && !cmd.empty()) {
	if(detached_child()) {
	    putenv( gnu::autosprintf("EYEFI_MACADDRESS=%s",macaddress.c_str()) );
	    putenv( gnu::autosprintf("EYEFI_UPLOADED=%s",tf.c_str()) );
	    char *argv[] = { (char*)"/bin/sh", (char*)"-c", (char*)cmd.c_str(), 0 };
	    execv("/bin/sh",argv);
	    syslog(LOG_ERR,"Failed to execute '%s'",cmd.c_str());
	    _exit(-1);
	}
    }

    r.success = true;
    return SOAP_OK;
}
