#ifndef _TRANSMISSION_H_
#define _TRANSMISSION_H_

#include "BaseTypes.hpp"
#include "wininet.h"

using ArsLexis::String;

void  DeinitConnection();
bool  FInitConnection();
void  DeinitWinet();
DWORD GetHttpBody(const String& host, const INTERNET_PORT port, const String& url, String& bodyOut);

#endif
