#ifndef _PRIME_LIB_SEGD3ERR_H_
#define _PRIME_LIB_SEGD3ERR_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <exception>
#include <string>

namespace Prime {
namespace SEGD3 {

class SegD3Exception : public std::exception
{
public:
	SegD3Exception() throw() : err_num(0), sMsg("No error") {};
	SegD3Exception(const SegD3Exception& e) throw() : err_num(e.err_num), sMsg(e.sMsg) {};
	SegD3Exception(int err,const char *format,...) throw();
	SegD3Exception(const char *format,...) throw();
	virtual ~SegD3Exception() throw() {};

	virtual const char *what() const throw() { return sMsg.c_str(); };
	virtual int get_errno() const { return err_num; };
protected:
	std::string sMsg;
	int err_num;
};


class SegD3Uninitialized : public SegD3Exception
{
public:
	SegD3Uninitialized() throw() : SegD3Exception("SegD3 object usage before initialization") {};
};

class SegD3IncorrectFormat : public SegD3Exception
{
public:
	SegD3IncorrectFormat() throw() : SegD3Exception("Incorrect trace samples format") {};
};


class SegD3GetFromAnotherPlace : public SegD3Exception
{
public:
	SegD3GetFromAnotherPlace(const char *param_name) throw() : SegD3Exception("Parameter %s must be obtained from another place", param_name) {};
};

class SegD3Overflow : public SegD3Exception
{
public:
	SegD3Overflow() throw() : SegD3Exception("Overflow") {};
	virtual ~SegD3Overflow() throw() {};
};

class SegD3BCDOverflow : public SegD3Overflow
{
public:
	SegD3BCDOverflow() throw() { sMsg = "Overflow when converted from BCD format to integer"; };
};

class SegD3BCDInvalid : public SegD3Exception
{
public:
	SegD3BCDInvalid() throw() : SegD3Exception("Invalid BCD data") {};
};

class SegD3ExtendedValue : public SegD3Exception
{
public:
	SegD3ExtendedValue() throw() : SegD3Exception("Get value at another place") {};
};

class SegD3BinaryOverflow : public SegD3Overflow
{
public:
	SegD3BinaryOverflow() throw() { sMsg = "Overflow when converted from binary format to integer"; };
};

class SegD3InvalidChannelSet : public SegD3Exception
{
public:
	SegD3InvalidChannelSet() throw() : SegD3Exception("Incorrect channel set header") {};
};

class SegD3TraceIncorrectScantype : public SegD3Exception
{
public:
	SegD3TraceIncorrectScantype(unsigned st, unsigned nscan, unsigned chset, unsigned trace) throw() :
		SegD3Exception("Incorrect scantype %u in demux trace header %u of channelset %u of scantype %u", st, trace, chset, nscan) {};
};

class SegD3TraceIncorrectChannelSet : public SegD3Exception
{
public:
	SegD3TraceIncorrectChannelSet(unsigned ch, unsigned nscan, unsigned chset, unsigned trace) throw() :
		SegD3Exception("Incorrect channelset %u in demux trace header %u of channelset %u of scantype %u", ch, trace, chset, nscan) {};
};


class SegD3TOCFile : public SegD3Exception
{
public:
	SegD3TOCFile() throw() : SegD3Exception("TOC file encountered") {};
};

class SegD3InconsistentVersion : public SegD3Exception
{
public:
	SegD3InconsistentVersion() throw() : SegD3Exception("SEG-D version is not consistent with this operation.") {};
};



} // namespace SEGD3
} // namespace Prime

#endif // _PRIME_LIB_SEGD3ERR_H_
