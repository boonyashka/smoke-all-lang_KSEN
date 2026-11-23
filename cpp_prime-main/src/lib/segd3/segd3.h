#ifndef _PRIME_LIB_SEGD3_H_
#define _PRIME_LIB_SEGD3_H_

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "trshead/types.h"
#include "Seism/Trace.h"
#include "segd3utils.h"
#include "segd3err.h"
#include "timestamp.h"
#include <memory>
#include <sys/types.h>
#include <dirent.h>


#define SEGD_HEADER_BLOCK 32
#define SEGD_CHANNELSET_BLOCK2 32
#define SEGD_CHANNELSET_BLOCK3 96
#define DEMUX_TRACE_HEADER_BLOCK 20
#define TOC_LABEL "SEGD TOC FILE"


namespace Prime {
namespace SEGD3 {

enum SegD3FileType { SEGD3_FTYPE_LABEL, SEGD3_FTYPE_RECORDS, SEGD3_FTYPE_TOC, SEGD3_FTYPE_UNKNOWN };
enum SegD3FormatCode {
	FMT_20BINARY = 8015,
	FMT_8QUATERNARY = 8022,
	FMT_16QUATERNARY = 8024,
	FMT_24INTEGER = 8036,
	FMT_32INTEGER = 8038,
	FMT_8HEX = 8042,
	FMT_16HEX = 8044,
	FMT_32HEX = 8048,
	FMT_32IEEE = 8058,
	FMT_64IEEE = 8080
};

enum SegDFormat_t { SEGD_FORMAT_BCD, SEGD_FORMAT_BINARY, SEGD_FORMAT_STRING, SEGD_FORMAT_TIMESTAMP };
const char* getFormatName(Prime::SEGD3::SegDFormat_t fmt);
Prime::SEGD3::SegDFormat_t getFormatFromName(const char* fmt_name);

struct RemapItem {
	Prime::HdrField prime_field;
	std::string segd_header_type;
	int block_num_or_id; // block_id if GehHeader or TraceHeaderExtension or block number if Extended or External header or ChannelSet
//	int offset; // in tetrades for BCD (4 bit)
//	int length; // in tetrades for BCD (4 bit)
	int start_bit;
	int end_bit;
	SegDFormat_t format;

	RemapItem() : prime_field(H_ERR), block_num_or_id(0), start_bit(0), end_bit(0), format(SEGD_FORMAT_BCD), segd_header_type("Extended header") {};
	bool operator==(const RemapItem& rhs) const {
		return (prime_field == rhs.prime_field && segd_header_type == rhs.segd_header_type && block_num_or_id == rhs.block_num_or_id &&
				start_bit == rhs.start_bit && end_bit == rhs.end_bit && format == rhs.format);
	};
	bool operator!=(const RemapItem& rhs) const {
		return !(prime_field == rhs.prime_field && segd_header_type == rhs.segd_header_type && block_num_or_id == rhs.block_num_or_id &&
				start_bit == rhs.start_bit && end_bit == rhs.end_bit && format == rhs.format);
	};
};

struct ChannelTypeDescr
{
	unsigned char n;
	const char* descr;

	ChannelTypeDescr(): n(0), descr(NULL) {};
	ChannelTypeDescr(unsigned char t, const char* d): n(t), descr(d) {};

	bool operator<(const ChannelTypeDescr& ct) const { return (n < ct.n); };
};

typedef struct ChannelTypeDescr ChannelTypeDescrT;

ChannelTypeDescrT* get_channel_types(int version, int *pTNum);

class StorageUnitLabel
{
	char sLabel[128];
public:
	StorageUnitLabel() {};

	char* get_buffer() { return &sLabel[0]; }; // for initialization
	bool is_valid() const { return check_revision(); };
	bool operator<(const StorageUnitLabel& sul) const ;

