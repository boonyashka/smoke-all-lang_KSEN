#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>
#include "segd3utils.h"
#include <endian.h>
#include <stdint.h>

namespace Prime {
namespace SEGD3 {

unsigned int BcdToInt (const void * p , int begin , int n) throw(SegD3BCDOverflow, SegD3BCDInvalid, SegD3ExtendedValue)
{
	int i = 0;
	unsigned int val = 0;
	const unsigned char *ptr = (const unsigned char *)p;
	bool bFF = false;

	if (n < 1 || n > 9) throw SegD3BCDOverflow();

	for (i = 0; i < n; i++) {
		val *= 10;
		unsigned int digit;
		if (begin++ & 1) digit = (*ptr++ & 15);
		else digit = (*ptr >> 4) & 15;
		if(bFF) {
			if(digit != 15) throw SegD3BCDInvalid();
		}
		else {
			if(digit > 9) {
				if(i == 0 && digit == 15) bFF = true;
				else throw SegD3BCDInvalid();
			}
		}
		val += digit;
	}
	if(bFF) throw SegD3ExtendedValue();
	return val;
}

int BinaryToInt(const void *p, int bytes) throw(SegD3Overflow)
{
	if(bytes > sizeof(int)) throw SegD3BinaryOverflow();
// Original code is strange:
//	int r = 0, q = -1;
//	const unsigned char *f = (const unsigned char *) p;
//	unsigned char *g = (unsigned char *) &r, *h = (unsigned char *) &q;
//	int i, j;
//	for(i = 0, j = bytes - 1; i < bytes; i++, j--){
//		g[i] = f[j];
//		h[i] = f[j];
//	}
//	if(f[0] < 128) return r; else return q;

	const unsigned char *f = (const unsigned char *) p;
	int res = (f[0] < 128 ? 0 : -1); // -1 needed if bytes < 4
	unsigned char *g = (unsigned char *) &res;
	int i, j;
	for(i = 0, j = bytes - 1; i < bytes; i++, j--)
		g[i] = f[j];
	return res;
}

Timestamp BinaryToTimestamp(const void *p)
{
//	const unsigned char *f = (const unsigned char *) p;
	Timestamp res = be64toh(*((uint64_t*)p));
//	unsigned char *g = (unsigned char *) &res;
//	int i, j;
//	for(i = 0, j = 7; i < 8; i++, j--)
//		g[i] = f[j];
	return res;
}

unsigned int BinaryToUint(const void *p, int bytes) throw(SegD3Overflow)
{
	if(bytes > sizeof(unsigned int)) throw SegD3BinaryOverflow();
	if(bytes == 4)
		return be32toh(*((uint32_t*)p));
	unsigned int r = 0;
	const unsigned char *f = (unsigned char *)p;
	for(int i = 0; i < bytes; i++) r = (r << 8) + f[i];
	return r;
}

unsigned long long BinaryToUint64(const void *p, int bytes) throw(SegD3Overflow)
{
	if(bytes > sizeof(unsigned long long)) throw SegD3BinaryOverflow();
	if(bytes == 8)
		return be64toh(*((uint64_t*)p));
	unsigned int r = 0;
	const unsigned char *f = (unsigned char *)p;
	for(int i = 0; i < bytes; i++) r = (r << 8) + f[i];
	return r;
}

float BinaryToFloat(const void *p) throw(SegD3Overflow)
{
	uint32_t res = be32toh(*((uint32_t*)p));
//	const unsigned char *f = (const unsigned char *) p;
//	float res = 0;
//	unsigned char *g = (unsigned char *) &res;
//	int i, j;
//	for(i = 0, j = 3; i < 4; i++, j--)
//		g[i] = f[j];
	return *((float*)&res);
}

void ibmtoieee(int Count, const void *IbmFloat, float *ThisFloat, float fmp)
{
//	Count -		Number of float point
//	IbmFloat -	Input array with float number in IBM-360-390 format
//	ThisFloat -	Ouput array with float number in this computer format
//	note:
//	IbmFloat and ThisFloat can be the same.
//	IbmFloat - is 4 byte data in form PMMM.
	const char *cw;
	char c0, c1, c2;
	int i, j;
	float a, a1, a2 = 256*256, a3 = 16;
	int n1, n2, n3;

	cw = (const char*) IbmFloat;
	for(i = 0; i < Count; i++)
	{
		a = 0.0;
		if((c0 = cw[0] & 0x7f) != 0x00)
		{
			a1 = 4096;
			for(j = 1; j <= 3; j++)
			{
				c1 = cw[j];
				a = a + a1 * ((c1 >> 4) & 0x0f);
				a1 *= 0.0625;
				a = a + a1 * (c1 & 0x0f);
				a1 *= 0.0625;
			}
			n1 = c0 - 64;
			if(n1 != 0)
			{
				n2 = (n1 > 0) ? n1 : -n1;
				n3 = 1;
				for(j = 0; j < n2; j++) a = (n1 > 0) ? a * a3 : a / a3;
			}
			if((cw[0] & 0x80) != 0) a = -a;
		}
		a /= a2;
		ThisFloat[i] = a * fmp;
		cw += 4;
	}
}


void invertieee32(int l, const unsigned char *s, unsigned char *d, float fmp)
{
//	int i = 0, k = 0;
	for(int i = 0; i < l; i++) { //, k += 4){
		*((uint32_t *)d + i) = be32toh(*((uint32_t *)s + i));
//			d[0 + k] = s[3 + k];
//			d[1 + k] = s[2 + k];
//			d[2 + k] = s[1 + k];
//			d[3 + k] = s[0 + k];
			*((float *)d + i) *= fmp;
	}
}

void invertieee64(int l, const unsigned char *s, unsigned char *d, double fmp)
{
//	int i = 0, k = 0;
	for(int i = 0; i < l; i++) { //, k += 8){
		*((uint64_t *)d + i) = be64toh(*((uint64_t *)s + i));
//			d[0 + k] = s[7 + k];
//			d[1 + k] = s[6 + k];
//			d[2 + k] = s[5 + k];
//			d[3 + k] = s[4 + k];
//			d[4 + k] = s[3 + k];
//			d[5 + k] = s[2 + k];
//			d[6 + k] = s[1 + k];
//			d[7 + k] = s[0 + k];
			*((double *)d + i) *= fmp;
	}
}


void f8015toieee(int Count, const void *src, float *dist, float fmp)
{
//	Count -         Number of float point
//	src -           Input array 8015 format source
//	dist -          Output array in this computer format
//	fmt-            Some multipler for all point
	int n, i, j;
	int p[4];
	const unsigned char *c = (const unsigned char *) src;
	const unsigned short *s = (const unsigned short *) src;
	unsigned short ss;
	unsigned char *cc = (unsigned char *) &ss;
	unsigned char b;
	float *d = (float *) dist;
	int sg;
	n = Count >> 2;

	for(j = 0; j < n; j++) {
		p[0] = ((c[0] >> 4) << 1) - 15; p[1] = ((c[0] & 0x0f) << 1) - 15;
		p[2] = ((c[1] >> 4) << 1) - 15; p[3] = ((c[1] & 0x0f) << 1) - 15;
		for(i = 0; i < 4; i++){
			ss = s[i + 1];
			b = cc[0]; cc[0] = cc[1]; cc[1] = b;
			sg = (ss >> 15) == 1 ? -1 : 1;
			ss = ss & 0x7fff;
			d[i] = powf(2.0,  p[i]) * ss * fmp * sg;
		}
		c += 10;
		s += 5;
		d += 4;
	}
}


void f8036toieee(int Count, const void *src, float *dist, float fmp)
{
//	Count -         Number of float point
//	src -           Input array 8015 format source
//	dist -          Output array in this computer format
//	fmt-            Some multipler for all point
	int  j, l;
	const char *c = (const char *) src;
	char *ll = (char*)&l;
	float *d = (float *)dist;
	float mult_coef = powf(2.0, fmp);

	for(j = 0; j < Count; ++j, ++d, c += 3) {
		l = 0;
		ll[0] = c[2];
		ll[1] = c[1];
		ll[2] = c[0];
		if(c[0] < 0) ll[3] = 0xff;
		*d = mult_coef * l;
	}
}



//
// Next code was got from DMNG's libSegD of SeiSeeMP programm
//
typedef unsigned char byte;
static int nbyte;
void swap4(void* x)
{
    byte* cbuf;
    byte  tem;

    cbuf=(byte*)x;        /* assign address of input to char array */

    tem=cbuf[0]; cbuf[0]=cbuf[3]; cbuf[3]=tem;
    tem=cbuf[2]; cbuf[2]=cbuf[1]; cbuf[1]=tem;
}
void swap8(void* x)
{
    byte* cbuf;
    byte  tem;

    cbuf=(byte*)x;        /* assign address of input to char array */

    tem=cbuf[0]; cbuf[0]=cbuf[7]; cbuf[7]=tem;
    tem=cbuf[1]; cbuf[1]=cbuf[6]; cbuf[6]=tem;
    tem=cbuf[2]; cbuf[2]=cbuf[5]; cbuf[5]=tem;
    tem=cbuf[3]; cbuf[3]=cbuf[4]; cbuf[4]=tem;
}
void swap2(void* x)
{
    byte* cbuf;
    byte  tem;

    cbuf=(byte*)x;        /* assign address of input to char array */

    tem=cbuf[0];
    cbuf[0]=cbuf[1];
    cbuf[1]=tem;
}
static char GET_C(const void* inp)
{
    int n;
    char c;

    const byte*  inps = (const byte*) inp;

    n = inps[nbyte++];
    c = (char) ((n > 127) ? (n-256) : n);

    return (c);
}
static short GET_S(const void* inp)
{
    int n1, n2;
    int n;
    short s;
    const byte*  inps = (const byte*) inp;

    n1 = inps[nbyte++];
    n2 = inps[nbyte++];
    n = (n1<<8);
    n = n | n2;
    s = (short) ((n > 32767)? n - 65536 : n);

    return (s);
}
static int GET_4(const void* inp, int swap)
{
    int num;

    memcpy(&num,(const char*)(inp)+nbyte,4);
    nbyte+=4;

    if(swap) swap4(&num);

    return (num);
}

/* f8022toieee - convert 8 bit quaternary demultiplexed data into floating numbers
 *
 * Credits:
 *      SEP:  Stew Levin
 *
 * Parameters:
 *    inp   - input sfio unit
 *    to     - output vector
 *    len    - number of packets of 4 floats in vectors
 *
 * Format 8022 is a 1 byte per word representation.
 * According to the SEG specifications, the bit
 * layout of the byte is:
 *
 *
 *  Bit       0     1     2     3     4     5     6     7
 *-----------------------------------------------------------
 * Byte 1     S    C2    C1    C0    Q-1   Q-2   Q-3   Q-4
 *
 * S=sign bit. - (One = negative number)
 * C=quaternary exponent. - This is a 3 bit positive binary exponent of 4
 *               CCC
 *   written as 4    where CCC can assume values of 0-7.
 * Q1-4-fraction. - This is a 4 bit one's complement binary fraction.
 *   The radix point is to the left of the most significant bit (Q-1)
 *                                  -1
 *   with the MSB being defined as 2 .  The fraction can have values
 *           -4        -4
 *   inp 1-2   to -1+2  .  Negative zero is invalid and must be
 *   converted to positive zero.
 *                          CCC   MP                   MP
 * Input signal = S.QQQQ x 4   x 2   millivolts where 2    is the
 *   value required to descale the data word to the recording system
 *   input level.  MP is defined in Byte 8 of each of the corre-
 *   sponding channel set descriptors in the scan type header.
 */
void f8022toieee(int Count, const void* src, float* dist, float fmp)
{
	int i;
	int ex1_4;
	int expo;
	short fraction;
	float mult_coef = powf(2.0, fmp);

	nbyte=0;
	for (i = 0; i < Count; i ++) {
		ex1_4 = GET_C(src);
		expo = ((ex1_4 >> 3) & 14) - 4;
		fraction = ex1_4 & 15;
		if (ex1_4 & 128) fraction = -(15^fraction);
		*(dist++) = ldexp((double) fraction, expo) * mult_coef;
	}
}


/* F8024_to_float - convert 16 bit quaternary demultiplexed data into floating numbers
 *
 * Credits:
 *      SEP:  Stew Levin
 *
 * Parameters:
 *    inp   - input sfio unit
 *    out     - output vector
 *    len    - number of packets of 4 floats in vectors
 *
 *
 * Format 8024 is a 2 byte per word representation.
 * According out the SEG specifications, the bit
 * layout of the bytes is:
 *
 *
 *  Bit       0     1     2     3     4     5     6     7
 *-----------------------------------------------------------
 * Byte 1     S    C2    C1    C0    Q-1   Q-2   Q-3   Q-4
 * Byte 2    Q-5   Q-6   Q-7   Q-8   Q-9   Q-10  Q-11  Q-12
 *
 * S=sign bit. - (One = negative number)
 * C=quaternary exponent. - This is a 3 bit positive binary exponent of 4
 *               CCC
 *   written as 4    where CCC can assume values of 0-7.
 * Q1-12-fraction. - This is a 12 bit one's complement binary fraction.
 *   The radix point is out the left of the most significant bit (Q-1)
 *                                  -1
 *   with the MSB being defined as 2 .  The fraction can have values
 *           -12        -12
 *   inp 1-2    out -1+2   .  Negative zero is invalid and must be
 *   converted out positive zero.
 *                                    CCC   MP                   MP
 * Input signal = S.QQQQ,QQQQ,QQQQ x 4   x 2   millivolts where 2
 *   is the value required out descale the data word out the recording
 *   system input level.  MP is defined in Byte 8 of each of the corre-
 *   sponding channel set descriptors in the scan type header.
 */

void f8024toieee(int Count, const void* src, float* dst, float fmp)
{
    int i;
    int ex1_4;
    int expo;
    short fraction;
	float mult_coef = powf(2.0, fmp);

    nbyte=0;
    for (i = 0; i < Count; i ++)
    {
        ex1_4 = GET_S(src);
        expo = ((ex1_4 >> 11) & 14) - 12;
        fraction = ex1_4 & 4095;
        if (ex1_4 & 32768) fraction = -(4095^fraction);
        *(dst++) = ldexp((double) fraction, expo) * mult_coef;
    }
}


/* F8038_to_float - convert 32 bit quaternary demultiplexed data into floating numbers
 *
 * Credits:
 *      SEP:  Stew Levin
 *
 * Parameters:
 *    inp   - input sfio unit
 *    out     - output vector
 *    len    - number of packets of 4 floats in vectors
 *
 *
 * Format 8038 is a 4 byte per word representation.
 * According out the SEG specifications, the bit
 * layout of the bytes is:
 *
 *
 *  Bit       0     1     2     3     4     5     6     7
 *-----------------------------------------------------------
 * Byte 1    Q-1   Q-2   Q-3   Q-4   Q-5   Q-6   Q-7   Q-8
 * Byte 2    Q-9   Q-10  Q-11  Q-12  Q-13  Q-14  Q-15  Q-16
 * Byte 3    Q-17  Q-18  Q-19  Q-20  Q-21  Q-22  Q-23  Q-24
 * Byte 4    Q-25  Q-26  Q-27  Q-28  Q-29  Q-30  Q-31  Q-32
 *
 * Q1-32-fraction. - This is a 32 bit two's complement binary integer.
 *                         MP                   MP
 * Input signal = Q...Q x 2   millivolts where 2
 *   is the value required out descale the data word out the recording
 *   system input level.  MP is defined in Byte 8 of each of the corre-
 *
 * Note this conversion routine assumes the target architecture is
 * already 2's complement.
 */
void f8038toieee(int Count, const void* src, float* dst, float fmp)
//void F8038_to_float (void* inp, float* out, int len)
{
    int    i;
    int    v;
    int*   p = (int*)&v;
    float mult_coef = powf(2.0, fmp);

    nbyte=0;
    for (i = 0; i < Count; i ++)
    {
        v = GET_4(src,1);
        *(dst++) = double(*p) * mult_coef;
    }
}

/* F8042_to_float - convert 8 bit hexadecimal demultiplexed data into floating numbers
 *
 * Credits:
 *      SEP:  Stew Levin
 *
 * Parameters:
 *    inp   - input sfio unit
 *    out     - output vector
 *    len    - number of floats in vector
 *
 * Format 8042 is a 1 byte per word representation.
 * According out the SEG specifications, the bit
 * layout of the byte is:
 *
 *
 *  Bit       0     1     2     3     4     5     6     7
 *-----------------------------------------------------------
 * Byte 1     S    C1    C0    Q-1   Q-2   Q-3   Q-4   Q-5
 *
 * S=sign bit. - (One = negative number)
 * C=hexadecimal exponent. - This is a 2 bit positive binary exponent of 16
 *                CC
 *   written as 16    where CC can assume values of 0-3.
 * Q1-5-fraction. - This is a 5 bit positive binary fraction.
 *   The radix point is out the left of the most significant bit (Q-1)
 *                                  -1
 *   with the MSB being defined as 2 .  The fraction can have values
 *           -5        -5
 *   inp 1-2   out -1+2  .
 *                             CC    MP                   MP
 * Input signal = S.QQQQ,Q x 16   x 2   millivolts where 2    is the
 *   value required out descale the data word out the recording system
 *   input level.  MP is defined in Byte 8 of each of the corre-
 *   sponding channel set descriptors in the scan type header.
 */

void f8042toieee(int Count, const void* src, float* out, float fmp)
{
	int i;
	int ex1_4;
	int expo;
	short fraction;
	float mult_coef = powf(2.0, fmp);

    nbyte=0;
    for (i = 0; i < Count; i ++)
    {
        ex1_4 = GET_C(src);
        expo = ((ex1_4 >> 3) & 12) - 5;
        fraction = ex1_4 & 31;
        if (ex1_4 & 128) fraction = -fraction;
        *(out++) = ldexp((double) fraction, expo) * mult_coef;
    }
}

/* F8044_to_float - convert 16 bit hexadecimal demultiplexed data into floating numbers
 *
 * Credits:
 *      SEP:  Stew Levin
 *
 * Parameters:
 *    inp   - input sfio unit
 *    out     - output vector
 *    len    - number of floats in vector
 *
 *
 * Format 8044 is a 2 byte per word representation.
 * According out the SEG specifications, the bit
 * layout of the bytes is:
 *
 *
 *  Bit       0     1     2     3     4     5     6     7
 *-----------------------------------------------------------
 * Byte 1     S    C1    C0    Q-1   Q-2   Q-3   Q-4   Q-5
 * Byte 2    Q-6   Q-7   Q-8   Q-9   Q-10  Q-11  Q-12  Q-13
 *
 * S=sign bit. - (One = negative number)
 * C=hexadecimal exponent. - This is a 2 bit positive binary exponent of 16
 *                CC
 *   written as 16    where CC can assume values of 0-3.
 * Q1-13-fraction. - This is a 13 bit positive binary fraction.
 *   The radix point is out the left of the most significant bit (Q-1)
 *                                  -1
 *   with the MSB being defined as 2 .  The fraction can have values
 *           -13        -13
 *   inp 1-2    out -1+2   .
 *                                       CC    MP                   MP
 * Input signal = S.QQQQ,QQQQ,QQQQ,Q x 16   x 2   millivolts where 2
 *   is the value required out descale the data word out the recording system
 *   input level.  MP is defined in Byte 8 of each of the corre-
 *   sponding channel set descriptors in the scan type header.
 */

void f8044toieee(int Count, const void* src, float* out, float fmp)
{
	int i;
	int ex1_4;
	int expo;
	short fraction;
	float mult_coef = powf(2.0, fmp);

    nbyte=0;
    for (i = 0; i < Count; i ++)
    {
        ex1_4 = GET_S(src);
        expo = ((ex1_4 >> 11) & 12) - 13;
        fraction = ex1_4 & 8191;
        if (ex1_4 & 32768) fraction = -fraction;
        *(out++) = ldexp((double) fraction, expo) * mult_coef;
    }
}


} // namespace SEGD3
} // namespace Prime
