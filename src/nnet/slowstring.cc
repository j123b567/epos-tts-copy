#include "slowstring.h"
#include <string.h>


void CString::replace (const CString &source, const CString &target)
{
	if (!data) return;
	CString rest = *this, result = "";
	char *pos;
	while (pos = strstr (rest.data, source)) {
		result += rest.substr (0,pos-rest.data) + target;
		rest = rest.substr (pos-rest.data+strlen(source));
	}
	result += rest;
	*this = result;
} 