	unsigned get_sequence_number() const  { return BcdToInt(&sLabel[0], 0, 4); };
	bool check_revision() const  { return (memcmp(&sLabel[4], "SD3.0", 5) == 0); };
	size_t get_maximum_block_size() const  { return 0; };
	bool is_fixed_record_len() const  { return (memcmp(&sLabel[9], "FIXREC", 6) == 0); };
	std::string get_serial_number() const  { return std::string(&sLabel[50], 12); };
};

typedef enum {
	BCD_VALUE, BYTE_VALUE, INT_VALUE, UINT_VALUE, FLOAT_VALUE, DOUBLE_VALUE, TIMESTAMP_VALUE
} SegdDValType;

typedef struct {
	const char *field_name;
	SegdDValType valtype;
	int offset_halfbyte;
	int length_halfbyte;
} FieldsDescription;

struct SegDInfo
{
	int nFileNum;
	unsigned nVersionMajor;
	unsigned nVersionMinor;
	int nSPnum;
	int nSPfrac;
	int nSPind;
	int nSLN;
	int nSLNfrac;
	int nScanTypeNum;
	int nChannelCode;
	std::string sChannelType;
	unsigned nChannelSetNum;
	unsigned nSamplingIntMks;
	unsigned nTraceSamples;
	unsigned nChannelTraces;
	float fDSMFactor;
	int nStartTimeMks;
	int nEndTimeMks;
};

struct Fn_SegDInfo_FileChan_Cmp {
	bool operator()(const SegDInfo& lhs, const SegDInfo& rhs) const {
		if(lhs.nFileNum != rhs.nFileNum)
			return (lhs.nFileNum < rhs.nFileNum);
		if(lhs.nScanTypeNum != rhs.nScanTypeNum)
			return (lhs.nScanTypeNum < rhs.nScanTypeNum);
		return (lhs.nChannelSetNum < rhs.nChannelSetNum);
	}
};

struct Fn_SegDInfo_ChanInfo_Cmp {
	bool tracenum_cmp;
	Fn_SegDInfo_ChanInfo_Cmp(bool _tracenum_cmp = true) : tracenum_cmp(_tracenum_cmp) {};

	bool operator()(const SegDInfo& lhs, const SegDInfo& rhs) const {
		if(lhs.nChannelCode != rhs.nChannelCode)
			return (lhs.nChannelCode < rhs.nChannelCode);
		if(lhs.nSamplingIntMks != rhs.nSamplingIntMks)
			return (lhs.nSamplingIntMks < rhs.nSamplingIntMks);
		if(lhs.nTraceSamples != rhs.nTraceSamples)
			return (lhs.nTraceSamples < rhs.nTraceSamples);
//		if(lhs.nStartTimeMks != rhs.nStartTimeMks)
//			return (lhs.nStartTimeMks < rhs.nStartTimeMks);
//		if(lhs.nEndTimeMks != rhs.nEndTimeMks)
//			return (lhs.nEndTimeMks < rhs.nEndTimeMks);
//		if(lhs.fDSMFactor != rhs.fDSMFactor)
//			return (lhs.fDSMFactor < rhs.fDSMFactor);
		if(tracenum_cmp)
			return (lhs.nChannelTraces < rhs.nChannelTraces);
		return false;
	}
};

void RemapField(Prime::IO::TraceHead& h, const RemapItem& ri, const unsigned char* sData, size_t data_length) ;

class DumpInterface
{
protected:
	unsigned char *content;
	size_t length;

	unsigned char high_tetrad(unsigned char byte) const { return byte >> 4; };
	unsigned char low_tetrad(unsigned char byte) const { return byte & 0x0F; };
public:

	DumpInterface(unsigned char *_content, size_t _len) : content(_content), length(_len) {};
	virtual ~DumpInterface() { content = nullptr; };

	std::string dump_raw(int bytes_in_row = 12, int group_size = 4, int selected_field = -1) const;
	std::string dump() const;
	virtual const FieldsDescription* get_block_descr(int *num) const = 0;
};

class GeneralBlock : public DumpInterface
{
protected:
	unsigned char sHeader[SEGD_HEADER_BLOCK];
public:
	GeneralBlock() : DumpInterface(&sHeader[0], SEGD_HEADER_BLOCK) { memset(&sHeader[0], 0, SEGD_HEADER_BLOCK); };
	GeneralBlock(const GeneralBlock& h) : DumpInterface(&sHeader[0], SEGD_HEADER_BLOCK) { memcpy(&sHeader[0], &h.sHeader[0], SEGD_HEADER_BLOCK); };
	virtual ~GeneralBlock() {};
	GeneralBlock& operator=(const GeneralBlock& h) { memcpy(&sHeader[0], &h.sHeader[0], SEGD_HEADER_BLOCK); content = &sHeader[0]; return *this; };
	virtual const FieldsDescription* get_block_descr(int *num) const { if(num) *num = 0; return nullptr; };

	unsigned char* get_buffer() { return &sHeader[0]; }; // for initialization
	virtual bool is_valid() const { return true; };
	unsigned char get_type(unsigned ver)  { return (unsigned char)(ver == 2 ? sHeader[18] : sHeader[31]); };
	void RemapField(Prime::IO::TraceHead& h, const RemapItem& ri) const ;
};

typedef std::shared_ptr<GeneralBlock> GeneralBlockPtr;

class GeneralHeader1 : public GeneralBlock
{
	static FieldsDescription block_descr[];
public:
	GeneralHeader1() : GeneralBlock() {};
	GeneralHeader1(const GeneralBlock& h) : GeneralBlock(h) {};
	const FieldsDescription* get_block_descr(int *num) const { if(num) *num = 19; return block_descr; };
	bool is_valid() const { return true; };

