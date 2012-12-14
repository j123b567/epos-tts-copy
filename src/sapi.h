
#ifndef __SAPI_EPOS__
	#define __SAPI_EPOS__

#include "epos.h"
#include <winsock2.h>

// SAPI DEFINITION
void define_sapi_tokens();
void clear_sapi_tokens();
void define_phone_alts();

#define MAX_KEY_LENGTH 255
#define SAPI_TOKEN_HKEY	HKEY_LOCAL_MACHINE
#define SAPI_TOKEN_SUBKEY "SOFTWARE\\Microsoft\\Speech\\Voices\\Tokens\\"
#define SAPI_PHONE_ALTS "SOFTWARE\\Microsoft\\Speech\\PhoneConverters\\Tokens\\"
#define SAPI_PHCONV_CLSID "{9185F743-1143-4C28-86B5-BFF14F20E5C8}"
//#define SAPI_TOKEN_INFO {"NULL","409","CLSID"}
//#define SAPI_ENGINE_CLSID "{1909197A-5D63-4896-BCC6-198CD6411F09}"
#define SAPI_ENGINE_CLSID "{3851E52F-D13D-4FF3-9423-834E882D1EDF}"

#endif