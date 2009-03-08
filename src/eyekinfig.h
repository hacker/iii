#ifndef __EYEKINFIG_H
#define __EYEKINFIG_H

#include <confuse.h>
#include <string>

class eyekinfig_t {
    public:
	std::string macaddress;
	cfg_t *cfg;

	eyekinfig_t(const std::string& ma);
	~eyekinfig_t();

	std::string get_targetdir();
	std::string get_upload_key();

	std::string get_on_start_session();
	std::string get_on_upload_photo();
	std::string get_on_mark_last_photo_in_roll();

	int get_umask();
};

#endif /* __EYEKINFIG_H */
