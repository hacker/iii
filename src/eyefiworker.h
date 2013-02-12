#ifndef __EYEFIWORKER_H
#define __EYEFIWORKER_H

#include "soapeyefiService.h"

class eyefiworker : public eyefiService {
    public:

	eyefiworker();
	~eyefiworker();

	int run(int port) __attribute__ ((noreturn));

	int StartSession(std::string macaddress, std::string cnonce,
		int transfermode, long transfermodetimestamp,
		struct rns__StartSessionResponse &r);
	int GetPhotoStatus(std::string credential, std::string macaddress,
		std::string filename, long filesize, std::string filesignature, int flags,
		struct rns__GetPhotoStatusResponse &r);
	int MarkLastPhotoInRoll(std::string macaddress, int mergedelta,
		struct rns__MarkLastPhotoInRollResponse &r);
	int UploadPhoto(int fileid, std::string macaddress,
		std::string filename, long filesize, std::string filesignature,
		std::string encryption, int flags,
		struct rns__UploadPhotoResponse &r);

	eyefiService *copy() { throw std::logic_error("Not meant to be called"); }
};

#endif /* __EYEFIWORKER_H */
