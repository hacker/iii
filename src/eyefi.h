//gsoap efs service name:		eyefi
//gsoap efs service location:		http://api.eye.fi/api/soap/eyefilm/v1
//gsoap efs service namespace:		EyeFi/SOAP/EyeFilm
//gsoap efs service method-action:	StartSession "urn:StartSession"
//gsoap efs service method-action:	GetPhotoStatus "urn:GetPhotoStatus"
//gsoap efs service method-action:	MarkLastPhotoInRoll "urn:MarkLastPhotoInRoll"
//gsoap rns service namespace:		http://localhost/api/soap/eyefilm

struct rns__StartSessionResponse {
    std::string credential;
    std::string snonce;
    int transfermode;
    unsigned int transfermodetimestamp;
    bool upsyncallowed;
};

int efs__StartSession(
	std::string macaddress,std::string cnonce,
	int transfermode,long transfermodetimestamp,
	struct rns__StartSessionResponse &r );

struct rns__GetPhotoStatusResponse {
    int fileid;
    long offset;
};

int efs__GetPhotoStatus(
	std::string credential, std::string macaddress,
	std::string filename, long filesize, std::string filesignature,
	struct rns__GetPhotoStatusResponse &r );

struct rns__MarkLastPhotoInRollResponse {
};

int efs__MarkLastPhotoInRoll(
	std::string macaddress, int mergedelta,
	struct rns__MarkLastPhotoInRollResponse &r );

struct rns__UploadPhotoResponse {
    bool success;
};

int efs__UploadPhoto(
	int fileid, std::string macaddress,
	std::string filename, long filesize, std::string filesignature,
	std::string encryption, int flags,
	struct rns__UploadPhotoResponse& r );