	unsigned get_file_number() const  { return BcdToInt(&sHeader[0], 0, 4); };
	SegD3FormatCode get_format() const ;
	size_t get_format_sample_size() const ;
	unsigned get_headers_number() const  { return BcdToInt(&sHeader[11], 0,1); }; // Total general headers without general header 1 (for segD3 >= 2)
	double get_base_scan_interval() const ; // seconds
	double get_record_length() const ; // seconds
	unsigned get_scantypes_number() const  { return BcdToInt(&sHeader[27], 0,2); };
	unsigned get_channelsets() const  { return BcdToInt(&sHeader[28], 0,2); };
	unsigned get_sample_skew_blocks() const  { return BcdToInt(&sHeader[29], 0,2); };
	unsigned get_extended_header_blocks() const  { return BcdToInt(&sHeader[30], 0,2); };
	unsigned get_external_header_blocks() const  { return BcdToInt(&sHeader[31], 0,2); };
	bool is_normal_record() const
	{
		unsigned rec_type = BcdToInt(&sHeader[25], 0,1);
		return (rec_type == 8);
	};
	void get_date(int& year, int& day, int& hour, int& minute, int& second) const ;
	unsigned char get_type()  { return 1; };

	void test() const;
};

class GeneralHeader2 : public GeneralBlock
{
	static FieldsDescription block_descr2[], block_descr3[];

	mutable unsigned version_major, version_minor;

	void extract_version() const {
		version_major = sHeader[10];
		version_minor = sHeader[11];
	};
	unsigned get_extended_header_blocks3() const  { return BinaryToUint(&sHeader[5], 3); };
	unsigned get_extended_header_blocks2() const  { return BinaryToUint(&sHeader[5], 2); };

	unsigned get_external_header_blocks3() const  { return BinaryToUint(&sHeader[27], 3); };
	unsigned get_external_header_blocks2() const  { return BinaryToUint(&sHeader[7], 2); };

	unsigned get_general_trailer_blocks3() const  { return BinaryToUint(&sHeader[12], 4); };
	unsigned get_general_trailer_blocks2() const  { return BinaryToUint(&sHeader[12], 2); };

	double get_record_length3() const  { return double(BinaryToUint(&sHeader[16], 4))  * (1E-6); }; // seconds
	double get_record_length2() const  { return double(BinaryToUint(&sHeader[14], 3))  * (1E-6); }; // seconds

public:
	GeneralHeader2() : GeneralBlock(), version_major(0), version_minor(0) {};
	GeneralHeader2(const GeneralBlock& h) : GeneralBlock(h) { version_major = sHeader[10]; version_minor = sHeader[11]; };
	const FieldsDescription* get_block_descr(int *num) const { if(num) *num = (version_major == 3 ? 13 : 10); return (version_major == 3 ? block_descr3 : block_descr2); };

	bool is_valid() const { return ((get_version_major() == 3 && sHeader[31] == 0x02) || (get_version_major() == 2 && sHeader[18] == 0x02)); };

	unsigned get_version_major() const { if(version_major == 0) extract_version(); return version_major; };
	unsigned get_version_minor() const { if(version_major == 0) extract_version(); return version_minor; };

	unsigned get_file_number() const  { return BinaryToUint(&sHeader[0], 3); };
	unsigned get_channelsets() const  { return BinaryToUint(&sHeader[3], 2); };
	unsigned get_sample_skew_blocks() const  { if(get_version_major() < 3) throw SegD3InconsistentVersion(); return BinaryToUint(&sHeader[8], 2); };
	unsigned get_general_trailer_blocks() const  { if(get_version_major() <= 2) return get_general_trailer_blocks2(); return get_general_trailer_blocks3(); };
	double get_record_length() const  { if(get_version_major() <= 2) return get_record_length2(); return get_record_length3(); }; // seconds
	unsigned get_headers_number() const  { if(get_version_major() < 3) throw SegD3InconsistentVersion(); return BinaryToUint(&sHeader[22], 2); }; // Total general headers without general header 1 (for segD3 >= 2)
	unsigned get_extended_header_blocks() const  { if(get_version_major() <= 2) return get_extended_header_blocks2(); return get_extended_header_blocks3(); };
	unsigned get_external_header_blocks() const  { if(get_version_major() <= 2) return get_external_header_blocks2(); return get_external_header_blocks3(); };
	double get_base_scan_interval() const  { if(get_version_major() < 3) throw SegD3InconsistentVersion(); return double(BinaryToUint(&sHeader[24], 3)) * (1E-6); }; // seconds
};

class GeneralHeader3 : public GeneralBlock
{
	static FieldsDescription block_descr[];
public:
	GeneralHeader3() : GeneralBlock() {};
	GeneralHeader3(const GeneralBlock& h) : GeneralBlock(h) {};
	const FieldsDescription* get_block_descr(int *num) const { if(num) *num = 7; return block_descr; };

