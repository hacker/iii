#ifndef __EYEFIWORKER_H
#define __EYEFIWORKER_H

#include <tr1/memory>
using std::tr1::shared_ptr;

#include "soapeyefiService.h"

#include "eyekinfig.h"
#include "eyetil.h"

struct eyefi_session;

class eyefiworker : public eyefiService {
    public:
	shared_ptr<eyekinfig_t> kinfig;
	shared_ptr<tmpdir_t> indir;
	shared_ptr<mimewrite_tarfile> mime_tarfile;
	shared_ptr<mimewrite_string> mime_idigest;

	eyefiworker();
	~eyefiworker() { }

	int run(int port) __attribute__ ((noreturn));

	int StartSession(const std::string& macaddress, const std::string& cnonce,
		int transfermode, long transfermodetimestamp,
		struct rns__StartSessionResponse &r) override;
	int GetPhotoStatus(const std::string& credential, const std::string& macaddress,
		const std::string& filename, long filesize, const std::string& filesignature, int flags,
		struct rns__GetPhotoStatusResponse &r) override;
	int MarkLastPhotoInRoll(const std::string& macaddress, int mergedelta,
		struct rns__MarkLastPhotoInRollResponse &r) override;
	int UploadPhoto(int fileid, const std::string& macaddress,
		const std::string& filename, long filesize, const std::string& filesignature,
		const std::string& encryption, int flags,
		struct rns__UploadPhotoResponse &r) override;

	void *mime_writeopen(void *handle,const char *id,const char *type,const char *description,
		enum soap_mime_encoding encoding);
	int mime_write(void *handle,const char *buf,size_t len);
	void mime_writeclose(void *handle);

	eyefiService *copy() { throw std::logic_error("Not meant to be called"); }
};

#endif /* __EYEFIWORKER_H */
