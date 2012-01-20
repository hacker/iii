#ifndef __EYEFIWORKER_H
#define __EYEFIWORKER_H

#include "soapeyefiService.h"

class eyefiworker : public eyefiService {
    public:

	eyefiworker();

	int run(int port) __attribute__ ((noreturn));

};

#endif /* __EYEFIWORKER_H */