	bool is_valid() const { return (sHeader[31] == 0x03); };

	Timestamp get_time_zero() const  { return BinaryToTimestamp(&sHeader[0]); };
	size_t get_record_size() const  { return BinaryToUint64(&sHeader[8], 8); };
	size_t get_headers_and_data_size() const  { return BinaryToUint64(&sHeader[16], 8); };
	size_t get_headers_size() const  { return BinaryToUint(&sHeader[24], 4); };
	bool is_extended_recording_mode() const  { return (sHeader[28] > 0); };
	bool is_relative_time_mode() const  { return (sHeader[29] > 0); };
};

class SourceInfo : public GeneralBlock
{
	static FieldsDescription block_descr[];
public:
	SourceInfo() : GeneralBlock() {};
	SourceInfo(const GeneralBlock& h) : GeneralBlock(h) {};
	const FieldsDescription* get_block_descr(int *num) const { if(num) { if(sHeader[31] >= 0x16 && sHeader[31] <= 0x18) *num = 8; else *num = 7; } return block_descr; };
	bool is_valid(int ver) const { return ((ver == 3 && ((sHeader[31] >= 0x15 && sHeader[31] <= 0x19) || sHeader[31] == 0x1F)) || ver == 2); };

	unsigned char get_source_type() const  { return sHeader[31]; };
	unsigned get_file_number() const  { return BinaryToUint(&sHeader[0], 3); };
	int get_line() const  { return BinaryToInt(&sHeader[3], 3); };
	int get_line_fraction() const  { return BinaryToUint(&sHeader[6], 2); };
	int get_point() const  { return BinaryToInt(&sHeader[8], 3); };
	int get_point_fraction() const  { return BinaryToUint(&sHeader[11], 2); };
	unsigned char get_point_index() const  { return sHeader[13]; };
	virtual unsigned get_depth_cm(unsigned ver) const ;
};

class ChannelSet
{
protected:
	unsigned char sData[SEGD_CHANNELSET_BLOCK3];
	unsigned version_major, version_minor;
	off_t nDataLength, nDataOffset;
	unsigned nTraceLength, nTraceSamples, nSamplingIntervalmks;
public:
	ChannelSet() : version_major(0), version_minor(0), nTraceLength(0), nDataLength(0), nDataOffset(0), nTraceSamples(0), nSamplingIntervalmks(0) { memset(&sData[0], 0, SEGD_CHANNELSET_BLOCK3); };
	virtual ~ChannelSet() {};

	unsigned char* get_buffer() { return &sData[0]; }; // for initialization
	virtual bool is_valid() const = 0;
	bool is_dummy() const { return (get_chanels_number() == 0); };
	void RemapField(Prime::IO::TraceHead& h, const RemapItem& ri) ;

	unsigned& trace_length() { return nTraceLength; };
	const unsigned& trace_length() const { return nTraceLength; };

	off_t& data_length() { return nDataLength; };
	const off_t& data_length() const { return nDataLength; };

	off_t& data_offset() { return nDataOffset; };
	const off_t& data_offset() const { return nDataOffset; };

	unsigned& trace_samples() { return nTraceSamples; };
	const unsigned& trace_samples() const { return nTraceSamples; };

	unsigned& sampling_interval_mks() { return nSamplingIntervalmks; };
	const unsigned& sampling_interval_mks() const { return nSamplingIntervalmks; };

	unsigned get_version_major() const { return version_major; };
	unsigned get_version_minor() const { return version_minor; };

	unsigned get_scantype_number() const  { return BcdToInt(&sData[0], 1, 2); };

	virtual unsigned get_number() const  = 0;
	virtual unsigned char get_type() const  = 0;
	virtual int get_start_time_mks() const  = 0;
	virtual int get_end_time_mks() const  = 0;
	virtual unsigned get_samples_number() const  { return 0; };
	virtual float get_DSM_factor() const  = 0;
	virtual unsigned get_chanels_number() const  = 0;
	virtual unsigned get_sampling_interval() const  = 0; // microseconds
	virtual unsigned get_subscan_intervals() const  { return 1; };
	virtual unsigned char get_trace_header_extensions() const  = 0;
	virtual unsigned char get_streamer() const  { return sData[30]; };
};

class ChannelSet2 : public ChannelSet
{
public:
	ChannelSet2(unsigned minor_ver = 1) : ChannelSet() { version_major = 2; version_minor = minor_ver; };
	bool is_valid() const { return true; };

