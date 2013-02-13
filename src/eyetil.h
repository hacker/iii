#ifndef __EYETIL_H
#define __EYETIL_H

#include <vector>
#include <string>
#include <archive.h>
#include <archive_entry.h>
#include "openssl/md5.h"

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

	tarchive_t(void *p,size_t s);
	~tarchive_t();

	bool read_next_header();

	std::string entry_pathname();

	bool read_data_into_fd(int fd);
};

binary_t integrity_digest(const void *ptr,size_t size,
	const std::string& ukey);

#endif /* __EYETIL_H */
