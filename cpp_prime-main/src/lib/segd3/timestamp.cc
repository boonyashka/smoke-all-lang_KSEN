#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "timestamp.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace Prime {
namespace SEGD3 {

static struct tm tmSegDBeg {0, 0, 0, 6, 0, 80, 0, 0, -1};
static time_t tSegDBeg = mktime(&tmSegDBeg);

const int dayArr[2][12] = { { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
		{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };
const char *monthArr[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

bool isLeap( int year ) {
	return (((year) % 4) == 0 && (((year) % 100) != 0 || ((year) % 400) == 0));
}

Timestamp utcToTimestamp( const Utc& utc ) {
	struct tm tmp {utc.sec, utc.minute, utc.hour, utc.day, utc.month-1, utc.year - 1900, 0, 0, -1};
	time_t t = mktime(&tmp) - tSegDBeg;
	Timestamp res = Timestamp(t) * 1000000 + utc.microsec;
	return res;
}

Utc timestampToUtc( Timestamp ts ) {
	Utc result;
	int iy, nd, md, im;
	int leapIX;

	time_t t_sec = ts / 1000000;
	time_t t = t_sec + tSegDBeg;
	struct tm res = *localtime(&t);
	result.year = res.tm_year + 1900;
	result.month = res.tm_mon + 1;
	result.day = res.tm_mday;
	result.hour = res.tm_hour;
	result.minute = res.tm_min;
	result.sec = res.tm_sec;
	result.microsec = ts - (Timestamp(t_sec) * 1000000);
	return result;
}

const char* Utc::toString(bool round_to_sec) const {
	// not threadsafe code
	static char str[28];
	int ss = sec, mi = minute, hh = hour, dd = day, mm = month, yy = year;
	if(round_to_sec) {
		if(microsec >= 500000) {
			if(ss >= 59) {
				ss = 0;
				if(mi >= 59) {
					mi = 0;
					if(hh >= 23) {
						hh = 0;
						int dayMax = dayArr[isLeap(yy) ? 1 : 0][mm - 1];
						if(dd >= dayMax) {
							dd = 1;
							if(mm >= 12) {
								mm = 1;
								++yy;
							}
							else ++mm;
						}
						else ++dd;
					}
					else ++hh;
				}
				else ++mi;
			}
			else ++ss;
		}
	}
	if(round_to_sec) snprintf(str, 28, "%2d %s %4d %02d:%02d:%02d", dd, monthArr[mm-1], yy, hh, mi, ss);
	else snprintf(str, 28, "%2d %s %4d %02d:%02d:%02d.%06d", dd, monthArr[mm-1], yy, hh, mi, ss, microsec);
	return str;
};

} // namespace SEGD3
} // namespace Prime