	unsigned get_number() const  { return BcdToInt(&sData[1], 0, 2); };
	unsigned char get_type() const  { return BcdToInt(&sData[10], 0, 1); };
	int get_start_time_mks() const  { return BinaryToInt(&sData[2], 2) * 2 * 1000; };
	int get_end_time_mks() const  { return BinaryToInt(&sData[4], 2) * 2 * 1000; };
//	unsigned get_samples_number() const  { return BinaryToUint(&sData[12], 4); };
	float get_DSM_factor() const  {
		unsigned char imp0 = sData[6];
		unsigned char imp1 = sData[7];
		int z = imp1 & 0x80;
		int m = (imp1 & 0x7F) << 8 | imp0;
		float res = (z ? -1 : 1) * m / float(1024);
		if(__isinff(res) || __isnanf(res)) res = float(1);
		else res = powf(2., res);
		return res;
	};
	unsigned get_chanels_number() const  { return BcdToInt(&sData[8], 0, 4); };
	unsigned get_sampling_interval() const  { throw SegD3InconsistentVersion(); };
	unsigned get_subscan_intervals() const  { unsigned ss_exp = BcdToInt(&sData[11], 0, 1); return (ss_exp > 0 ? (unsigned)1 << ss_exp : unsigned(1)); };
	unsigned char get_trace_header_extensions() const  { return (sData[28] & 0x0F); };
};

class ChannelSet3 : public ChannelSet
{
public:
	ChannelSet3(unsigned minor_ver = 0) : ChannelSet() { version_major = 3; version_minor = minor_ver; };
	bool is_valid() const { return ((sData[31] == 0x30) && (sData[63] == 0x31) && (sData[95] == 0x32)); };

	unsigned get_number() const  { return BinaryToUint(&sData[1], 2); };
	unsigned char get_type() const  { return sData[3]; };
	int get_start_time_mks() const  { return BinaryToInt(&sData[4], 4); };
	int get_end_time_mks() const  { return BinaryToInt(&sData[8], 4); };
	unsigned get_samples_number() const  { return BinaryToUint(&sData[12], 4); };
	float get_DSM_factor() const  { float res = BinaryToFloat(&sData[16]); if(__isinff(res) || __isnanf(res)) res = float(1); return res; };
	unsigned get_chanels_number() const  { return BinaryToUint(&sData[20], 3); };
	unsigned get_sampling_interval() const  { return BinaryToUint(&sData[23], 3); }; // microseconds
	unsigned char get_trace_header_extensions() const  { return sData[27]; };
};

typedef std::vector<ChannelSet3> ChannelSets3;
typedef std::vector<ChannelSet2> ChannelSets2;

class ScanType
{
	std::vector<ChannelSet3> vChannelSets3;
	std::vector<ChannelSet2> vChannelSets2;
	unsigned version_major, version_minor;
	std::vector<unsigned char> vSampleSkew;
public:
	ScanType() : version_major(0), version_minor(0) {};
	//ScanType(int channel_sets, int sample_skew_headers);
	void add_channel_set(const ChannelSet3& cs)  {
		if(version_major == 0 || version_major == 3) {
			if(!version_major) { version_major = 3; version_minor = cs.get_version_minor(); }
			if(version_minor == cs.get_version_minor()) vChannelSets3.push_back(cs);
			else throw SegD3InconsistentVersion();
		}
		else throw SegD3InconsistentVersion();
	};
	void add_channel_set(const ChannelSet2& cs)  {
		if(version_major == 0 || version_major == 2) {
			if(!version_major) { version_major = 2; version_minor = cs.get_version_minor(); }
			if(version_minor == cs.get_version_minor()) vChannelSets2.push_back(cs);
			else throw SegD3InconsistentVersion();
		}
		else throw SegD3InconsistentVersion();
	};

	unsigned char* get_sample_skew_buffer(unsigned sz_buf)  { vSampleSkew.resize(sz_buf); return vSampleSkew.data(); };
	void get_channeltype_list(std::vector<std::string>& chantypes) const ;
	ChannelSet3& get_channelset3(size_t chset_num) { if(version_major != 3) throw SegD3InconsistentVersion(); return vChannelSets3[chset_num]; };
	ChannelSet2& get_channelset2(size_t chset_num) { if(version_major != 2) throw SegD3InconsistentVersion(); return vChannelSets2[chset_num]; };
	ChannelSets3& get_channelsets3() { if(version_major != 3) throw SegD3InconsistentVersion(); return vChannelSets3; };
	ChannelSets2& get_channelsets2() { if(version_major != 2) throw SegD3InconsistentVersion(); return vChannelSets2; };
	const ChannelSets3& get_channelsets3() const { if(version_major != 3) throw SegD3InconsistentVersion(); return vChannelSets3; };
	const ChannelSets2& get_channelsets2() const { if(version_major != 2) throw SegD3InconsistentVersion(); return vChannelSets2; };
};

typedef std::vector<ScanType> ScanTypes;

class DemuxTraceHeader
{
	unsigned char trh[DEMUX_TRACE_HEADER_BLOCK];
public:
	DemuxTraceHeader() { memset(&trh[0], 0, DEMUX_TRACE_HEADER_BLOCK); };
	unsigned char* get_buffer() { return &trh[0]; }; // for initialization

