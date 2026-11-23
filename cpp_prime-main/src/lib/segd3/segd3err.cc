#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "segd3err.h"

namespace Prime {
namespace SEGD3 {

SegD3Exception::SegD3Exception(int err,const char *format,...) throw()
{
	va_list list;
	va_start(list, format);
	char sBuf[2048];
	vsnprintf(sBuf, 2048, format, list);
	va_end(list);
	sMsg = sBuf;
	if(err > 0) {
		sMsg +=" (";
		sMsg += strerror_r(err, sBuf, 2048);
		sMsg += ")";
	}
	err_num = err;
}

SegD3Exception::SegD3Exception(const char *format,...) throw()
{
	va_list list;
	va_start(list, format);
	char sBuf[2048];
	vsnprintf(sBuf, 2048, format, list);
	va_end(list);
	sMsg = sBuf;
	err_num = 0;
}




} // namespace SEGD3
} // namespace Prime


