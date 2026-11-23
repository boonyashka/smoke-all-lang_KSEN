#ifndef PRIME_LIB_SEGD3_UTILS_H_
#define PRIME_LIB_SEGD3_UTILS_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "segd3err.h"
#include "timestamp.h"

namespace Prime {
namespace SEGD3 {

//
// Преобразование данных из BCD формата
// Параметры:
// p - указатель на данные
// begin = {0,1} : 0 - начать с первой тетрады байта на который указывает p, 1 - начать со второй тетрады
// n = {1 - 8} : количество символов BCD (тетрад)
// Может кидать SegD3BCDOverflow, SegD3BCDInvalid и SegD3BCDExtendedValue. В последнем случае
// в поле содержится недопустимое для BCD формата значение (все цифры = F), указывающее в данном
// случае на то, что значение для этого поля должно браться в другом месте.
//
unsigned int BcdToInt (const void * p , int begin , int n) throw(SegD3BCDOverflow, SegD3BCDInvalid, SegD3ExtendedValue);

//
// Считывание данных из строки двоичного формата в целое
// p - указатель на данные
// bytes = {1 - sizeof(int)} : количество байт
//
int BinaryToInt(const void *p, int bytes = 4) throw(SegD3Overflow);

//
// Считывание данных из строки двоичного формата в беззнаковое целое
// p - указатель на данные
// bytes = {1 - sizeof(unsigned int)} : количество байт
//
unsigned int BinaryToUint(const void *p, int bytes = 4) throw(SegD3Overflow);

//
// Считывание данных из строки двоичного формата в беззнаковое целое
// p - указатель на данные
// bytes = {1 - sizeof(unsigned long long)} : количество байт
//
unsigned long long BinaryToUint64(const void *p, int bytes = 8) throw(SegD3Overflow);

//
// Считывание данных из строки двоичного формата в float
// p - указатель на данные
//
float BinaryToFloat(const void *p) throw(SegD3Overflow);

//
// Считывание данных из строки двоичного формата в SEGD Timestamp
// p - указатель на данные
//
Timestamp BinaryToTimestamp(const void *p);

void ibmtoieee(int Count, const void *IbmFloat, float *ThisFloat, float fmp);

void invertieee32(int Count, const unsigned char *src, unsigned char *dist, float fmp);
void invertieee64(int Count, const unsigned char *src, unsigned char *dist, double fmp);

void f8015toieee(int Count, const void *src, float *dist, float fmp);

void f8036toieee(int Count, const void *src, float *dist, float fmp);

void f8022toieee(int Count, const void* src, float* dist, float fmp);
void f8024toieee(int Count, const void* src, float* dst, float fmp);
void f8038toieee(int Count, const void* src, float* dst, float fmp);
void f8042toieee(int Count, const void* src, float* out, float fmp);
void f8044toieee(int Count, const void* src, float* out, float fmp);

} // namespace SEGD3
} // namespace Prime



#endif /* PRIME_LIB_SEGD3_UTILS_H_ */
