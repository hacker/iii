#ifndef __EYETIL_H
#define __EYETIL_H

#include <vector>
#include <string>
#include <archive.h>
#include <archive_entry.h>

class binary_t : public std::vector<unsigned char> {
    public:
	binary_t() { }
	binary_t(size_type  n) : std::vector<unsigned char>(n) { }
	binary_t(const std::string& h) { from_hex(h); }
	binary_t(const void *d,size_t s) { from_data(d,s); }

	binary_t& from_hex(const std::string& h);
	binary_t& from_data(const void *d,size_t s);

	std::string hex() const;
	binary_t md5() const;
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

	tarchive_t(void *p,size_t s);
	~tarchive_t();

	bool read_next_header();

	std::string entry_pathname();

	bool read_data_into_fd(int fd);
};

binary_t integrity_digest(const void *ptr,size_t size,
	const std::string& ukey);

#endif /* __EYETIL_H */