	unsigned get_file_number() const ;
	unsigned get_scantype() const  { return BcdToInt(&trh[2], 0, 2); };
	unsigned get_channel_set() const ;
	unsigned get_trace_number() const  { return BcdToInt(&trh[4], 0, 4); };
	double get_first_timing_word() const ;
	unsigned get_trace_header_extensions() const  { return trh[9]; };
	unsigned char get_sample_skew() const  { return trh[10]; };
	unsigned char get_trace_edit() const  { return trh[11]; };
	void RemapField(Prime::IO::TraceHead& h, const RemapItem& ri) const ;
};

class TraceHeaderExtension : public GeneralBlock
{
//public:
//	TraceHeaderExtension() : GeneralBlock() {};
//	bool is_valid() const { return (sHeader[31] == 0x40); };
//
//	int get_receiver_line() const ;
//	int get_receiver_line_fraction() const ;
//	int get_receiver_number() const ;
//	int get_receiver_number_fraction() const ;
//	int get_receiver_index() const  { return BinaryToInt(&sHeader[6], 1); };
//	unsigned char get_reshoot_index() const  { return (unsigned char)sHeader[7]; };
//	unsigned char get_sensor_type() const  { return (unsigned char)sHeader[20]; };
//	unsigned get_exttrace_number() const  { return BinaryToUint(&sHeader[21], 3); };
//	unsigned get_samples() const  { return BinaryToUint(&sHeader[24], 4); };
//
	int version_major, version_minor;
public:
	TraceHeaderExtension(int major_ver, int minor_ver) : GeneralBlock(), version_major(major_ver), version_minor(minor_ver) {};
	bool is_valid() const { return (version_major < 3 || sHeader[31] == 0x40); };

	int get_receiver_line() const ;
	int get_receiver_line_fraction() const ;
	int get_receiver_number() const ;
	int get_receiver_number_fraction() const ;
	int get_receiver_index() const  { return BinaryToInt(&sHeader[6], 1); };
	unsigned char get_reshoot_index() const  { return (unsigned char)sHeader[7]; };
	unsigned char get_sensor_type() const  { return (unsigned char)sHeader[20]; };
	unsigned get_exttrace_number() const  { if(version_major < 3) throw SegD3InconsistentVersion(); return BinaryToUint(&sHeader[21], 3); };
	unsigned get_samples() const  { return (version_major < 3 ? BinaryToUint(&sHeader[7], 3) : BinaryToUint(&sHeader[24], 4)); };

};

class SensorInfoTraceHeaderExtension : public GeneralBlock
{
public:
	SensorInfoTraceHeaderExtension() : GeneralBlock() {};

	bool is_valid() const { return (sHeader[31] == 0x41); }
	Timestamp get_instrument_test_time() const { return *((Timestamp*)&sHeader[0]); };
	float get_sensitivity() const { return BinaryToFloat(&sHeader[8]); };
	unsigned char get_istrument_test_result() const { return sHeader[12]; };
	std::string get_serial_number() const {
		int end_pos = 13;
		for(; end_pos < 31 && sHeader[end_pos] != 0x20; ++end_pos)
			;
		return std::string((const char*)&sHeader[13], end_pos - 13);
	};
	bool is_test_passed() const { return (sHeader[12] == 0x01); };
};

class GeneralTrailerBlock : public GeneralBlock
{
public:
	GeneralTrailerBlock() : GeneralBlock() {};
	bool is_valid() const { return (sHeader[31] == 0x70); };

	unsigned get_trailer_blocks() const  { return BinaryToUint(&sHeader[4], 4); };
};

class TraceData
{
	float *pSamples, descale_multiplier;
	int nSamples;

	void convert_data(const char* data, SegD3FormatCode format) ; // Дописать обработку остальных форматов
public:
	TraceData()	: nSamples(0), pSamples(nullptr), descale_multiplier(1) { };
	void Init(const char* data, int samples, SegD3FormatCode format, float dsm_factor = 1) ;
	~TraceData() { if(pSamples) { free(pSamples); pSamples = nullptr; } };

