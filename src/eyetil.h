#ifndef __EYETIL_H
#define __EYETIL_H

#include <vector>
#include <string>
#include <fstream>
#include <archive.h>
#include <archive_entry.h>
#include "openssl/md5.h"
#include "soapH.h"

struct throwable_exit {
    int rc;
    throwable_exit(int rc_) : rc(rc_) { }
};

class binary_t : public std::vector<unsigned char> {
    public:
	binary_t() { }
	binary_t(size_type  n) : std::vector<unsigned char>(n) { }
	binary_t(const std::string& h) { from_hex(h); }
	binary_t(const void *d,size_t s) { from_data(d,s); }

	binary_t& from_hex(const std::string& h);
	binary_t& from_data(const void *d,size_t s);
	binary_t& make_nonce();

	std::string hex() const;
	binary_t md5() const;
};

struct md5_digester {
    MD5_CTX ctx;
    md5_digester() { init(); }

    void init();
    void update(const void *d,size_t l);
    binary_t final();

    template<typename T>
    void update(const T& x) { update(&x,sizeof(x)); }

    template<typename T>
    struct update_iterator : public std::iterator<std::output_iterator_tag,T,void,T*,T&> {
	md5_digester *d;
	update_iterator(md5_digester *d_) : d(d_) { }
	update_iterator(const update_iterator& x) : d(x.d) { }

	update_iterator& operator*() { return *this; }
	update_iterator& operator++() { return *this; }
	update_iterator& operator++(int) { return *this; }

	update_iterator& operator=(const T& x) {
	    d->update(x); return *this;
	}
    };

    template<typename T>
    update_iterator<T> updater() {
	return update_iterator<T>(this);
    }

};
template<> inline void md5_digester::update<binary_t>(const binary_t& x) {
    update((const unsigned char*)&(x.front()),x.size());
}

#pragma pack(1)
struct block512_t {
    enum { words = 512 / sizeof(uint16_t) };
    uint16_t data[words];

    inline uint8_t *dptr(size_t o) { return ((uint8_t*)this)+o; }

    static uint16_t tcpcksum(block512_t& data);
};
#pragma pack()

struct integrity_digester {
    md5_digester md5;
    size_t data_size;
    block512_t data;

    integrity_digester() : data_size(0) { }
    void update(const void *d,size_t s);
    binary_t final(const std::string& ukey);
};


class tmpdir_t {
    public:
	std::string dir;

	tmpdir_t(const std::string& dt);
	~tmpdir_t();

	std::string get_file(const std::string& f);
};

class tarchive_t {
    public:
	struct archive *a;
	struct archive_entry *e;

	tarchive_t(const char *);
	~tarchive_t();

	bool read_next_header();

	std::string entry_pathname();

	bool read_data_into_fd(int fd);
};

struct mimewrite_base {
    virtual ~mimewrite_base() { }

    virtual int write(const char *buf,size_t len) = 0;
    virtual void close() = 0;
};
struct mimewrite_string : public mimewrite_base {
    std::string str;
    int write(const char *buf,size_t len) { str.append(buf,len); return SOAP_OK; };
    void close() { }
};
struct mimewrite_tarfile : public mimewrite_base {
    std::string fn;
    std::fstream f;
    integrity_digester idigest;
    mimewrite_tarfile(tmpdir_t& d);
    ~mimewrite_tarfile();
    int write(const char *buf,size_t len);
    void close() { }
};

#endif /* __EYETIL_H */
