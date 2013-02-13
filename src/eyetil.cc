#include <stdlib.h>
#include <sys/stat.h>
#include <syslog.h>
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <openssl/md5.h>
#include "eyetil.h"

#include "config.h"
#ifdef HAVE_LIBUUID
# include <uuid/uuid.h>
#endif

binary_t& binary_t::from_hex(const std::string& h) {
    std::string::size_type hs = h.length();
    if(hs&1)
	throw std::runtime_error("odd number of characters in hexadecimal number");
    size_t rvs = hs>>1;
    resize(rvs);
    const unsigned char *hp = (const unsigned char*)h.data();
    iterator oi=begin();
    char t[3] = { 0,0,0 };
    for(size_t i=0;i<rvs;++i) {
	t[0]=*(hp++); t[1]=*(hp++);
	*(oi++) = static_cast<binary_t::value_type>(0xff&strtol(t,0,16));
    }
    return *this;
}

binary_t& binary_t::from_data(const void *d,size_t s) {
    resize(s);
    std::copy((const unsigned char*)d,(const unsigned char *)d+s,
	    begin() );
    return *this;
}

binary_t& binary_t::make_nonce() {
#ifdef HAVE_LIBUUID
    uuid_t uuid;
    uuid_generate(uuid);
    from_data((unsigned char*)uuid,sizeof(uuid));
#else
    resize(16);
    std::generate_n(begin(),16,rand);
#endif /* HAVE_LIBUUID */
    return *this;
}

std::string binary_t::hex() const {
    std::string rv;
    rv.reserve((size()<<1)+1);
    char t[3] = {0,0,0};
    for(const_iterator i=begin(),ie=end();i!=ie;++i) {
	size_t rc = snprintf(t,sizeof(t),"%02x",*i);
	assert(rc<sizeof(t));
	rv += t;
    }
    return rv;
}

binary_t binary_t::md5() const {
    binary_t rv(MD5_DIGEST_LENGTH);
    if(!MD5(
		(const unsigned char*)&(front()),size(),
		(unsigned char*)&(rv.front()) ))
	throw std::runtime_error("failed to md5()");
    return rv;
}

void md5_digester::init() {
    if(!MD5_Init(&ctx)) throw std::runtime_error("failed to MD5_Init()");
}
void md5_digester::update(const void *d,size_t l) {
    if(!MD5_Update(&ctx,d,l)) throw std::runtime_error("failed to MD5_Update()");
}
binary_t md5_digester::final() {
    binary_t rv(MD5_DIGEST_LENGTH);
    if(!MD5_Final((unsigned char*)&(rv.front()), &ctx))
	throw std::runtime_error("failed to MD5_Final()");
    return rv;
}

uint16_t block512_t::tcpcksum(block512_t& data) {
    uint32_t sum = std::accumulate(data.data,data.data+words,0);
    while(uint32_t hw = sum>>16) sum = (sum&0xffff)+hw;
    return 0xffff&~sum;
}

void integrity_digester::update(const void *d_,size_t s) {
    uint8_t *d=(uint8_t*)d_;
    if(data_size) {
	int l = sizeof(data)-data_size;
	if(l>s) {
	    memmove(data.dptr(data_size),d,s); data_size+=s; return;
	}
	memmove(data.dptr(data_size),d,l); d+=l; s-=l;
	md5.update<uint16_t>( data.tcpcksum(data) );
    }
    if(s<sizeof(data)) {
	memmove(data.dptr(0),d,s); data_size=s; return;
    }
    size_t bb=s/sizeof(block512_t);
    std::transform((block512_t*)d,((block512_t*)d)+bb,
	    md5.updater<uint16_t>(),block512_t::tcpcksum);
    size_t ss=bb*sizeof(block512_t);
    d+=ss; s-=ss;
    assert(s<sizeof(block512_t));
    if(s) memmove(data.dptr(0),d,data_size=s);
}

binary_t integrity_digester::final(const std::string& ukey) {
    assert(!data_size);
    md5.update( binary_t(ukey) );
    return md5.final();
}

static void make_path_for_template(const std::string& p,mode_t m) {
    struct stat st;
    std::string pp;
    for(std::string::size_type sl=p.find('/',1);
	    sl!=std::string::npos;
	    sl=p.find('/',sl+1)) {
	if(stat( (pp=p.substr(0,sl)).c_str() ,&st)
		|| !S_ISDIR(st.st_mode)) {
	    if(mkdir(pp.c_str(),m))
		throw std::runtime_error("failed to mkdir()");
	}
    }
}

tmpdir_t::tmpdir_t(const std::string& dt) : dir(dt) {
    make_path_for_template(dt,0777);
    if(!mkdtemp((char*)dir.data()))
	throw std::runtime_error("failed to mkdtmp()");
}
tmpdir_t::~tmpdir_t() {
    assert(!dir.empty());
    if(rmdir(dir.c_str())) {
	syslog(LOG_WARNING,"Failed to remove '%s' directory",dir.c_str());
    }
}

std::string tmpdir_t::get_file(const std::string& f) {
    std::string::size_type ls = f.rfind('/');
    return dir+'/'+(
	    (ls==std::string::npos)
	    ? f
	    : f.substr(ls+1)
	);
}

tarchive_t::tarchive_t(void *p,size_t s) : a(archive_read_new()), e(0) {
    if(!a) throw std::runtime_error("failed to archive_read_new()");
    if(archive_read_support_format_tar(a)) {
	archive_read_finish(a);
	throw std::runtime_error("failed to archive_read_support_format_tar()");
    }
    if(archive_read_open_memory(a,p,s)) {
	archive_read_finish(a);
	throw std::runtime_error("failed to archive_read_open_memory()");
    }
}
tarchive_t::~tarchive_t() {
    assert(a);
    archive_read_finish(a);
}

bool tarchive_t::read_next_header() {
    assert(a);
    return archive_read_next_header(a,&e)==ARCHIVE_OK;
}

std::string tarchive_t::entry_pathname() {
    assert(a); assert(e);
    return archive_entry_pathname(e);
}

bool tarchive_t::read_data_into_fd(int fd) {
    assert(a);
    return archive_read_data_into_fd(a,fd)==ARCHIVE_OK;
}


binary_t integrity_digest(const void *ptr,size_t size,const std::string& ukey) {
    md5_digester rv;
    std::transform( (block512_t*)ptr, ((block512_t*)ptr)+size/sizeof(block512_t),
	    rv.updater<uint16_t>(), block512_t::tcpcksum );
    rv.update( binary_t(ukey) );
    return rv.final();
}

mimewrite_tarfile::mimewrite_tarfile(tmpdir_t& d) {
    f.open((fn=d.get_file("the-tarfile.tar")).c_str(),std::ios_base::in|std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
}
mimewrite_tarfile::~mimewrite_tarfile() {
    unlink(fn.c_str());
}
int mimewrite_tarfile::write(const char *buf,size_t len) {
    return f.write(buf,len) ? (idigest.update(buf,len),SOAP_OK) : SOAP_ERR;
}
