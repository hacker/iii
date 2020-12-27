#include <signal.h>
#ifndef NDEBUG
# include <sys/resource.h>
#endif
#include <syslog.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <sys/wait.h>
#include <autosprintf.h>
#include "eyekinfig.h"
#include "eyetil.h"
#include "eyefiworker.h"
#ifdef HAVE_SQLITE
# include "iiidb.h"
#endif

#ifdef WITH_IPV6
# define BINDTO "::"
#else
# define BINDTO 0
#endif

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
#ifdef HAVE_SQLITE
    sqlite3_initialize();
#endif
    }

static void *fmimewriteopen_(struct soap *soap,
	void *handle, const char *id, const char *type, const char *description,
	enum soap_mime_encoding encoding) {
    return static_cast<eyefiworker*>(soap)->mime_writeopen(handle,id,type,description,encoding);
}
static int fmimewrite_(struct soap *soap,void *handle,const char *buf,size_t len) {
    return static_cast<eyefiworker*>(soap)->mime_write(handle,buf,len);
}
static void fmimewriteclose_(struct soap *soap,void *handle) {
    static_cast<eyefiworker*>(soap)->mime_writeclose(handle);
}

int eyefiworker::run(int bindport) {
    if(!soap_valid_socket(bind(BINDTO,bindport,64)))
	throw std::runtime_error("failed to bind()");
    signal(SIGCHLD,SIG_IGN);
    fmimewriteopen=fmimewriteopen_; fmimewrite=fmimewrite_; fmimewriteclose=fmimewriteclose_;
    while(true) {
	if(!soap_valid_socket(accept()))
	    throw std::runtime_error("failed to accept()");
	pid_t p = fork();
	if(p<0) throw std::runtime_error("failed to fork()");
	if(!p) {
	    recv_timeout = 600; send_timeout = 120;
	    (void)serve();
	    soap_destroy(this); soap_end(this); soap_done(this);
#ifndef NDEBUG
	    struct rusage ru;
	    if(getrusage(RUSAGE_SELF,&ru)) {
		syslog(LOG_NOTICE,"Failed to getrusage(): %d",errno);
	    }else{
		syslog(LOG_INFO,"maxrss: %ld\n",ru.ru_maxrss);
	    }
#endif /* NDEBUG */
	    throw throwable_exit(0);
	}
	close(socket); socket = SOAP_INVALID_SOCKET;
    }
}

static binary_t session_nonce;
#ifdef HAVE_SQLITE
    static struct {
	std::string filesignature;
	long filesize;
	std::string filename;
	inline void reset() { filesignature.erase(); filename.erase(); filesize=0; }
	inline void set(const std::string n,const std::string sig,long siz) {
	    filename = n; filesignature = sig; filesize = siz;
	}
	inline bool is(const std::string n,const std::string sig,long siz) {
	    return filesize==siz && filename==n && filesignature==sig;
	}
    }   already;
#endif /* HAVE_SQLITE */