	const float* get_trace() const { return pSamples; };
	int get_samples_num() const { return nSamples; };
};


struct TChSet {
	int nScanType;
	int nChannelSet;
	const ChannelSet* pChan;

	TChSet() : nScanType(0), nChannelSet(0), pChan(nullptr) {};
	TChSet(int _scan_type, int _chan_set, const ChannelSet* _chset_ptr) : nScanType(_scan_type), nChannelSet(_chan_set), pChan(_chset_ptr) {};
	bool operator<(const TChSet& rhs) const { if(nScanType == rhs.nScanType) return (nChannelSet < rhs.nChannelSet); return (nScanType < rhs.nScanType);};
	bool operator==(const TChSet& rhs) const { return (nScanType == rhs.nScanType && nChannelSet == rhs.nChannelSet); };
};

class SegDRecord
{
	std::string file_path;
	std::map<int, GeneralBlockPtr> GenHeaders;
	GeneralHeader1& general_header1() { return *dynamic_cast<GeneralHeader1*>(GenHeaders[1].get()); };
	GeneralHeader2& general_header2() { return *dynamic_cast<GeneralHeader2*>(GenHeaders[2].get()); };
	GeneralHeader3& general_header3() { return *dynamic_cast<GeneralHeader3*>(GenHeaders[3].get()); };

	int src_block_num;
	SourceInfo& src_info() { return *dynamic_cast<SourceInfo*>(GenHeaders[src_block_num].get());};
	ScanTypes vScanTypes;

	std::vector<GeneralBlockPtr> vExtendedHeaderBlocks;
	std::vector<GeneralBlockPtr> vExternalHeaderBlocks;

	GeneralTrailerBlock trailer;
	char *sTraceBuffer;

	off_t header_offset, data_offset, trailer_offset;
	size_t nRecNumber, nRecordSize;
	std::vector<TChSet> vChSets2Read;
	std::map< std::string, std::vector<TChSet> > channel_map;
	float fDSM;
	unsigned nCurrChSet, nCurrTrace, nCurrNTR; //, nTraceSize;
	unsigned nOutputTraceSamples;
	Prime::IO::TraceHead PrimeHeader;
	TraceData tr;
	bool bInitialized;

	unsigned get_headers_number() const ;
	double get_base_scan_interval() const ;
	double get_record_length() const ;
	unsigned get_channelsets() const ;
	unsigned get_sample_skew_blocks() const ;
	unsigned get_extended_header_blocks() const ;
	unsigned get_external_header_blocks() const ;
public:
	SegDRecord() : sTraceBuffer(NULL), header_offset(0), data_offset(0), trailer_offset(0), src_block_num(-1),
		nRecordSize(0), nRecNumber(0), fDSM(1), nCurrChSet(0), nCurrTrace(0), nCurrNTR(0), nOutputTraceSamples(0), bInitialized(false) {};
	~SegDRecord() { clear(); };
	void clear();

	void Init(const std::string& path, FILE* in, off_t start_offset, off_t file_size) ; // returns total size of the record
	void get_channeltype_list(std::vector<std::string>& chantypes) const ;
	unsigned get_version_major() const { return general_header2().get_version_major(); };
	std::vector<const ChannelSet*> get_channelset(const std::string& chset_type) const;

	void start_traces_iteration(const std::vector<unsigned char> chTypes, unsigned output_samples, bool samples_number_must_be_equal) ;
	bool get_next_trace(FILE *in, Prime::Tree::Trace& t, const std::vector<RemapItem>& remap = std::vector<RemapItem>()) ;
	unsigned get_file_number() const ;
	bool is_source_info_valid() const { return (src_block_num >= 0); };

	const GeneralHeader1& general_header1() const {
		const auto it = GenHeaders.find(1);
		if(it != GenHeaders.end())
			return *dynamic_cast<const GeneralHeader1*>(it->second.get());
		throw SegD3Exception("General header block 1 is not found");
	}
	const GeneralHeader2& general_header2() const {
		const auto it = GenHeaders.find(2);
		if(it != GenHeaders.end())
			return *dynamic_cast<const GeneralHeader2*>(it->second.get());
		throw SegD3Exception("General header block 2 is not found");
	}
	const GeneralHeader3& general_header3() const {
		const auto it = GenHeaders.find(3);
		if(it != GenHeaders.end())
			return *dynamic_cast<const GeneralHeader3*>(it->second.get());
		throw SegD3Exception("General header block 3 is not found");
	}
	const GeneralBlock& general_header(int n) const {
		const auto it = GenHeaders.find(n);
		if(it != GenHeaders.end())
			return *dynamic_cast<const GeneralBlock*>(it->second.get());
		throw SegD3Exception("General header block %d is not found", n);
	}
	const SourceInfo& source_info() const {
		const auto it = GenHeaders.find(src_block_num);
		if(it != GenHeaders.end())
			return *dynamic_cast<SourceInfo*>(it->second.get());
		throw SegD3Exception("Source info general header block %d is not found", src_block_num);
	}
	const ScanTypes& scan_types() const { return vScanTypes; };
	bool is_initialized() const { return bInitialized; };
};

class SegDFile
{
	mutable FILE *in;
	mutable char* pIOBuf;

