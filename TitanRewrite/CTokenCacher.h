#include <curl/curl.h>
#include "mainfuncs.h"


#pragma once

#ifndef _CTOKENCACHER_H_
#define _CTOKENCACHER_H_

class CTokenCacher {
public:
	CTokenCacher();
	void cacheTokens();
	void cacheTokenForCookie(std::string cookie);
private:
	CURL* curl;
};

#endif