static bool detached_child() {
    pid_t p = fork();
    if(p<0) {
	syslog(LOG_ERR,"Failed to fork away for hook execution");
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
    return false;
}

static int E(eyefiworker* efs,const char *c,const std::exception& e) {
    efs->keep_alive=0;
    syslog(LOG_ERR,"error while processing %s: %s",c,e.what());
    return soap_sender_fault(efs,gnu::autosprintf("error processing %s",c),0);
}

int eyefiworker::StartSession(
	const std::string& macaddress,const std::string& cnonce,
	int transfermode,long transfermodetimestamp,
	struct rns__StartSessionResponse &r ) try {
    syslog(LOG_INFO,
	    "StartSession request from %s with cnonce=%s, transfermode=%d, transfermodetimestamp=%ld",
	    macaddress.c_str(), cnonce.c_str(), transfermode, transfermodetimestamp );
    kinfig.reset(new eyekinfig_t(macaddress));
    umask(kinfig->get_umask());

    r.credential = binary_t(macaddress+cnonce+kinfig->get_upload_key()).md5().hex();

    r.snonce = session_nonce.make_nonce().hex();
    r.transfermode=transfermode;
    r.transfermodetimestamp=transfermodetimestamp;
    r.upsyncallowed=false;

    std::string cmd = kinfig->get_on_start_session();
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
}catch(const std::exception& e) { return E(this,"StartSession",e); }

int eyefiworker::GetPhotoStatus(
	const std::string& credential, const std::string& macaddress,
	const std::string& filename, long filesize, const std::string& filesignature,
	int flags,
	struct rns__GetPhotoStatusResponse &r ) try {
    syslog(LOG_INFO,
	    "GetPhotoStatus request from %s with credential=%s, filename=%s, filesize=%ld, filesignature=%s, flags=%d; session nonce=%s",
	    macaddress.c_str(), credential.c_str(), filename.c_str(), filesize, filesignature.c_str(), flags,
	    session_nonce.hex().c_str() );

    if(!(kinfig && kinfig->macaddress==macaddress))
	throw std::runtime_error("I'm not talking to this peer");

    std::string computed_credential = binary_t(macaddress+kinfig->get_upload_key()+session_nonce.hex()).md5().hex();

#ifndef NDEBUG
    syslog(LOG_DEBUG, " computed credential=%s", computed_credential.c_str());
#endif

    if (credential != computed_credential) throw std::runtime_error("card authentication failed");

    indir.reset(new tmpdir_t(kinfig->get_targetdir()+"/.incoming.XXXXXX"));

#ifdef HAVE_SQLITE
    iiidb_t D(*kinfig);
    seclude::stmt_t S = D.prepare(
	    "SELECT fileid FROM photo"
	    " WHERE mac=:mac AND filename=:filename"
	    "  AND filesize=:filesize AND filesignature=:filesignature"
    ).bind(":mac",macaddress)
     .bind(":filename",filename).bind(":filesize",filesize)
     .bind(":filesignature",filesignature);
    if(!S.step()) {
	r.fileid = 1; r.offset = 0;
    }else{
	r.fileid = S.column<long>(0);
	r.offset = filesize;
	already.set(filename,filesignature,filesize);
    }
#else /* HAVE_SQLITE */
    r.fileid=1, r.offset=0;
#endif /* HAVE_SQLITE */
    return SOAP_OK;
}catch(const std::exception& e) { return E(this,"GetPhotoStatus",e); }

int eyefiworker::MarkLastPhotoInRoll(
	const std::string& macaddress, int mergedelta,
	struct rns__MarkLastPhotoInRollResponse&/* r */ ) try {
    syslog(LOG_INFO,
	    "MarkLastPhotoInRoll request from %s with mergedelta=%d",
	    macaddress.c_str(), mergedelta );
    if(!(kinfig && kinfig->macaddress==macaddress))
	throw std::runtime_error("I'm not talking to this peer");

    std::string cmd = kinfig->get_on_mark_last_photo_in_roll();
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
    keep_alive = 0;
    return SOAP_OK;
}catch(const std::exception& e) { return E(this,"MarkLastPhotoInRoll",e); }

void *eyefiworker::mime_writeopen(void *handle,const char *id,const char *type,const char *description,
	enum soap_mime_encoding encoding) {
    if(!id) return NULL;
    if(!strcmp(id,"FILENAME")) {
	mime_tarfile.reset(new mimewrite_tarfile(*indir));
	return mime_tarfile.get();
    }else if(!strcmp(id,"INTEGRITYDIGEST")) {
	mime_idigest.reset(new mimewrite_string());
	return mime_idigest.get();
    }
    return NULL;
}
int eyefiworker::mime_write(void *handle,const char *buf,size_t len) {
    if(!handle) return SOAP_ERR;
    return static_cast<mimewrite_base*>(handle)->write(buf,len);
}
void eyefiworker::mime_writeclose(void *handle) {
    if(handle) static_cast<mimewrite_base*>(handle)->close();
}

int eyefiworker::UploadPhoto(
	int fileid, const std::string& macaddress,
	const std::string& filename, long filesize, const std::string& filesignature,
	const std::string& encryption, int flags,
	struct rns__UploadPhotoResponse& r ) try {
    syslog(LOG_INFO,
	    "UploadPhoto request from %s with fileid=%d, filename=%s, filesize=%ld,"
	    " filesignature=%s, encryption=%s, flags=%04X",
	    macaddress.c_str(), fileid, filename.c_str(), filesize,
	    filesignature.c_str(), encryption.c_str(), flags );
    if(!(kinfig && kinfig->macaddress==macaddress))
	throw std::runtime_error("I'm not talking to this peer");

    std::string::size_type fnl=filename.length();
    if(fnl<sizeof(".tar") || strncmp(filename.c_str()+fnl-sizeof(".tar")+sizeof(""),".tar",sizeof(".tar")))
	throw std::runtime_error(gnu::autosprintf("honestly, I expected the tarball coming here, not '%s'",filename.c_str()).operator std::string());
    std::string the_file(filename,0,fnl-sizeof(".tar")+sizeof(""));
    std::string the_log = the_file+".log";

    if(!indir) throw std::runtime_error("I haven't even created a directory!");
    shared_ptr<tmpdir_t> dir; dir.swap(indir);
    if(!mime_tarfile) throw std::runtime_error("I haven't written the tarball!");
    shared_ptr<mimewrite_tarfile> file; file.swap(mime_tarfile);
    if(!mime_idigest) throw std::runtime_error("I haven't seen the integrity digest!");
    shared_ptr<mimewrite_string> idigest; idigest.swap(mime_idigest);

#ifdef HAVE_SQLITE
    if(!file->f.tellg()) {
	if(!already.is(filename,filesignature,filesize))
	    throw std::runtime_error("got zero-length upload for unknown file");
	r.success = true;
	return SOAP_OK;
    }
#endif

    if(idigest->str != file->idigest.final(kinfig->get_upload_key()).hex())
	throw std::runtime_error("Integrity digest doesn't match, disintegrating.");

    std::string tf, lf;
    for(tarchive_t a(file->fn.c_str());a.read_next_header();) {
	std::string ep = a.entry_pathname(), f = dir->get_file(ep);
	if(ep==the_file) tf = f;
	else if(ep==the_log) lf = f;
	else continue;
	int fd=open(f.c_str(),O_CREAT|O_WRONLY,0666);
	if(fd<0)
	    throw std::runtime_error(gnu::autosprintf("failed to create output file '%s'",f.c_str()).operator std::string());
	if(!a.read_data_into_fd(fd))
	    throw std::runtime_error(gnu::autosprintf("failed to untar file into '%s'",f.c_str()).operator std::string());
	close(fd);
    }

    if(tf.empty()) throw std::runtime_error("haven't seen THE file");

    std::string::size_type ls = tf.rfind('/');
    // XXX: actually, lack of '/' signifies error here
    std::string tbn = (ls==std::string::npos)?tf:tf.substr(ls+1);
    ls = lf.rfind('/');
    std::string lbn = (ls==std::string::npos)?lf:lf.substr(ls+1);
    std::string ttf,tlf;
    bool success = false;
    std::string td = kinfig->get_targetdir();
    for(int i=0;i<32767;++i) {
	const char *fmt = i ? "%1$s/(%3$05d)%2$s" : "%1$s/%2$s";
	ttf = (const char*)gnu::autosprintf(fmt,td.c_str(),tbn.c_str(),i);
	if(!lf.empty()) tlf = (const char*)gnu::autosprintf(fmt,td.c_str(),lbn.c_str(),i);
	if( (!link(tf.c_str(),ttf.c_str())) && (lf.empty() || !link(lf.c_str(),tlf.c_str())) ) {
	    unlink(tf.c_str());
	    if(!lf.empty()) unlink(lf.c_str());
	    success=true;
	    break;
	}
    }
    std::string cmd = kinfig->get_on_upload_photo();
    if(success) {
#ifdef HAVE_SQLITE
	{
	    iiidb_t D(*kinfig);
	    D.prepare(
		    "INSERT INTO photo"
		    " (ctime,mac,fileid,filename,filesize,filesignature,encryption,flags)"
		    " VALUES"
		    " (:ctime,:mac,:fileid,:filename,:filesize,:filesignature,:encryption,:flags)"
	    ).bind(":ctime",time(0))
	     .bind(":mac",macaddress)
	     .bind(":fileid",fileid).bind(":filename",filename)
	     .bind(":filesize",filesize).bind(":filesignature",filesignature)
	     .bind(":encryption",encryption).bind(":flags",flags)
	     .step();
	}
#endif /* HAVE_SQLITE */
	if((!cmd.empty()) && detached_child()) {
	    putenv( gnu::autosprintf("EYEFI_UPLOADED_ORIG=%s",tbn.c_str()) );
	    putenv( gnu::autosprintf("EYEFI_MACADDRESS=%s",macaddress.c_str()) );
	    putenv( gnu::autosprintf("EYEFI_UPLOADED=%s",ttf.c_str()) );
	    if(!lf.empty()) putenv( gnu::autosprintf("EYEFI_LOG=%s",tlf.c_str()) );
	    char *argv[] = { (char*)"/bin/sh", (char*)"-c", (char*)cmd.c_str(), 0 };
	    execv("/bin/sh",argv);
	    syslog(LOG_ERR,"Failed to execute '%s'",cmd.c_str());
	    _exit(-1);
	}
    }

    r.success = true;
    return SOAP_OK;
}catch(const std::exception& e) { return E(this,"UploadPhoto",e); }

