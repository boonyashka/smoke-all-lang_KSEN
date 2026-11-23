#ifndef PRIME_LIB_SEGD3_TIMESTAMP_H_
#define PRIME_LIB_SEGD3_TIMESTAMP_H_

//
// SEG-D UTC <--> GPS conversion
// The following is ANSI C++ code.
// It should be easily translated to most other common languages like Java, C, C#, Fortran, Perl
//

#include <limits.h>

namespace Prime {
namespace SEGD3 {


#if LONG_MAX <= 2147483647L

#if defined(_MSC_VER)
typedef __int64 Timestamp;
#else
typedef long long Timestamp;
#endif

#else
typedef long Timestamp;
#endif

class Utc {
public:
	int year;
	int month;		// 1..12
	int day;		// 1..31
	int hour;		// 0..23
	int minute;		// 0..59
	int sec;		// 0..60 60=leap second
	int microsec;	// 0-999999 fraction of sec

	Utc(int y, int m, int d, int h, int min, int s, int us) : year(y), month(m), day(d), hour(h), minute(min), sec(s), microsec(us) {};
	Utc() : year(0), month(0), day(0), hour(0), minute(0), sec(0), microsec(0) {};

	const char* toString(bool round_to_sec = false) const;
};


Timestamp utcToTimestamp( const Utc& utc );

Utc timestampToUtc( Timestamp ts );


} // namespace SEGD3
} // namespace Prime

#endif /* PRIME_LIB_SEGD3_TIMESTAMP_H_ */