	std::string sBasePath, sRelName;
	StorageUnitLabel Label;
	//std::vector<SegDRecord> vRecords;
	//std::vector<SegDRecord>::iterator itCurrRecord;
	SegDRecord m_record;
	SegD3FileType file_type;
	std::vector<unsigned char> chTypes;
	off_t filelength;

public:
	SegDFile(off_t length = 0) : in(NULL), pIOBuf(nullptr), file_type(SEGD3_FTYPE_UNKNOWN), filelength(length) {};
	SegDFile(const std::string& base_path, const std::string& rel_name, off_t length = 0) : sBasePath(base_path), sRelName(rel_name), in(NULL), pIOBuf(nullptr), file_type(SEGD3_FTYPE_UNKNOWN), filelength(length) {};
	SegDFile(const SegDFile& f);
	~SegDFile();
	void clear();

	SegDFile& operator=(const SegDFile& f);

	void Init(const std::string& fullpath);
	void Init(int fd_dir, const std::string& base_dir_path, const std::string& rel_path);

	void Open(int fd_dir);
	void Close();

	//size_t get_records_num() const { return vRecords.size(); };
	SegD3FileType get_type() const { return file_type; };
	unsigned get_version_major() const { return m_record.get_version_major(); };
	void get_channeltype_list(std::vector<std::string>& chantypes) const;
	void get_channels_info(std::vector<SegDInfo>& chan_info) const;

	const SegDRecord& record() const { return m_record; };
	std::string path() const { return sBasePath + sRelName; };
	const std::string& base_path() const { return sBasePath; }; // file directory path with trailing slash
	const std::string& filename() const { return sRelName; };
	off_t get_file_length() const { return filelength; };

	bool operator<(const SegDFile& f) const;

	void start_traces_iteration(const std::vector<unsigned char>& chantypes, unsigned output_samples, bool samples_number_must_be_equal, int fd_parent = -1);
	bool get_next_trace(Prime::Tree::Trace& t, const std::vector<RemapItem>& remap = std::vector<RemapItem>());
};

class SegDSource
{
	std::vector<SegDFile> Files;
	std::vector<SegDFile>::iterator itCurrFile;
	std::vector<unsigned char> chTypes;
	std::string sPath;
	bool bInitialized;
	bool bMPIWorker;
	std::vector<std::string> vChannelTypes;
	size_t nRecords;
	std::vector<SegDInfo> vSegDInfo;
	std::map<std::string, DIR*> mapDir;
	int nFFID_start, nFFID_end;

	void process_folder(std::vector<SegDFile>& files, const char* path, const char* entry, bool process_dot_files, int parent_path_fd = -1);

public:
	SegDSource() : bInitialized(false), bMPIWorker(false), nRecords(0), nFFID_start(0), nFFID_end(INT_MAX) {};
	~SegDSource() {};

	void clear();
	size_t get_records_num() const { return nRecords; };
	size_t get_files_num() const { return Files.size(); };
	unsigned get_record_ffid(size_t record_num) const { return Files[record_num].record().get_file_number(); }
	const std::vector<std::string>& get_channeltype_list() const { return vChannelTypes; };
	void get_channels_info(const std::vector<std::string>& vChanTypes, unsigned *sampling_rate_mks, unsigned *trace_samples, size_t *approx_traces) const;
	void get_all_channels_info(std::map<std::string, std::string>& mapChanInfo);

	// single node interface
	void reset(const char* path, int ffid_start = 0, int ffid_end = INT_MAX); // path is directory or single file
	void prepare_files(const char* path, const std::vector<std::string>& channel_types);
	void start_file_iteration(const std::vector<std::string>& vChanTypes, int ffid_start = 0, int ffid_end = INT_MAX);
	bool get_next_file(SegDFile& f, unsigned output_samples, bool samples_number_must_be_equal);

	// mpi worker interface
	void reset_worker(const char* path, const std::vector<std::string>& channel_types, int ffid_start = 0, int ffid_end = INT_MAX);
	bool dispatch_next_file(std::string* full_file_path);
	bool next_file(SegDFile& f, const std::string& path, const std::vector<std::string>& vChanTypes, unsigned output_samples, bool samples_number_must_be_equal);
};




} // namespace SEGD3
} // namespace Prime

#endif // _PRIME_LIB_SEGD3_H_
