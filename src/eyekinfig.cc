#include <cassert>
#include <stdexcept>
#include <autosprintf.h>
#include "eyekinfig.h"

#include "config.h"

eyekinfig_t::eyekinfig_t(const std::string& ma) : macaddress(ma), cfg(0) {
    try {
	static cfg_opt_t opts[] = {
	    CFG_STR((char*)"targetdir",(char*)"/var/lib/" PACKAGE "/%s",CFGF_NONE),
	    CFG_STR((char*)"uploadkey",(char*)"",CFGF_NONE),
	    CFG_STR((char*)"on-start-session",(char*)"",CFGF_NONE),
	    CFG_STR((char*)"on-upload-photo",(char*)"",CFGF_NONE),
	    CFG_STR((char*)"on-mark-last-photo-in-roll",(char*)"",CFGF_NONE),
	    CFG_INT((char*)"umask",022,CFGF_NONE),
	    CFG_END()
	};
	cfg = cfg_init(opts,CFGF_NONE);
	if(!cfg)
	    throw std::runtime_error("failed to cfg_init()");
	std::string::size_type ls = macaddress.rfind('/');
	std::string cf = gnu::autosprintf( EYEKIN_CONF_DIR "/%s.conf",
		macaddress.c_str()+((ls==std::string::npos)?0:ls+1) );
	int r = cfg_parse(cfg,cf.c_str());
	if(r != CFG_SUCCESS) {
	    cfg_free(cfg); cfg=0;
	    if(CFG_FILE_ERROR) throw std::runtime_error(gnu::autosprintf("failed to open configuration file '%s'",cf.c_str()).operator std::string());
	    throw std::runtime_error(gnu::autosprintf("failed to parse configuration file '%s'",cf.c_str()).operator std::string());
	}
    }catch(...) {
	if(cfg) cfg_free(cfg), cfg=0;
	throw;
    }
}

eyekinfig_t::~eyekinfig_t() {
    if(cfg) cfg_free(cfg);
}

std::string eyekinfig_t::get_targetdir() {
    assert(cfg);
    return gnu::autosprintf(cfg_getstr(cfg,"targetdir"),macaddress.c_str());
}

std::string eyekinfig_t::get_upload_key() {
    assert(cfg);
    return cfg_getstr(cfg,"uploadkey");
}

std::string eyekinfig_t::get_on_start_session() {
    assert(cfg);
    return cfg_getstr(cfg,"on-start-session");
}
std::string eyekinfig_t::get_on_upload_photo() {
    assert(cfg);
    return cfg_getstr(cfg,"on-upload-photo");
}

std::string eyekinfig_t::get_on_mark_last_photo_in_roll() {
    assert(cfg);
    return cfg_getstr(cfg,"on-mark-last-photo-in-roll");
}


int eyekinfig_t::get_umask() {
    assert(cfg);
    return 0777&cfg_getint(cfg,"umask");
}
