#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "opsyserv/BufferedIO.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <iterator>
#include <algorithm>
#include "segd3.h"
#include <iostream>
#include <iomanip>
#include <assert.h>

namespace Prime {
namespace SEGD3 {




const size_t ChannelTypes2Num = 11;
const size_t ChannelTypes3Num = 36;
ChannelTypeDescrT chan_descr2[ChannelTypes2Num] = {
	{ 0x0, "Unused" },
	{ 0x01, "Seis" },
	{ 0x02, "Time break" },
	{ 0x03, "Up hole" },
	{ 0x04, "Water break" },
	{ 0x05, "Time counter" },
	{ 0x06, "External Data" },
	{ 0x07, "Other" },
	{ 0x08, "Signature/unfiltered" },
	{ 0x09, "Signature/filtered" },
	{ 0x0C, "Auxiliary Data Trailer" }
};

ChannelTypeDescrT chan_descr3[ChannelTypes3Num] = {
	{ 0x10, "Seis" },
	{ 0x11, "Electromagnetic (EM)" },
	{ 0x20, "Time break" },
	{ 0x21, "Clock timebreak" },
	{ 0x22, "Field timebreak" },
	{ 0x30, "Up hole" },
	{ 0x40, "Water break" },
	{ 0x50, "Time counter" },
	{ 0x60, "External Data" },
	{ 0x61, "Acoustic range measurement" },
	{ 0x62, "Acoustic reference measured" },
	{ 0x63, "Acoustic reference nominal" },
	{ 0x70, "Other" },
	{ 0x80, "Signature/unfiltered" },
	{ 0x90, "Signature/filtered" },
	{ 0x91, "Source signature/unfiltered" },
	{ 0x92, "Source signature/filtered" },
	{ 0x93, "Source signature/estimated" },
	{ 0x94, "Source signature/measured" },
	{ 0x95, "Source base plate" },
	{ 0x96, "Source reference sweep" },
	{ 0x97, "Source other" },
	{ 0xA0, "Auxiliary Data Trailer" },
	{ 0xB0, "True reference sweep" },
	{ 0xB1, "Radio reference sweep" },
	{ 0xB2, "Radio similarity signal" },
	{ 0xB3, "Wireline reference sweep" },
	{ 0xC0, "Depth" },
	{ 0xC1, "Wind" },
	{ 0xC2, "Current" },
	{ 0xC3, "Voltage" },
	{ 0xC4, "Velocity" },
	{ 0xC5, "Acceleration" },
	{ 0xC6, "Pressure" },
	{ 0xC7, "Tension" },
	{ 0xF0, "Calibration trace" }
};
const int SegD3FmtNum = 10;
const int segd3_formats[SegD3FmtNum] = {8015, 8022, 8024, 8036, 8038, 8042, 8044, 8048, 8058, 8080};
const size_t segd3_format_size[SegD3FmtNum] = {5, 1, 2, 3, 4, 1, 2, 4, 4, 8};

static const char* SegDFieldFormatName[] = {"BCD", "Binary", "String", "Timestamp", 0};

const char* getFormatName(SegDFormat_t fmt)
{
	return SegDFieldFormatName[int(fmt)];
}

SegDFormat_t getFormatFromName(const char* fmt_name)
{
	for(int i = 0; SegDFieldFormatName[i]; ++i)
		if(!strcmp(fmt_name, SegDFieldFormatName[i])) return SegDFormat_t(i);
	throw std::range_error("SegD field format name is invalid");
}


const unsigned SegDConst_FF1 = 0x0FF;
const unsigned SegDConst_FF2 = 0x0FFFF;
const unsigned SegDConst_FF3 = 0x0FFFFFF;

ChannelTypeDescrT* get_channel_types(int version, int *pTNum)
{
	if(version == 3) *pTNum = ChannelTypes3Num;
	else *pTNum = ChannelTypes2Num;
	return (version == 3 ? &chan_descr3[0] : &chan_descr2[0]);
}

const char* get_channel_type_desc(int version, unsigned char t)
{
	int ndesc = 0;
	ChannelTypeDescrT* chan_descr = get_channel_types(version, &ndesc);
	ChannelTypeDescrT cdt(t, "");
	ChannelTypeDescrT *itChDescr = std::lower_bound(&chan_descr[0], &chan_descr[ndesc], cdt);
	if(itChDescr == &chan_descr[ndesc] || itChDescr->n != cdt.n) return nullptr;
	return itChDescr->descr;
}


std::string DumpInterface::dump_raw(int bytes_in_row, int group_size, int selected_field) const
{
	std::ostringstream oss;
	oss << std::hex << std::uppercase;
	int ndesc = 0;
	const FieldsDescription* d = get_block_descr(&ndesc);
	int sel_byte1, sel_tetrad1, sel_byte2, sel_tetrad2;
	if(selected_field >= 0 && selected_field < ndesc) {
		d = d + selected_field;
		sel_byte1 = d->offset_halfbyte / 2;
		sel_tetrad1 = d->offset_halfbyte & 0x1;

		sel_byte2 = (d->offset_halfbyte + d->length_halfbyte) / 2;
		sel_tetrad2 = (d->offset_halfbyte + d->length_halfbyte) & 0x1;
	}
	else
		d = nullptr;

	for(size_t i = 0; i < length; ++i) {

		if(d) { // selection
			if(i == sel_byte1) {
				if(sel_tetrad1) oss << "#" << high_tetrad(content[i]) << "#" << low_tetrad(content[i]) << "#";
				else oss << "#" << std::setw(2) << std::setfill('0') << (int)content[i];
			}
			else if(i == sel_byte2) {
				if(sel_tetrad2) oss << std::setw(2) << std::setfill('0') << int(content[i]) << "#";
				else oss << "#" << high_tetrad(content[i]) << "#" << low_tetrad(content[i]) << "#";
			}
			else oss << std::setw(2) << std::setfill('0') << int(content[i]);
		}
		else oss << std::setw(2) << std::setfill('0') << int(content[i]);
		if((i+1) % bytes_in_row) {
			oss << " ";
			if((i+1) % group_size == 0)
				oss << " ";
		}
		else
			oss << std::endl;
	}
	return oss.str();
}

//void RemapField(Prime::IO::TraceHead& h, const RemapItem& ri, const unsigned char* sData, size_t data_length)
//{
//	assert(ri.offset + ri.length <= data_length * (ri.format == SEGD_FORMAT_BCD ? 2 : 1));
//	if(!ri.prime_field.IsOriginal()) throw SegD3Exception("Prime field must be one of standard headers");
//	int val = 0;
//	try {
//		switch(ri.format) {
//		case SEGD_FORMAT_BCD:
//			val = BcdToInt(&sData[ri.offset/2], ri.offset % 2, ri.length);
//			if(ri.prime_field.IsFloat())
//				h.setValue(ri.prime_field, float(val));
//			else
//				h.Set(ri.prime_field, val);
//			break;
//		case SEGD_FORMAT_BINARY:
//			if(ri.prime_field.IsFloat())
//				h.setValue(ri.prime_field, BinaryToFloat(&sData[ri.offset / 2]));
//			else
//				h.Set(ri.prime_field, BinaryToInt(&sData[ri.offset / 2], ri.length));
//			break;
//		case SEGD_FORMAT_TIMESTAMP:
//			throw SegD3Exception("Timestamp format is not implemented yet for header fields remap operation");
//			break;
//		case SEGD_FORMAT_STRING: {
//			std::string s((const char*)&sData[ri.offset / 2], ri.length / 2);
//			errno = 0;
//			if(ri.prime_field.IsFloat()) {
//				float val = strtof(s.c_str(), nullptr);
//				if(errno == 0)
//					h.Set(ri.prime_field, val);
//				else throw SegD3Exception(errno, "Convert string to float value error");
//			}
//			else {
//				int val = strtol(s.c_str(), nullptr, 10);
//				if(errno == 0)
//					h.Set(ri.prime_field, val);
//				else throw SegD3Exception(errno, "Convert string to int value error");
//			}
//		}
//			break;
//		default: break;
//		}
//	}
//	catch(const std::exception& e) {
//		std::ostringstream oss;
//		oss << "Remap " << ri.segd_header_type << "(num=" << ri.block_num_or_id << "), offset=" << ri.offset << "(tetrades), length=" << ri.length << "(tetrades) " << getFormatName(ri.format) << " value to " << ri.prime_field.GetDescr() << " error: " << e.what();
//		throw SegD3Exception(oss.str().c_str());
//	}
//}

void RemapField(Prime::IO::TraceHead& h, const RemapItem& ri, const unsigned char* sData, size_t data_length)
{
	assert(ri.start_bit / 8 < data_length && ri.end_bit / 8 < data_length);
	if(!ri.prime_field.IsOriginal()) throw SegD3Exception("Prime field must be one of standard headers");
	int val = 0;
	try {
		switch(ri.format) {
		case SEGD_FORMAT_BCD:
			val = BcdToInt(&sData[ri.start_bit/8], (ri.start_bit % 8 ? 1 : 0), (ri.end_bit - ri.start_bit + 1) / 4);
//			val = BcdToInt(&sData[ri.offset/2], ri.offset % 2, ri.length);
			if(ri.prime_field.IsFloat())
				h.setValue(ri.prime_field, float(val));
			else
				h.Set(ri.prime_field, val);
			break;
		case SEGD_FORMAT_BINARY:
			if(ri.prime_field.IsFloat())
				h.setValue(ri.prime_field, BinaryToFloat(&sData[ri.start_bit / 8]));
			else {
				int bytes = (ri.end_bit + 1) / 8 - ri.start_bit / 8;
				if(ri.start_bit % 8 == 0 /*&& (ri.end_bit + 1) % 8 == 0 it must be always true*/)
					h.Set(ri.prime_field, BinaryToInt(&sData[ri.start_bit / 8], bytes));
				else {
					const unsigned char *f = &sData[ri.start_bit / 8];
					unsigned char mask = ((unsigned char)1 << ri.start_bit % 8) - 1;
					int res = 0;
					unsigned char *g = (unsigned char *) &res;
					int i, j;
					for(i = 0, j = bytes - 1; i < bytes; i++, j--)
						g[i] = (j ? f[j] : f[j] & mask);
					h.Set(ri.prime_field, res);
				}
			}
			break;
		case SEGD_FORMAT_TIMESTAMP:
			throw SegD3Exception("Timestamp format is not implemented yet for header fields remap operation");
			break;
		case SEGD_FORMAT_STRING: {
			std::string s((const char*)&sData[ri.start_bit / 8], (ri.end_bit - ri.start_bit + 1) / 8);
			errno = 0;
			if(ri.prime_field.IsFloat()) {
				float val = strtof(s.c_str(), nullptr);
				if(errno == 0)
					h.Set(ri.prime_field, val);
				else throw SegD3Exception(errno, "Convert string to float value error");
			}
			else {
				int val = strtol(s.c_str(), nullptr, 10);
				if(errno == 0)
					h.Set(ri.prime_field, val);
				else throw SegD3Exception(errno, "Convert string to int value error");
			}
		}
			break;
		default: break;
		}
	}
	catch(const std::exception& e) {
		std::ostringstream oss;
		oss << "Remap " << ri.segd_header_type << "(num=" << ri.block_num_or_id << "), start_bit=" << ri.start_bit << ", end_bit=" << ri.end_bit << " " << getFormatName(ri.format) << " value to " << ri.prime_field.GetDescr() << " error: " << e.what();
		throw SegD3Exception(oss.str().c_str());
	}
}

void GeneralBlock::RemapField(Prime::IO::TraceHead& h, const RemapItem& ri) const
{
	Prime::SEGD3::RemapField(h, ri, &sHeader[0], SEGD_HEADER_BLOCK);
}

void ChannelSet::RemapField(Prime::IO::TraceHead& h, const RemapItem& ri)
{
	Prime::SEGD3::RemapField(h, ri, &sData[0], (version_major == 3 ? SEGD_CHANNELSET_BLOCK3 : SEGD_CHANNELSET_BLOCK2));
}

void DemuxTraceHeader::RemapField(Prime::IO::TraceHead& h, const RemapItem& ri) const
{
	Prime::SEGD3::RemapField(h, ri, &trh[0], DEMUX_TRACE_HEADER_BLOCK);
}

void GeneralHeader1::test() const
{
	std::cout << "sHeader=" << (void*)sHeader << ", content=" << (void*)content << std::endl;
}

std::string DumpInterface::dump() const
{
	std::ostringstream oss;
	int ndesc = 0;
	const FieldsDescription* d = get_block_descr(&ndesc);
	for(int i = 0; i < ndesc; ++i) {
		oss << d[i].field_name << ": ";
		try {
			switch(d[i].valtype) {
			case BCD_VALUE:
				try {
					oss << BcdToInt(content + d[i].offset_halfbyte / 2, d[i].offset_halfbyte & 0x1, d[i].length_halfbyte);
				}
				catch(const SegD3ExtendedValue& e) {
					oss << "Value in another place";
				}
				break;
			case BYTE_VALUE: {
				std::ios_base::fmtflags oldf = oss.setf(std::ios_base::hex);

				if(d[i].length_halfbyte < 2) {
					if(d[i].offset_halfbyte & 0x1)
						oss << ((content[d[i].offset_halfbyte / 2] >> 4) & 0x0F);
					else
						oss << ((content[d[i].offset_halfbyte / 2]) & 0x0F);
				}
				else oss << int(content[d[i].offset_halfbyte / 2]);
				oss.setf(oldf);
			}
				break;
			case INT_VALUE:
				oss << BinaryToInt(content + d[i].offset_halfbyte / 2, d[i].length_halfbyte / 2);
				break;
			case UINT_VALUE:
				if(d[i].length_halfbyte > 8)
					oss << BinaryToUint64(content + d[i].offset_halfbyte / 2, d[i].length_halfbyte / 2);
				else
					oss << BinaryToUint(content + d[i].offset_halfbyte / 2, d[i].length_halfbyte / 2);
				break;
			case FLOAT_VALUE:
				if(d[i].length_halfbyte * 2 != sizeof(float)) throw SegD3IncorrectFormat();
				oss << BinaryToFloat(content + d[i].offset_halfbyte / 2);
				break;
			case DOUBLE_VALUE:
				if(d[i].length_halfbyte * 2 != sizeof(double)) throw SegD3IncorrectFormat();
				oss << BinaryToFloat(content + d[i].offset_halfbyte / 2);
				break;
			case TIMESTAMP_VALUE:
				oss << timestampToUtc(BinaryToTimestamp(content + d[i].offset_halfbyte / 2)).toString(false);
				break;
			default:
				oss << "Type error";
				break;
			}
		}
		catch(const SegD3Exception& e) {
			oss << e.what();
		}
		oss << std::endl;
	}
	return oss.str();
}


FieldsDescription GeneralHeader1::block_descr[] = {
	{"File number", BCD_VALUE, 0, 4},
	{"Format code", BCD_VALUE, 4, 4},
	{"General constants", BCD_VALUE, 8, 12},
	{"Year", BCD_VALUE, 20, 2},
	{"Additional general header blocks", BCD_VALUE, 22, 1},
	{"Day", BCD_VALUE, 23, 3},
	{"Hour", BCD_VALUE, 26, 2},
	{"Minute", BCD_VALUE, 28, 2},
	{"Second", BCD_VALUE, 30, 2},
	{"Manufacture's code", BCD_VALUE, 32, 6},
	{"Base scan interval", BYTE_VALUE, 44, 2},
	{"Polarity", BYTE_VALUE, 46, 1},
	{"Record type", BYTE_VALUE, 50, 1},
	{"Record length", BCD_VALUE, 51, 3},
	{"Scan types per record", BCD_VALUE, 54, 2},
	{"Channel sets per scan type", BCD_VALUE, 56, 2},
	{"Skew blocks", BCD_VALUE, 58, 2},
	{"Extended header blocks", BCD_VALUE, 60, 2},
	{"External header blocks", BCD_VALUE, 62, 2},
	{nullptr}
};

FieldsDescription GeneralHeader2::block_descr2[] = {
	{"Extended file number", UINT_VALUE, 0, 6},
	{"Extended channel sets per scan type", UINT_VALUE, 6, 4},
	{"Extended header blocks", UINT_VALUE, 10, 4},
	{"External header blocks", UINT_VALUE, 14, 4},
	{"Revision major", BYTE_VALUE, 20, 2},
	{"Revision minor", BYTE_VALUE, 22, 2},
	{"General trailer blocks", UINT_VALUE, 24, 4},
	{"Extended record length", UINT_VALUE, 28, 6},
	{"General header block number", BYTE_VALUE, 36, 2},
	{"Sequence number", UINT_VALUE, 40, 4},
	{nullptr}
};
FieldsDescription GeneralHeader2::block_descr3[] = {
	{"Extended file number", UINT_VALUE, 0, 6},
	{"Extended channel sets per scan type", UINT_VALUE, 6, 4},
	{"Extended header blocks", UINT_VALUE, 10, 6},
	{"Extended skew blocks", UINT_VALUE, 16, 4},
	{"Revision major", BYTE_VALUE, 20, 2},
	{"Revision minor", BYTE_VALUE, 22, 2},
	{"General trailer blocks", UINT_VALUE, 24, 8},
	{"Extended record length", UINT_VALUE, 32, 8},
	{"Record set number", UINT_VALUE, 40, 4},
	{"Extended number of additional blocks in general header", UINT_VALUE, 44, 4},
	{"Dominant sampling interval", UINT_VALUE, 48, 6},
	{"External header blocks", UINT_VALUE, 54, 6},
	{"General header block number", BYTE_VALUE, 62, 2},
	{nullptr}
};

FieldsDescription GeneralHeader3::block_descr[] = {
	{"Time Zero", TIMESTAMP_VALUE, 0, 16},
	{"Record size", UINT_VALUE, 16, 16},
	{"Data size", UINT_VALUE, 32, 16},
	{"Header size", UINT_VALUE, 48, 8},
	{"Extended recording mode", BYTE_VALUE, 56, 2},
	{"Relative time mode", BYTE_VALUE, 58, 2},
	{"General header block number", BYTE_VALUE, 62, 2},
	{nullptr}
};

FieldsDescription SourceInfo::block_descr[] = {
	{"Expanded file number", UINT_VALUE, 0, 6},
	{"Source line number", INT_VALUE, 6, 6},
	{"Source line number, fraction", UINT_VALUE, 12, 4},
	{"Source point number", INT_VALUE, 16, 6},
	{"Source point number, fraction", UINT_VALUE, 22, 4},
	{"Source point index", BYTE_VALUE, 26, 2},
	{"Source type (only for ver >= 3.0)", BYTE_VALUE, 62, 2},
	{"Depth", UINT_VALUE, 28, 4},
	{nullptr}
};

void SegDSource::clear()
{
	Files.clear();
	itCurrFile = Files.end();
	for(auto& it : mapDir)
		if(it.second) { closedir(it.second); it.second = nullptr; }
	mapDir.clear();
	sPath.clear();
	bInitialized = false;
	bMPIWorker = false;
	vChannelTypes.clear();
	chTypes.clear();
	nRecords = 0;
}

void SegDSource::process_folder(std::vector<SegDFile>& files, const char *parent_path, const char* rel_path, bool process_dot_files, int parent_path_fd)
{
	DIR *dir = NULL;
	dirent *entry = NULL;
	struct stat st_entry;
	std::string sDirPath;
	if(parent_path) {
		sDirPath = parent_path;
		if(*sDirPath.rbegin() != '/') sDirPath += "/";
	}
	sDirPath += rel_path;
	if(*sDirPath.rbegin() != '/') sDirPath += "/";

	if(mapDir.find(sDirPath) == mapDir.end()) {
		if(parent_path_fd == -1) {
			dir = opendir(sDirPath.c_str());
		}
		else {
			int d = openat(parent_path_fd, rel_path, O_RDONLY | O_DIRECTORY, 0);
			dir = fdopendir(d);
		}
		if(!dir) throw SegD3Exception(errno, "Fail to open directory %s", sDirPath.c_str());
		mapDir.insert(std::pair<std::string, DIR*>(sDirPath, dir));
	}
	else dir = mapDir[sDirPath];
	parent_path_fd = dirfd(dir);

	errno = 0;
	while((entry = readdir(dir))) {
		if(entry->d_name[0] == '.' && entry->d_name[1] == '.') continue;
		if(entry->d_name[0] == '.' && (!process_dot_files || entry->d_name[1] == 0)) continue;

		std::string sEntryPath = sDirPath; sEntryPath += entry->d_name;
		if( fstatat(parent_path_fd, entry->d_name, &st_entry, 0) != 0 )
			throw SegD3Exception(errno, "Fail to stat file %s%s", sDirPath.c_str(), entry->d_name);
		if(S_ISDIR(st_entry.st_mode)) {
			process_folder(files, sDirPath.c_str(), entry->d_name, process_dot_files, parent_path_fd);
			continue;
		}
		if(!S_ISREG(st_entry.st_mode) && !S_ISLNK(st_entry.st_mode)) continue;

		SegDFile f(sDirPath, entry->d_name, st_entry.st_size);
		files.push_back(f);
		errno = 0;
	}
}

void SegDSource::reset(const char* path, int ffid_start, int ffid_end)
{
	sPath = path;
	if(*sPath.rbegin() == '/') sPath.erase(sPath.length() - 1); // removing trailing slash if exists

	struct stat st_entry;
	bool bProcessDotFiles = (getenv("PROCESS_DOT_FILES") != NULL);
	std::vector<SegDFile> vFiles;
	bMPIWorker = false;

	try {
		if(stat(sPath.c_str(), &st_entry) == -1)
			throw SegD3Exception(errno, "Fail to stat path %s", sPath.c_str());
		if(S_ISDIR(st_entry.st_mode)) {
			process_folder(vFiles, nullptr, sPath.c_str(), bProcessDotFiles);
		}
		else {
			std::string sBasePath, sRelName;
			auto slash_pos = sPath.rfind('/');
			if(slash_pos != std::string::npos) {
				sBasePath = sPath.substr(0, slash_pos + 1);
				sRelName = sPath.substr(slash_pos + 1);
			}
			else {
				sBasePath.clear();
				sRelName = sPath;
			}
			SegDFile f(sBasePath, sRelName, st_entry.st_size);
			vFiles.push_back(f);
			errno = 0;
		}

		Files.reserve(vFiles.size());
		for(auto& f: vFiles) {
			auto it = mapDir.find(f.base_path());
			if(it != mapDir.end() && it->second)
				f.Init(dirfd(it->second), f.base_path().c_str(), f.filename().c_str());
			else
				f.Init(f.path().c_str());
			if(f.get_type() == SEGD3_FTYPE_RECORDS) {
				nRecords++;
				f.get_channeltype_list(vChannelTypes);
				int fnum = f.record().get_file_number();
				if(fnum >= ffid_start && fnum <= ffid_end) {
					f.get_channels_info(vSegDInfo);
					Files.push_back(f);
				}
			}
		}
	}
	catch(const std::exception& e) {
		throw SegD3Exception("SegDSource::reset(%s) error: %s", path, e.what());
	}
	if(Files.size() > 1) std::sort(Files.begin(), Files.end());
	itCurrFile = Files.end();
	bInitialized = true;
}

void SegDSource::prepare_files(const char* path, const std::vector<std::string>& channel_types)
{
	sPath = path;
	if(*sPath.rbegin() == '/') sPath.erase(sPath.length() - 1); // removing trailing slash if exists

	struct stat st_entry;
	bool bProcessDotFiles = (getenv("PROCESS_DOT_FILES") != NULL);
	std::vector<SegDFile> vFiles;
	bMPIWorker = false;

	try {
		if(stat(sPath.c_str(), &st_entry) == -1)
			throw SegD3Exception(errno, "Fail to stat path %s", sPath.c_str());
		if(S_ISDIR(st_entry.st_mode)) {
			process_folder(vFiles, nullptr, sPath.c_str(), bProcessDotFiles);
		}
		else {
			std::string sBasePath, sRelName;
			auto slash_pos = sPath.rfind('/');
			if(slash_pos != std::string::npos) {
				sBasePath = sPath.substr(0, slash_pos + 1);
				sRelName = sPath.substr(slash_pos + 1);
			}
			else {
				sBasePath.clear();
				sRelName = sPath;
			}
			SegDFile f(sBasePath, sRelName, st_entry.st_size);
			vFiles.push_back(f);
			errno = 0;
		}
		Files.swap(vFiles);
	}
	catch(const std::exception& e) {
		throw SegD3Exception("SegDSource::reset(%s) error: %s", path, e.what());
	}
	if(Files.size() > 1) std::sort(Files.begin(), Files.end());
	itCurrFile = Files.end();
}

void SegDSource::reset_worker(const char* path, const std::vector<std::string>& channel_types, int ffid_start, int ffid_end)
{
	sPath = path;
	if(*sPath.rbegin() == '/') sPath.erase(sPath.length() - 1); // removing trailing slash if exists
	nFFID_start = ffid_start;
	nFFID_end = ffid_end;
	vChannelTypes = channel_types;
	bMPIWorker = true;
}

void SegDSource::get_channels_info(const std::vector<std::string>& vChanTypes, unsigned *sampling_rate_mks, unsigned *trace_samples, size_t *approx_traces) const
{
	SegDFile f;
	*approx_traces = 0;
	if(!bInitialized) {
		if(Files.empty()) throw SegD3Exception("SegDSource::get_channels_info() error: called before files initialization or no files detected in specified path");
	}
	for(auto itFile = Files.begin(); itFile != Files.end(); ++itFile) {
		if(itFile->get_type() == SEGD3_FTYPE_UNKNOWN) {
			f.Init(itFile->path().c_str());
			if(f.get_type() != SEGD3_FTYPE_RECORDS) { f.clear(); continue; }
			else break;
		}
		else if(itFile->get_type() == SEGD3_FTYPE_RECORDS) {
			f.Init(itFile->path().c_str());
			break;
		}
	}
	std::vector<unsigned char> chan_types;
	unsigned smp_rate = 0, samples = 0;
	bool bInfoSet = false;
	for(auto ct : vChanTypes) {
		try {
			std::vector<const ChannelSet*> vChSet = f.record().get_channelset(ct);
			for(auto chi : vChSet) {
				chan_types.push_back(chi->get_type());
				if(!bInfoSet) {
					smp_rate = chi->sampling_interval_mks();
					samples = chi->trace_samples();
					bInfoSet = true;
				}
				else {
					if(smp_rate != chi->sampling_interval_mks())
						throw SegD3Exception("Different sampling intervals in specified channel types.");
					if(samples != chi->trace_samples() && trace_samples && *trace_samples == 0)
						throw SegD3Exception("Different samples number in specified channel types.");
				}
			}
		}
		catch(const SegD3Exception& e) { // Not found
			continue;
		}
	}

	f.start_traces_iteration(chan_types, 0, false, -1);
	Prime::Tree::Trace t;
	size_t file_traces = 0;
	for(; f.get_next_trace(t); ) ++file_traces;
	for(const auto& ff: Files) {
		*approx_traces += file_traces * size_t(ceil(double(ff.get_file_length()) / double(f.get_file_length())));
	}
	*approx_traces += *approx_traces / 10;

	if(!bInfoSet) throw SegD3Exception("No one of specified channels found.");
	if(sampling_rate_mks) *sampling_rate_mks = smp_rate;
	if(trace_samples) *trace_samples = samples;
}

void SegDSource::get_all_channels_info(std::map<std::string, std::string>& mapChanInfo)
{
	if(!bInitialized) throw SegD3Exception("SegDSource::get_all_channels_info() error: called before files initialization with reset()");
	std::vector<SegDInfo> vAll(vSegDInfo);
	std::sort(vAll.begin(), vAll.end(), Fn_SegDInfo_ChanInfo_Cmp());
	auto i = vAll.begin();
	std::vector<SegDInfo> vChan;
	vChan.push_back(*i++);
	auto itCurrCh = vChan.begin();
	Fn_SegDInfo_ChanInfo_Cmp cmp_wo_trs(false);
	for(; i != vAll.end(); ++i) {
		if(cmp_wo_trs(*itCurrCh, *i))
			itCurrCh = vChan.insert(vChan.end(), *i);
		else
			itCurrCh->nChannelTraces += i->nChannelTraces;
	}
	mapChanInfo.clear();
	i = vChan.begin();
	std::ostringstream oss;
	oss << "dt=" << i->nSamplingIntMks << "mks, samples=" << i->nTraceSamples << ", traces=" << i->nChannelTraces;
	std::string sCurrChType = i->sChannelType;
	mapChanInfo.insert(std::pair<std::string, std::string>(sCurrChType, oss.str()));
	for(++i; i != vChan.end(); ++i) {
		if(sCurrChType != i->sChannelType) {
			std::ostringstream oss;
			oss << "dt=" << i->nSamplingIntMks << "mks, samples=" << i->nTraceSamples << ", traces=" << i->nChannelTraces;
			sCurrChType = i->sChannelType;
			mapChanInfo.insert(std::pair<std::string, std::string>(sCurrChType, oss.str()));
		}
		else {
			std::ostringstream oss;
			oss << std::endl << "dt=" << i->nSamplingIntMks << "mks, samples=" << i->nTraceSamples << ", traces=" << i->nChannelTraces;
			mapChanInfo[sCurrChType] += oss.str();
		}
	}
}

void SegDSource::start_file_iteration(const std::vector<std::string>& vChanTypes, int ffid_start, int ffid_end)
{
	if(bMPIWorker)
		throw SegD3Exception("SegDSource::start_file_iteration call in MPI worker mode");

	if(Files.empty())
		throw SegD3Exception("SegDSource::start_file_iteration() error: source files list empty");
	nFFID_start = ffid_start;
	nFFID_end = ffid_end;

	std::cout << "ReadSEGDPlus: list of input files (" << Files.size() << "):";
	for(const auto& f: Files) std::cout << " " << f.path(); std::cout << std::endl;

	itCurrFile = Files.begin();
	for(; itCurrFile != Files.end() && itCurrFile->get_type() != SEGD3_FTYPE_RECORDS; ++itCurrFile) {
		if(itCurrFile->get_type() == SEGD3_FTYPE_UNKNOWN) {
			auto it = mapDir.find(itCurrFile->base_path());
			itCurrFile->Open(it != mapDir.end() && it->second ? dirfd(it->second) : -1);
			if(itCurrFile->get_type() == SEGD3_FTYPE_RECORDS && itCurrFile->record().get_file_number() >= nFFID_start && itCurrFile->record().get_file_number() <= nFFID_end) break;
			else itCurrFile->Close();
		}
	}
	if(itCurrFile == Files.end())
		throw SegD3Exception("There are no files with FFID in range [%d, %d]", nFFID_start, nFFID_end);
	int ndesc = 0;
	ChannelTypeDescrT* chan_descr = get_channel_types(itCurrFile->get_version_major(), &ndesc);
	for(auto ct : vChanTypes) {
		int i = 0;
		for(; i < ndesc; ++i) {
			if(ct == chan_descr[i].descr) break;
		}
		if(i < ndesc) chTypes.push_back(chan_descr[i].n);
		else throw SegD3Exception("Channel type %s is incorrect. SegD version is %u.%u", ct.c_str(), itCurrFile->get_version_major(), itCurrFile->record().general_header2().get_version_minor());
	}
}

bool SegDSource::get_next_file(SegDFile& f, unsigned output_samples, bool samples_number_must_be_equal)
{
	if(bMPIWorker)
		throw SegD3Exception("SegDSource::get_next_file call in MPI worker mode");

	if(itCurrFile != Files.end()) {
		f = *itCurrFile;
		auto it = mapDir.find(f.base_path());
		f.start_traces_iteration(chTypes, output_samples, samples_number_must_be_equal, it == mapDir.end() ? -1 : dirfd(it->second));
		itCurrFile->Close();
		for(++itCurrFile; itCurrFile != Files.end() && itCurrFile->get_type() != SEGD3_FTYPE_RECORDS; ++itCurrFile) {
			if(itCurrFile->get_type() == SEGD3_FTYPE_UNKNOWN) {
				auto it = mapDir.find(itCurrFile->base_path());
				itCurrFile->Open(it != mapDir.end() && it->second ? dirfd(it->second) : -1);
				if(itCurrFile->get_type() == SEGD3_FTYPE_RECORDS && itCurrFile->record().get_file_number() >= nFFID_start && itCurrFile->record().get_file_number() <= nFFID_end) break;
				else itCurrFile->Close();
			}
		}
		return true;
	}
	else
		f.clear();
	return false;
}

bool SegDSource::dispatch_next_file(std::string* full_file_path)
{
	if(bMPIWorker)
		throw SegD3Exception("SegDSource::dispatch_next_file call in MPI worker mode");

	if(itCurrFile != Files.end()) {
		*full_file_path = itCurrFile->path();
		++itCurrFile;
		return true;
	}
	return false;
}

bool SegDSource::next_file(SegDFile& f, const std::string& full_file_path, const std::vector<std::string>& vChanTypes, unsigned output_samples, bool samples_number_must_be_equal)
{
	if(!bMPIWorker)
		throw SegD3Exception("SegDSource::next_file call not in MPI worker mode");
	f.clear();
	f.Init(full_file_path.c_str());
	if(f.get_type() != SEGD3_FTYPE_RECORDS || f.record().get_file_number() < nFFID_start || f.record().get_file_number() > nFFID_end)
		return false;
	int i = 0, ndesc = 0;
	ChannelTypeDescrT* chan_descr = get_channel_types(f.get_version_major(), &ndesc);
	for(auto ct : vChanTypes) {
		for(; i < ndesc; ++i) {
			if(ct == chan_descr[i].descr) break;
		}
		if(i < ndesc) chTypes.push_back(chan_descr[i].n);
		else throw SegD3Exception("Channel type %s is incorrect", ct.c_str());
	}
	auto it = mapDir.find(f.base_path());
	f.start_traces_iteration(chTypes, output_samples, samples_number_must_be_equal, it == mapDir.end() ? -1 : dirfd(it->second));
	return true;
}

SegDFile::SegDFile(const SegDFile& f) : in(nullptr), pIOBuf(nullptr)
{
	*this = f;
}

SegDFile::~SegDFile()
{
	clear();
}

void SegDFile::clear()
{
	sBasePath.clear();
	sRelName.clear();
	chTypes.clear();
	filelength = 0;
	Label = StorageUnitLabel();
	file_type = SEGD3_FTYPE_UNKNOWN;
	m_record.clear();
	if(in) { fclose(in); in = nullptr; }
	if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
}

SegDFile& SegDFile::operator=(const SegDFile& f)
{
	if(in) { fclose(in); in = nullptr; }
	if(f.in) { fclose(f.in); f.in = nullptr; }
	if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
	if(f.pIOBuf) { free(f.pIOBuf); f.pIOBuf = nullptr; }
	sBasePath = f.sBasePath;
	sRelName = f.sRelName;
	Label = f.Label;
	m_record = f.m_record;
	//itCurrRecord = vRecords.begin();
	//std::vector<SegDRecord>::const_iterator itFRec = f.itCurrRecord;
	//std::advance(itCurrRecord, std::distance(f.vRecords.begin(), itFRec));
	file_type = f.file_type;
	chTypes = f.chTypes;
	filelength = f.filelength;
	return *this;
}

bool SegDFile::operator<(const SegDFile& f) const
{
	switch(file_type) {
	case SEGD3_FTYPE_LABEL:
		return true;
		break;
	case SEGD3_FTYPE_RECORDS:
		return (m_record.get_file_number() < f.m_record.get_file_number());
		break;
	case SEGD3_FTYPE_TOC:
		return false;
		break;
	case SEGD3_FTYPE_UNKNOWN: {
		int cmp_res = sBasePath.compare(f.sBasePath);
		if(cmp_res) return (cmp_res < 0);
		cmp_res = sRelName.compare(f.sRelName);
		return ( cmp_res ? (cmp_res < 0) : false );
	}
	}

	return false;
}

void SegDFile::Init(const std::string& fullpath)
{
	clear();
	off_t file_offset = 0;
	struct stat file_stat;
	std::string sFullPath = fullpath;

	try {
		in = fopen(sFullPath.c_str(), "r");
		if(!in) throw SegD3Exception(errno, "file open error");
		pIOBuf = Prime::SysService::SetPreferredIOBuffer(in);

		if(fstat(fileno(in), &file_stat) == -1) throw SegD3Exception(errno, "Fail to stat file");
		auto slash_pos = sFullPath.rfind('/');
		if(slash_pos != std::string::npos) {
			sBasePath = sFullPath.substr(0, slash_pos + 1);
			sRelName = sFullPath.substr(slash_pos + 1);
		}
		else {
			sBasePath.clear();
			sRelName = sFullPath;
		}
		filelength = file_stat.st_size;
		if(fread(Label.get_buffer(), 128, 1, in) != 1) throw SegD3Exception(errno, "read storage unit label error");
		if(Label.is_valid()) {
			file_type = SEGD3_FTYPE_LABEL;
			if(in) { fclose(in); in = nullptr; }
			if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
			return;
		}
		else {
			if(strncmp((const char*)Label.get_buffer(), TOC_LABEL, 13) == 0) {
				file_type = SEGD3_FTYPE_TOC;
				if(in) { fclose(in); in = nullptr; }
				if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
				return;
			}

			// Storage unit label отсутствует, вернемся назад
			if(fseeko(in, file_offset, SEEK_SET) == -1) throw SegD3Exception(errno, "rewind storage unit label error");
		}

		m_record.Init(path(), in, file_offset, filelength);
		file_type = SEGD3_FTYPE_RECORDS;
		if(in) { fclose(in); in = nullptr; }
		if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
	}
	catch(const SegD3Exception& e) {
		if(in) { fclose(in); in = nullptr; }
		if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
		throw SegD3Exception("SegDFile::Init(%s) exception: %s", sFullPath.c_str(), e.what());
	}
}

void SegDFile::Init(int fd_dir, const std::string& base_dir_path, const std::string& rel_path)
{
	clear();
	off_t file_offset = 0;
	struct stat file_stat;
	sBasePath.clear();
	sRelName.clear();

	try {
		int f = openat(fd_dir, rel_path.c_str(), O_RDONLY);
		if(f == -1) throw SegD3Exception(errno, "file open error");
		in = fdopen(f, "r");
		if(!in) throw SegD3Exception(errno, "file open error");
		pIOBuf = Prime::SysService::SetPreferredIOBuffer(in);

		if(fstat(f, &file_stat) == -1) throw SegD3Exception(errno, "Fail to stat file");
		sBasePath = base_dir_path;
		if(!sBasePath.empty() && *sBasePath.rbegin() != '/') sBasePath += "/";
		sRelName = rel_path;
		filelength = file_stat.st_size;
		if(fread(Label.get_buffer(), 128, 1, in) != 1) throw SegD3Exception(errno, "read storage unit label error");
		if(Label.is_valid()) {
			file_type = SEGD3_FTYPE_LABEL;
			if(in) { fclose(in); in = nullptr; }
			if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
			return;
		}
		else {
			if(strncmp((const char*)Label.get_buffer(), TOC_LABEL, 13) == 0) {
				file_type = SEGD3_FTYPE_TOC;
				if(in) { fclose(in); in = nullptr; }
				if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
				return;
			}

			// Storage unit label отсутствует, вернемся назад
			if(fseeko(in, file_offset, SEEK_SET) == -1) throw SegD3Exception(errno, "rewind storage unit label error");
		}

		m_record.Init(path(), in, file_offset, filelength);
		file_type = SEGD3_FTYPE_RECORDS;
		if(in) { fclose(in); in = nullptr; }
		if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
	}
	catch(const SegD3Exception& e) {
		if(in) { fclose(in); in = nullptr; }
		if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
		throw SegD3Exception("SegDFile::Init(%s) exception: %s", path().c_str(), e.what());
	}

}

void SegDFile::Open(int fd_dir)
{
	if(in) { fclose(in); in = nullptr; }
	if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
	off_t file_offset = 0;
	struct stat file_stat;

	try {
		if(fd_dir != -1) {
			int f = openat(fd_dir, sRelName.c_str(), O_RDONLY);
			if(f == -1) throw SegD3Exception(errno, "file open error");
			in = fdopen(f, "r");
		}
		else {
			in = fopen(path().c_str(), "r");
		}
		if(!in) throw SegD3Exception(errno, "file open error");
		pIOBuf = Prime::SysService::SetPreferredIOBuffer(in);

		if(fstat(fileno(in), &file_stat) == -1) throw SegD3Exception(errno, "Fail to stat file");
		filelength = file_stat.st_size;
		if(fread(Label.get_buffer(), 128, 1, in) != 1) throw SegD3Exception(errno, "read storage unit label error");
		if(Label.is_valid()) {
			file_type = SEGD3_FTYPE_LABEL;
			if(in) { fclose(in); in = nullptr; }
			if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
			return;
		}
		else {
			if(strncmp((const char*)Label.get_buffer(), TOC_LABEL, 13) == 0) {
				file_type = SEGD3_FTYPE_TOC;
				if(in) { fclose(in); in = nullptr; }
				if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
				return;
			}
			// Storage unit label отсутствует, вернемся назад
			if(fseeko(in, file_offset, SEEK_SET) == -1) throw SegD3Exception(errno, "rewind storage unit label error");
		}

		m_record.Init(path(), in, file_offset, filelength);
		file_type = SEGD3_FTYPE_RECORDS;
	}
	catch(const SegD3Exception& e) {
		if(in) { fclose(in); in = nullptr; }
		if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
		throw SegD3Exception("SegDFile::Init(%s) exception: %s", path().c_str(), e.what());
	}
}

void SegDFile::Close()
{
	if(in) { fclose(in); in = nullptr; }
	if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
	m_record.clear();
	file_type = SEGD3_FTYPE_UNKNOWN;
}


void SegDFile::get_channels_info(std::vector<SegDInfo>& chan_info) const
{
	SegDInfo inf;
	inf.nFileNum = record().get_file_number();
	inf.nVersionMajor = get_version_major();
	inf.nVersionMinor = record().general_header2().get_version_minor();
	if(record().is_source_info_valid()) {
		inf.nSLN = record().source_info().get_line();
		inf.nSLNfrac = record().source_info().get_line_fraction();
		inf.nSPnum = record().source_info().get_point();
		inf.nSPfrac = record().source_info().get_point_fraction();
		inf.nSPind = record().source_info().get_point_index();
	}
	for(int scanN = 0; scanN < record().scan_types().size(); ++scanN) {
		inf.nScanTypeNum = scanN + 1;
		int nChanSets = (get_version_major() < 3 ? record().scan_types()[scanN].get_channelsets2().size() : record().scan_types()[scanN].get_channelsets3().size());

		if(get_version_major() == 2) {
			for(auto chset : record().scan_types()[scanN].get_channelsets2()) {
				inf.nChannelSetNum = chset.get_number();
				inf.nChannelCode = chset.get_type();
				inf.sChannelType = get_channel_type_desc(get_version_major(), chset.get_type());
				inf.nSamplingIntMks = chset.sampling_interval_mks();
				inf.nTraceSamples = chset.trace_samples();
				inf.nChannelTraces = chset.get_chanels_number();
				inf.fDSMFactor = chset.get_DSM_factor();
				inf.nStartTimeMks = chset.get_start_time_mks();
				inf.nEndTimeMks = chset.get_end_time_mks();
				chan_info.push_back(inf);
			}
		}
		else if(get_version_major() == 3) {
			for(auto chset : record().scan_types()[scanN].get_channelsets3()) {
				inf.nChannelSetNum = chset.get_number();
				inf.nChannelCode = chset.get_type();
				inf.sChannelType = get_channel_type_desc(get_version_major(), chset.get_type());
				inf.nSamplingIntMks = chset.sampling_interval_mks();
				inf.nTraceSamples = chset.trace_samples();
				inf.nChannelTraces = chset.get_chanels_number();
				inf.fDSMFactor = chset.get_DSM_factor();
				inf.nStartTimeMks = chset.get_start_time_mks();
				inf.nEndTimeMks = chset.get_end_time_mks();
				chan_info.push_back(inf);
			}
		}
		else
			throw SegD3InconsistentVersion();
	}
}

void SegDFile::get_channeltype_list(std::vector<std::string>& chantypes) const
{
	if(get_type() == SEGD3_FTYPE_RECORDS)
		m_record.get_channeltype_list(chantypes);
	else
		chantypes.clear();
}

void SegDFile::start_traces_iteration(const std::vector<unsigned char>& chantypes, unsigned output_samples, bool samples_number_must_be_equal, int fd_parent)
{
	chTypes = chantypes;
	if(in) { fclose(in); in = NULL; }
	if(fd_parent != -1) {
		int fd = openat(fd_parent, sRelName.c_str(), O_RDONLY);
		if(fd == -1) throw SegD3Exception(errno, "file %s open error at base directory %s", sRelName.c_str(), sBasePath.c_str());
		in = fdopen(fd, "r");
	}
	else {
		//abort();
		in = fopen(path().c_str(), "r");
	}
	if(!in) throw SegD3Exception(errno, "file %s open error", path().c_str());
	if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
	pIOBuf = Prime::SysService::SetPreferredIOBuffer(in);
	m_record.start_traces_iteration(chTypes, output_samples, samples_number_must_be_equal);
}

bool SegDFile::get_next_trace(Prime::Tree::Trace& t, const std::vector<RemapItem>& remap)
{
	if(!m_record.get_next_trace(in, t, remap)) {
		fclose(in);
		in = NULL;
		if(pIOBuf) { free(pIOBuf); pIOBuf = nullptr; }
		return false;
	}
	return true;
}


void SegDRecord::clear()
{
	file_path.clear();
	src_block_num = -1;
	GenHeaders.clear();
	vScanTypes.clear();
	vExtendedHeaderBlocks.clear();
	vExternalHeaderBlocks.clear();
	trailer = GeneralTrailerBlock();
	if(sTraceBuffer) { free(sTraceBuffer); sTraceBuffer = nullptr; }
	vChSets2Read.clear();
	channel_map.clear();
	PrimeHeader.clear();
	header_offset = data_offset = trailer_offset = 0;
	nRecNumber = nRecordSize = 0;
	fDSM = 1;
	nCurrChSet = nCurrTrace = nCurrNTR = nOutputTraceSamples = 0;
	bInitialized = false;
}

void SegDRecord::Init(const std::string& path, FILE* in, off_t start_offset, off_t file_size)
{
	clear();
	header_offset = start_offset;

	try {
		if(fseeko(in, header_offset, SEEK_SET) == -1) throw SegD3Exception(errno, "Fail to seek file to record header (%llu position)", header_offset);
		GeneralBlockPtr genhdr1_ptr(new GeneralHeader1());
		if(fread(genhdr1_ptr->get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1) throw SegD3Exception(errno, "Fail to read general header 1");
		if(!genhdr1_ptr->is_valid()) {
			if(strncmp((const char*)genhdr1_ptr->get_buffer(), TOC_LABEL, 13) == 0) throw SegD3TOCFile();
			else throw SegD3Exception("General header 1 is not valid");
		}
		GenHeaders[1] = genhdr1_ptr;

		GeneralBlockPtr genhdr2_ptr(new GeneralHeader2());
		if(fread(genhdr2_ptr->get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1) throw SegD3Exception(errno, "Fail to read general header 2");
		if(!genhdr2_ptr->is_valid()) {
			GeneralHeader2& genhead2(*dynamic_cast<GeneralHeader2*>(genhdr2_ptr.get()));
			if(genhead2.get_version_major() < 2 || genhead2.get_version_major() > 3 || (genhead2.get_version_major() == 3 && genhead2.get_version_minor() > 1))
				throw SegD3Exception("SEG-D version %u.%u is not supported.", genhead2.get_version_major(), genhead2.get_version_minor());
			throw SegD3Exception("General header 2 is not valid");
		}
		GenHeaders[2] = genhdr2_ptr;

		if(dynamic_cast<GeneralHeader2*>(genhdr2_ptr.get())->get_version_major() >= 3) {
			GeneralBlockPtr genhdr3_ptr(new GeneralHeader3());
			if(fread(genhdr3_ptr->get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1) throw SegD3Exception(errno, "Fail to read general header 3");
			if(!genhdr3_ptr->is_valid()) throw SegD3Exception("General header 3 is not valid");
			GenHeaders[3] = genhdr3_ptr;
		}
		unsigned hnum = 0;

		try {
			hnum = get_headers_number() + 1;
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get headers number from record: %s", e.what());
		}
		for(unsigned genhead = (general_header2().get_version_major() < 3 ? 2 : 3); genhead < hnum; genhead++) {
			GeneralBlockPtr genblk(new GeneralBlock());
			if(fread(genblk->get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1) throw SegD3Exception(errno, "Fail to read general header %u", genhead);
			if(general_header2().get_version_major() <= 2) {
				// first block is source info, others - skip
				if(genhead == 2) {
					src_block_num = genhead + 1;
					GeneralBlockPtr srcinfo(new SourceInfo(*genblk));
					GenHeaders[src_block_num] = srcinfo;
				}
				else GenHeaders[genhead + 1] = genblk;
			}
			else {
				int nBlockID = genblk->get_type(general_header2().get_version_major());
				switch(nBlockID) {
				case 0x15:
				case 0x16:
				case 0x17:
				case 0x18:
				case 0x19:
				case 0x1F: {
					src_block_num = nBlockID;
					GeneralBlockPtr srcinfo(new SourceInfo(*genblk));
					GenHeaders[nBlockID] = srcinfo;
				}
					break;
				default:
					GenHeaders[nBlockID] = genblk;
					break;
				}
			}
		}

		unsigned sskew_size = get_sample_skew_blocks() * SEGD_HEADER_BLOCK;
		for(int nScanType = 0; nScanType < general_header1().get_scantypes_number(); nScanType++) {
			ScanType scantype;
			for(unsigned chset = 0; chset < get_channelsets(); chset++) {
				switch(general_header2().get_version_major()) {
				case 2: {
					ChannelSet2 cs(general_header2().get_version_minor());
					if(fread(cs.get_buffer(), SEGD_CHANNELSET_BLOCK2, 1, in) != 1)
						throw SegD3Exception(errno, "Fail to read channel set %u header for scantype %u", chset, nScanType);
					if(!cs.is_valid()) throw SegD3InvalidChannelSet();
					if(!cs.is_dummy()) {
						scantype.add_channel_set(cs);
						auto cs_desc = std::lower_bound(&chan_descr2[0], &chan_descr2[ChannelTypes2Num], ChannelTypeDescrT(cs.get_type(), ""));
						if(cs_desc != &chan_descr2[ChannelTypes2Num]) {
							auto ch = channel_map.find(cs_desc->descr);
							if(ch != channel_map.end())
								ch->second.push_back(TChSet(nScanType, (int)chset, (const ChannelSet*)nullptr));
							else
								channel_map.insert(std::pair< std::string, std::vector<TChSet> >(cs_desc->descr, std::vector<TChSet>(1, TChSet(nScanType, (int)chset, (const ChannelSet*)nullptr))));
						}
					}
				}
				break;
				case 3: {
					ChannelSet3 cs(general_header2().get_version_minor());
					if(fread(cs.get_buffer(), SEGD_CHANNELSET_BLOCK3, 1, in) != 1)
						throw SegD3Exception(errno, "Fail to read channel set %u header for scantype %u", chset, nScanType);
					if(!cs.is_valid()) throw SegD3InvalidChannelSet();
					if(!cs.is_dummy()) {
						scantype.add_channel_set(cs);
						auto cs_desc = std::lower_bound(&chan_descr3[0], &chan_descr3[ChannelTypes3Num], ChannelTypeDescrT(cs.get_type(), ""));
						if(cs_desc != &chan_descr3[ChannelTypes3Num]) {
							auto ch = channel_map.find(cs_desc->descr);
							if(ch != channel_map.end())
								ch->second.push_back(TChSet(nScanType, (int)chset, (const ChannelSet*)nullptr));
							else
								channel_map.insert(std::pair< std::string, std::vector<TChSet> >(cs_desc->descr, std::vector<TChSet>(1, TChSet(nScanType, (int)chset, (const ChannelSet*)nullptr))));
						}
					}
				}
				break;
				default:
					throw SegD3InconsistentVersion();
				}
			}

			if(sskew_size) {
				if(fread(scantype.get_sample_skew_buffer(sskew_size), sskew_size, 1, in) != 1)
					throw SegD3Exception(errno, "Fail to read sample skew for scantype %u", nScanType);
			}
			vScanTypes.push_back(scantype);
		}

		vExtendedHeaderBlocks.clear();
		if(get_extended_header_blocks()) {
			for(unsigned ext_hdr_blknum = 0; ext_hdr_blknum < get_extended_header_blocks(); ++ext_hdr_blknum) {
				GeneralBlockPtr extblk(new GeneralBlock());
				if(fread(extblk->get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1)
					throw SegD3Exception(errno, "Fail to read extended header %u", ext_hdr_blknum);
				vExtendedHeaderBlocks.push_back(extblk);
			}
		}

		vExternalHeaderBlocks.clear();
		if(get_external_header_blocks()) {
			for(unsigned ext_hdr_blknum = 0; ext_hdr_blknum < get_external_header_blocks(); ++ext_hdr_blknum) {
				GeneralBlockPtr extblk(new GeneralBlock());
				if(fread(extblk->get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1)
					throw SegD3Exception(errno, "Fail to read external header %u", ext_hdr_blknum);
				vExternalHeaderBlocks.push_back(extblk);
			}
		}
		data_offset = ftello(in);
		if(data_offset == off_t(-1)) throw SegD3Exception(errno, "Fail to get data offset pointer from file");

		// Проверка
		if(general_header2().get_version_major() == 3) {
			if(size_t(data_offset - header_offset) != general_header3().get_headers_size())
				throw SegD3Exception("Headers size at general header 3 (%llu bytes) is not confirmed with real record headers size (%llu bytes)", general_header3().get_headers_size(), size_t(data_offset - header_offset));
			trailer_offset = header_offset + general_header3().get_headers_and_data_size();
			nRecordSize = general_header3().get_record_size();
			if(nRecordSize == 0) {
				if(general_header2().get_general_trailer_blocks() != 0xFFFFFFFF)
					nRecordSize = (trailer_offset - header_offset) + general_header2().get_general_trailer_blocks() * SEGD_HEADER_BLOCK;
			}
		}
		else {
			size_t headers_length = (size_t)general_header1().get_scantypes_number() *
				(size_t(get_channelsets() + get_sample_skew_blocks()) + get_headers_number() + 1 +
					get_extended_header_blocks() + get_external_header_blocks()) * SEGD_HEADER_BLOCK;
			if(size_t(data_offset - header_offset) != headers_length)
				throw SegD3Exception("Calculated headers size (%llu bytes) is not confirmed with real record headers size (%llu bytes)", headers_length, size_t(data_offset - header_offset));
			nRecordSize = 0;
			trailer_offset = 0;
		}

		if(nRecordSize == 0) {
			nRecordSize = file_size - data_offset;
		}

		off_t nOffset = data_offset;
		if(general_header2().get_version_major() == 3) {
			for(auto& scan_type : vScanTypes) {
				for(auto& chset : scan_type.get_channelsets3()) {
					if(general_header3().is_extended_recording_mode())
						throw SegD3IncorrectFormat();
					chset.data_offset() = nOffset;
					chset.trace_samples() = chset.get_samples_number();
					chset.sampling_interval_mks() = chset.get_sampling_interval();
					if(chset.sampling_interval_mks() == 0) {
						double fSamplingInterval;
						try {
							fSamplingInterval = general_header1().get_base_scan_interval();
						}
						catch(const SegD3ExtendedValue& e) {
							fSamplingInterval = general_header2().get_base_scan_interval();
						}
						chset.sampling_interval_mks() = round(fSamplingInterval * 1E6);
					}

					if(!chset.trace_samples())
						chset.trace_samples() = (chset.get_end_time_mks() - chset.get_start_time_mks()) / chset.sampling_interval_mks();

					chset.trace_length() = chset.trace_samples() * general_header1().get_format_sample_size() + DEMUX_TRACE_HEADER_BLOCK + chset.get_trace_header_extensions() * SEGD_HEADER_BLOCK;
					chset.data_length() = off_t(chset.trace_length()) * chset.get_chanels_number();
					nOffset += chset.data_length();
				}
			}
		}
		else if(general_header2().get_version_major() == 2) {
			unsigned nScanType = 1, nChsetNum = 1;
			for(auto& scan_type : vScanTypes) {
				for(auto& chset : scan_type.get_channelsets2()) {
					const char *sChType = get_channel_type_desc(general_header2().get_version_major(), chset.get_type());
					chset.data_offset() = nOffset;
					if(chset.get_chanels_number() == 0)
						continue;
					DemuxTraceHeader demux_trh;
					if(fread(demux_trh.get_buffer(), DEMUX_TRACE_HEADER_BLOCK, 1, in) != 1)
						throw SegD3Exception(errno, "Fail to read first trace demux header channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);

					if(nScanType != demux_trh.get_scantype() || nChsetNum != demux_trh.get_channel_set())
						throw SegD3Exception("First demux trace header of channelset %s scantype %u is not valid", (sChType ? sChType : "undefined"), nScanType);

					unsigned nTraceHdrExtNum = demux_trh.get_trace_header_extensions();
					if(nTraceHdrExtNum > 0) {
						TraceHeaderExtension trh_ext(general_header2().get_version_major(), general_header2().get_version_minor());
						char sTraceHdrExt[SEGD_HEADER_BLOCK];
						if(fread(trh_ext.get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1)
							throw SegD3Exception(errno, "Fail to read first trace ext header channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);
						chset.trace_samples() = trh_ext.get_samples();
					}

					// Get sampling interval
					double fSamplingInterval;
					try {
						fSamplingInterval = general_header1().get_base_scan_interval();
					}
					catch(const SegD3ExtendedValue& e) {
						fSamplingInterval = general_header2().get_base_scan_interval();
					}
					chset.sampling_interval_mks() = round(fSamplingInterval * 1E6 / chset.get_subscan_intervals());

					// Test several trace samples and subscan intervals guess
					std::vector<unsigned> vTraceSamplesGuess, vSubscanIntervalsGuess;
					if(chset.trace_samples() != 0) {
						vTraceSamplesGuess.push_back(chset.trace_samples());
						vSubscanIntervalsGuess.push_back(chset.get_subscan_intervals());
					}
					vTraceSamplesGuess.push_back((chset.get_end_time_mks() - chset.get_start_time_mks()) / chset.sampling_interval_mks());
					vSubscanIntervalsGuess.push_back(chset.get_subscan_intervals());
					vTraceSamplesGuess.push_back((chset.get_end_time_mks() - chset.get_start_time_mks()) / chset.sampling_interval_mks() + 1);
					vSubscanIntervalsGuess.push_back(chset.get_subscan_intervals());
					if(chset.get_subscan_intervals() != 1) {
						vTraceSamplesGuess.push_back((chset.get_end_time_mks() - chset.get_start_time_mks()) / unsigned(round(fSamplingInterval * 1E6)) + 1);
						vSubscanIntervalsGuess.push_back(1);
						vTraceSamplesGuess.push_back((chset.get_end_time_mks() - chset.get_start_time_mks()) / unsigned(round(fSamplingInterval * 1E6)));
						vSubscanIntervalsGuess.push_back(1);
					}
					//chset.trace_samples() = (chset.get_end_time_mks() - chset.get_start_time_mks()) / chset.sampling_interval_mks() + 1;

					int guess = 0;
					bool bRightGuess = false;
					for(; guess < vTraceSamplesGuess.size() && !bRightGuess; ++guess) {
						// check next demux trace header
						//chset.trace_length() = chset.trace_samples() * general_header1().get_format_sample_size() + nTraceHdrExtNum * SEGD_HEADER_BLOCK + DEMUX_TRACE_HEADER_BLOCK;
						chset.trace_length() = vTraceSamplesGuess[guess] * general_header1().get_format_sample_size() + nTraceHdrExtNum * SEGD_HEADER_BLOCK + DEMUX_TRACE_HEADER_BLOCK;
						off_t nNextTraceOffset = chset.data_offset();
						nNextTraceOffset += chset.trace_length();

						if(nNextTraceOffset + DEMUX_TRACE_HEADER_BLOCK >= data_offset + nRecordSize) {
							if(nNextTraceOffset != file_size)
								continue; // This guess is wrong
							else {
								bRightGuess = true;
								chset.trace_samples() = vTraceSamplesGuess[guess];
								chset.sampling_interval_mks() = round(fSamplingInterval * 1E6 / vSubscanIntervalsGuess[guess]);
								break;
							}
						}
						if(fseek(in, nNextTraceOffset, SEEK_SET) == -1)
							throw SegD3Exception(errno, "Fail to seek next trace header within channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);
						DemuxTraceHeader demux_trh_next;
						if(fread(demux_trh_next.get_buffer(), DEMUX_TRACE_HEADER_BLOCK, 1, in) != 1)
							throw SegD3Exception(errno, "Fail to read next trace demux header within channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);

						unsigned next_trh_filenum, next_trh_chsetnum, next_trh_trsnum, next_trh_scannum;
						try {
							next_trh_filenum = demux_trh_next.get_file_number();
							next_trh_chsetnum = demux_trh_next.get_channel_set();
							try {
								next_trh_trsnum = demux_trh_next.get_trace_number();
							}
							catch(const SegD3ExtendedValue& e) {
								next_trh_trsnum = unsigned(-1);
							}
							next_trh_scannum = demux_trh_next.get_scantype();
						}
						catch(const SegD3Exception& e) {
							continue; // This guess is wrong
						}

						unsigned curr_trh_trsnum;
						try {
							curr_trh_trsnum = demux_trh.get_trace_number();
						}
						catch(const SegD3ExtendedValue& e) {
							curr_trh_trsnum = unsigned(-1);
						}
						bool bUseTrsNumCheck = (curr_trh_trsnum != unsigned(-1) && next_trh_trsnum != unsigned(-1));
						bool bTrsNumbersOk = (!bUseTrsNumCheck || next_trh_trsnum == demux_trh.get_trace_number() + 1);

 						if(next_trh_filenum != demux_trh.get_file_number() ||
								(chset.get_chanels_number() > 1 &&  (next_trh_chsetnum != demux_trh.get_channel_set() || !bTrsNumbersOk)) ||
								(chset.get_chanels_number() == 1 &&  next_trh_scannum < demux_trh.get_scantype())
							)
						{
							// May be it is general trailer?
							if(nNextTraceOffset + SEGD_HEADER_BLOCK == data_offset + nRecordSize) {
								// try to read genral trailer
								GeneralTrailerBlock gentrailer;
								if(fseek(in, nNextTraceOffset, SEEK_SET) == -1)
									throw SegD3Exception(errno, "Fail to seek next trace header within channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);
								if(fread(gentrailer.get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1)
									throw SegD3Exception(errno, "Fail to read general trailer");
								if(!gentrailer.is_valid())
									continue; // This gues is wrong
									//throw SegD3Exception(errno, "Fail to check next channelset trace within channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);
							}
							else
								continue; // This gues is wrong
								//throw SegD3Exception("Fail to get trace length for second trace of channelset %s scantype %u", (sChType ? sChType : "undefined"), nScanType);
						}
						bRightGuess = true;
						chset.trace_samples() = vTraceSamplesGuess[guess];
						chset.sampling_interval_mks() = round(fSamplingInterval * 1E6 / vSubscanIntervalsGuess[guess]);
					}
					if(!bRightGuess)
						throw SegD3Exception("Fail to get trace length for channelset %s(%u) scantype %u", (sChType ? sChType : "undefined"), nChsetNum, nScanType);

					chset.data_length() = off_t(chset.trace_length()) * chset.get_chanels_number();
					nOffset += chset.data_length();

					if(nOffset < file_size) {
						if(fseek(in, nOffset, SEEK_SET) == -1)
							throw SegD3Exception(errno, "Fail to seek next channelset or scantype to position %llu", nOffset);
					}
					++nChsetNum;
				}
				++nScanType;
			}

		}
		else
			throw SegD3InconsistentVersion();


		//		Utc tsZero = timestampToUtc(general_header3().get_time_zero());
		//		printf("Record zero time: %ld %lX %s %ld, diff=%ld\n", general_header3().get_time_zero(), general_header3().get_time_zero(), tsZero.toString(true), utcToTimestamp(tsZero), utcToTimestamp(tsZero) - general_header3().get_time_zero());
		//		Utc tsT = {2022, 12, 31, 23, 59, 59, 600000};
		//		Utc tsT1 = {2001, 2, 28, 23, 59, 59, 600000};
		//		printf("Utc test: %s\n", tsT.toString(true));
		//		printf("Utc test1: %s\n", tsT1.toString(true));



		// Перейдем к следующей записи
//		if(nRecordSize == 0) {
//			// Длина General trailer не определена
//			if(fseeko(in, header_offset + general_header3().get_headers_and_data_size(), SEEK_SET) == -1)
//				throw SegD3Exception(errno, "Fail to seek to general trailer");
//
//			if(fread(trailer.get_buffer(), SEGD_HEADER_BLOCK, 1, in) != 1) {
//				int err = errno;
//				// Проверим, ошибка или просто trailer отсутствует
//				if(!feof(in)) {
//					if(ferror(in)) throw SegD3Exception(errno, "Reading general trailer error");
//				}
//				else return;
//			}
//			nRecordSize = general_header3().get_headers_and_data_size();
//
//			if(!trailer.is_valid()) {
//				// Вернемся назад, считая что trailer отсутствует
//				if(fseeko(in, header_offset + general_header3().get_headers_and_data_size(), SEEK_SET) == -1)
//					throw SegD3Exception(errno, "Fail to seek to general trailer");
//			}
//			else nRecordSize += (trailer.get_trailer_blocks() + 1) * SEGD_HEADER_BLOCK;
//		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("SegDRecord::Init() exception: %s", e.what());
	}
	file_path = path;
	bInitialized = true;
	return;
}

void SegDRecord::get_channeltype_list(std::vector<std::string>& chantypes) const
{
	ScanTypes::const_iterator itScanType;
	for(itScanType = vScanTypes.begin(); itScanType != vScanTypes.end(); ++itScanType) {
		itScanType->get_channeltype_list(chantypes);
	}
}

std::vector<const ChannelSet*> SegDRecord::get_channelset(const std::string& chset_type) const
{
	std::vector<const ChannelSet*> res;
	auto ch = channel_map.find(chset_type);
	if(ch == channel_map.end() || ch->second.empty())
		throw SegD3Exception("Channel set %s is not found in record", chset_type.c_str());
//	if(ch->second.size() > 1)
//		throw SegD3Exception("Multiple channels of type %s exists", chset_type.c_str());
	for(const auto& chi : ch->second) {
		if(general_header2().get_version_major() < 3)
			res.push_back(&vScanTypes[chi.nScanType].get_channelsets2()[chi.nChannelSet]);
		else
			res.push_back(&vScanTypes[chi.nScanType].get_channelsets3()[chi.nChannelSet]);
	}
	return res;
}

unsigned SegDRecord::get_file_number() const
{
	unsigned res = 0;
	try {
		res = general_header1().get_file_number();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_file_number();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get file number from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get file number from general header 1", e.what());
	}
	return res;
}

unsigned SegDRecord::get_headers_number() const
{
	unsigned res = 0;
	try {
		res = general_header1().get_headers_number();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_headers_number();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get headers number from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get headers number from general header 1", e.what());
	}
	return res;
}

double SegDRecord::get_base_scan_interval() const
{
	double res = 0;
	try {
		res = general_header1().get_base_scan_interval();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_base_scan_interval();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get base scan interval from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get base scan interval from general header 1", e.what());
	}
	return res;
}

double SegDRecord::get_record_length() const
{
	double res = 0;
	try {
		res = general_header1().get_record_length();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_record_length();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get record length from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get get record length from general header 1", e.what());
	}
	return res;
}

unsigned SegDRecord::get_channelsets() const
{
	unsigned res = 0;
	try {
		res = general_header1().get_channelsets();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_channelsets();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get channel sets number from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get channel sets number from general header 1", e.what());
	}
	return res;
}

unsigned SegDRecord::get_sample_skew_blocks() const
{
	unsigned res = 0;
	try {
		res = general_header1().get_sample_skew_blocks();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_sample_skew_blocks();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get sample skew blocks from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get sample skew blocks from general header 1", e.what());
	}
	return res;
}


unsigned SegDRecord::get_extended_header_blocks() const
{
	unsigned res = 0;
	try {
		res = general_header1().get_extended_header_blocks();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_extended_header_blocks();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get extended header blocks from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get extended header blocks from general header 1", e.what());
	}
	return res;
}

unsigned SegDRecord::get_external_header_blocks() const
{
	unsigned res = 0;
	try {
		res = general_header1().get_external_header_blocks();
	}
	catch(const SegD3ExtendedValue& e) {
		try {
			res = general_header2().get_external_header_blocks();
		}
		catch(const SegD3Exception& e) {
			throw SegD3Exception("Fail to get external header blocks from general header 2", e.what());
		}
	}
	catch(const SegD3Exception& e) {
		throw SegD3Exception("Fail to get sample external header from general header 1", e.what());
	}
	return res;
}

void SegDRecord::start_traces_iteration(const std::vector<unsigned char> chTypes, unsigned output_samples, bool samples_number_must_be_equal)
{
	// Вычисление смещение первой трассы каналов заданного типа в данных
	int nScanType = 0, nChannelSet = 0;
	bool bChanFound = false;
	if(chTypes.empty())
		throw SegD3Exception("Channel types list is empty.");
	bool bTraceSamplesSet = false;
	unsigned nSamplingInterval;

	for(ScanTypes::iterator itScanType = vScanTypes.begin(); itScanType != vScanTypes.end(); ++itScanType, ++nScanType) {
		if(general_header2().get_version_major() == 3) {
			for(ChannelSets3::const_iterator itChSet = itScanType->get_channelsets3().begin(); itChSet != itScanType->get_channelsets3().end(); ++itChSet, ++nChannelSet) {
				if(std::find(chTypes.begin(), chTypes.end(), itChSet->get_type()) != chTypes.end()) {
					vChSets2Read.push_back(TChSet(nScanType, nChannelSet, &(*itChSet)));
					char *sTmp = (char*)realloc(sTraceBuffer, itChSet->trace_length());
					if(!sTmp) throw SegD3Exception("Fail to allocate %lu bytes for trace buffer", itChSet->trace_length());
					sTraceBuffer = sTmp;
					memset(sTraceBuffer, 0, itChSet->trace_length());
					if(samples_number_must_be_equal && (itChSet->trace_samples() != output_samples)) {
						std::ostringstream ss;
						ss <<  "'" << file_path << "', scantype=" << itChSet->get_scantype_number() << ", channel='" << get_channel_type_desc(general_header2().get_version_major(), itChSet->get_type()) << "': number of trace samples " << itChSet->trace_samples() <<
						" differs from " << output_samples << " detected automatically. Set parameter 'Output trace size (samples)' to input data with different number of trace samples.";
						throw SegD3Exception(ss.str().c_str());
					}
					if(bTraceSamplesSet) {
						if( (output_samples == 0 && nOutputTraceSamples != itChSet->trace_samples()) || nSamplingInterval != itChSet->sampling_interval_mks() )
							throw SegD3Exception("Different trace length or time step in selected channels");
					}
					else {
						nOutputTraceSamples = (output_samples == 0 ? itChSet->trace_samples() : output_samples);
						nSamplingInterval = itChSet->sampling_interval_mks();
						bTraceSamplesSet = true;
					}
					bChanFound = true;
				}
			}
		}
		else if(general_header2().get_version_major() == 2) {
			for(std::vector<ChannelSet2>::const_iterator itChSet = itScanType->get_channelsets2().begin(); itChSet != itScanType->get_channelsets2().end(); ++itChSet, ++nChannelSet) {
				if(std::find(chTypes.begin(), chTypes.end(), itChSet->get_type()) != chTypes.end()) {
					vChSets2Read.push_back(TChSet(nScanType, nChannelSet, &(*itChSet)));
					char *sTmp = (char*)realloc(sTraceBuffer, itChSet->trace_length());
					if(!sTmp) throw SegD3Exception("Fail to allocate %lu bytes for trace buffer", itChSet->trace_length());
					sTraceBuffer = sTmp;
					memset(sTraceBuffer, 0, itChSet->trace_length());
					if(samples_number_must_be_equal && (itChSet->trace_samples() != output_samples)) {
						std::ostringstream ss;
						ss <<  "'" << file_path << "', scantype=" << itChSet->get_scantype_number() << ", channel='" << get_channel_type_desc(general_header2().get_version_major(), itChSet->get_type()) << "': number of trace samples " << itChSet->trace_samples() <<
						" differs from " << output_samples << " detected automatically. Set parameter 'Output trace size (samples)' to input data with different number of trace samples.";
						throw SegD3Exception(ss.str().c_str());
					}
					if(bTraceSamplesSet) {
						if( (output_samples == 0 && nOutputTraceSamples != itChSet->trace_samples()) || nSamplingInterval != itChSet->sampling_interval_mks() )
							throw SegD3Exception("Different trace length or time step in selected channels");
					}
					else {
						nOutputTraceSamples = (output_samples == 0 ? itChSet->trace_samples() : output_samples);
						nSamplingInterval = itChSet->sampling_interval_mks();
						bTraceSamplesSet = true;
					}
					bChanFound = true;
				}
			}
		}
		else
			throw SegD3InconsistentVersion();

		//if(bChanFound) break;
	}
	if(!bChanFound) {
		std::ostringstream ossErr;
		ossErr << "Logic error. Channels with types (" ;
		auto ct = chTypes.begin();
		ossErr << (int)*ct++;
		for(; ct != chTypes.end(); ++ct) ossErr << ", " << (int)*ct;
		ossErr << ") was not found in record.";
		throw SegD3Exception(ossErr.str().c_str());
	}

	PrimeHeader.Init(nOutputTraceSamples, double(nSamplingInterval) * double(1E-6), FR_IO_TIME, Prime::IO::Metr);
	if(is_source_info_valid()) {
		PrimeHeader.Set(H_SLN, src_info().get_line());
		PrimeHeader.Set(H_SLN_FRAC, src_info().get_line_fraction());
		PrimeHeader.Set(H_NSP, src_info().get_point());
		PrimeHeader.Set(H_SPN_FRAC, src_info().get_point_fraction());
		PrimeHeader.Set(H_SCODE, general_header2().get_version_major() != 2 ? src_info().get_source_type() : (unsigned char)0);
		PrimeHeader.Set(H_SWD, int(round(double(src_info().get_depth_cm(general_header2().get_version_major())) / 10.)));
	}
	PrimeHeader.Set(H_RECN, get_file_number() /*TODO: nRecNumber*/);
	int year, day, hour, minute, second;
	general_header1().get_date(year, day, hour, minute, second);
	PrimeHeader.Set(H_YEAR, year);
	PrimeHeader.Set(H_DAY, day);
	PrimeHeader.Set(H_HOUR, hour);
	PrimeHeader.Set(H_MINUTE, minute);
	PrimeHeader.Set(H_SECOND, second);

	nCurrTrace = nCurrChSet = nCurrNTR = 0;
	const ChannelSet* chan = vChSets2Read[nCurrChSet].pChan;
	fDSM = chan->get_DSM_factor();
	PrimeHeader.Set(H_CST, int(chan->get_type()));

}

bool SegDRecord::get_next_trace(FILE *in, Prime::Tree::Trace& t, const std::vector<RemapItem>& remap)
{
	const ChannelSet* chset = vChSets2Read[nCurrChSet].pChan; //(general_header2().get_version_major() < 3 ? (ChannelSet*)&scan_types[nScanType].get_channelset2(nChannelSet) : (ChannelSet*)&scan_types[nScanType].get_channelset3(nChannelSet));
	if(nCurrTrace >= chset->get_chanels_number()) {
		// channel set was read, move to next
		++nCurrChSet;
		if(vChSets2Read.size() <= nCurrChSet)
			return false;
		chset = vChSets2Read[nCurrChSet].pChan;
		fDSM = chset->get_DSM_factor();
		PrimeHeader.Set(H_CST, int(chset->get_type()));
		if( !(general_header2().get_version_major() < 3 && vChSets2Read[nCurrChSet].pChan->get_type() == vChSets2Read[nCurrChSet - 1].pChan->get_type()) )
			nCurrNTR = 0;
		nCurrTrace = 0;
	}
	off_t trace_offset = chset->data_offset() + nCurrTrace * chset->trace_length();

	if(nCurrTrace == 0) {
		if(fseeko(in, trace_offset, SEEK_SET) == -1)
			throw SegD3Exception(errno, "Seek to %u trace of %u channelset %u scantype failed", nCurrTrace, vChSets2Read[nCurrChSet].nChannelSet, vChSets2Read[nCurrChSet].nScanType);
	}
	if(fread(sTraceBuffer, chset->trace_length(), 1, in) != 1)
		throw SegD3Exception(errno, "Fail to read %u trace %u channelset %u scantype", nCurrTrace, vChSets2Read[nCurrChSet].nChannelSet, vChSets2Read[nCurrChSet].nScanType);
	trace_offset +=  chset->trace_length();
	size_t nTracePos = 0;
	float fSensorSensitivity = 1;
	DemuxTraceHeader dtrh;
	std::map<int, size_t> ExtTrsHeadersPos;
	memcpy(dtrh.get_buffer(), sTraceBuffer + nTracePos, DEMUX_TRACE_HEADER_BLOCK); nTracePos += DEMUX_TRACE_HEADER_BLOCK;
	if(dtrh.get_scantype() != vChSets2Read[nCurrChSet].nScanType + 1) throw SegD3TraceIncorrectScantype(dtrh.get_scantype(), vChSets2Read[nCurrChSet].nScanType + 1, vChSets2Read[nCurrChSet].nChannelSet + 1, nCurrTrace + 1);
	if(dtrh.get_channel_set() != vChSets2Read[nCurrChSet].nChannelSet + 1) throw SegD3TraceIncorrectChannelSet(dtrh.get_channel_set(), vChSets2Read[nCurrChSet].nScanType + 1, vChSets2Read[nCurrChSet].nChannelSet + 1, nCurrTrace + 1);

	unsigned nExtHeaders = chset->get_trace_header_extensions();
	int nActual = ((dtrh.get_trace_edit() == 0 || dtrh.get_trace_edit() == 3) ? 1 : 0);
	PrimeHeader.Set(H_ACT, nActual);
	PrimeHeader.Set(H_STN, dtrh.get_scantype());
	PrimeHeader.Set(H_CSN, dtrh.get_channel_set());
	for(unsigned nExtHeader = 0; nExtHeader < nExtHeaders; nExtHeader++) {
		if(general_header2().get_version_major() < 3) {
			if(nExtHeader == 0) {
				TraceHeaderExtension ext_trh(general_header2().get_version_major(), general_header2().get_version_minor());
				memcpy(ext_trh.get_buffer(), sTraceBuffer + nTracePos, SEGD_HEADER_BLOCK);
				// Получаем информацию о приёмнике
				PrimeHeader.Set(H_RLN, ext_trh.get_receiver_line());
				PrimeHeader.Set(H_F_RLN, ext_trh.get_receiver_line_fraction());
				PrimeHeader.Set(H_NRP, ext_trh.get_receiver_number());
				PrimeHeader.Set(H_F_RPN, ext_trh.get_receiver_number_fraction());
				PrimeHeader.Set(H_RCODE, int(ext_trh.get_sensor_type()));
				try {
					nCurrNTR = dtrh.get_trace_number();
				}
				catch(const SegD3ExtendedValue& e) {
					++nCurrNTR;
				}
				PrimeHeader.Set(H_NTR, nCurrNTR);
			}
			ExtTrsHeadersPos.insert(std::pair<int, size_t>(nExtHeader + 1, nTracePos));
			nTracePos += SEGD_HEADER_BLOCK;
			continue;
		}

		unsigned char hdrtype = *(sTraceBuffer + nTracePos + SEGD_HEADER_BLOCK - 1);
		ExtTrsHeadersPos.insert(std::pair<int, size_t>(hdrtype, nTracePos));
		if(hdrtype == 0x40) {
			TraceHeaderExtension ext_trh(general_header2().get_version_major(), general_header2().get_version_minor());
			memcpy(ext_trh.get_buffer(), sTraceBuffer + nTracePos, SEGD_HEADER_BLOCK);
			// Получаем информацию о приёмнике
			PrimeHeader.Set(H_RLN, ext_trh.get_receiver_line());
			PrimeHeader.Set(H_F_RLN, ext_trh.get_receiver_line_fraction());
			PrimeHeader.Set(H_NRP, ext_trh.get_receiver_number());
			PrimeHeader.Set(H_F_RPN, ext_trh.get_receiver_number_fraction());
			PrimeHeader.Set(H_RCODE, int(ext_trh.get_sensor_type()));
			try {
				nCurrNTR = dtrh.get_trace_number();
			}
			catch(const SegD3ExtendedValue& e) {
				nCurrNTR = ext_trh.get_exttrace_number();
			}
			PrimeHeader.Set(H_NTR, nCurrNTR);
//			PrimeHeader.Set(H_TBA, );  Mute time - start time, ms */
//			PrimeHeader.Set(H_TEA, ); Mute time - end time, ms */
//			PrimeHeader.Set(H_FMAX, ); Maximum trace value if the data is in float. Negative if unknown
//			PrimeHeader.Set(H_TBC, ); H_TBC - Time of first sample (bits 4-32), ms
		}
		else if(hdrtype == 0x41) {
			SensorInfoTraceHeaderExtension si_ext;
			memcpy(si_ext.get_buffer(), sTraceBuffer + nTracePos, SEGD_HEADER_BLOCK);
			if(si_ext.is_test_passed()) fSensorSensitivity = si_ext.get_sensitivity();
		}
		else if(hdrtype == 0x42) {

		}
		nTracePos += SEGD_HEADER_BLOCK;
	}

	// Remap
	for(auto& ri: remap) {
		if(ri.segd_header_type.compare("General header") == 0) {
			general_header(ri.block_num_or_id).RemapField(PrimeHeader, ri);
		}
		else if(ri.segd_header_type.compare("Extended header") == 0) {
			if(ri.block_num_or_id > 0 && ri.block_num_or_id <= vExtendedHeaderBlocks.size())
				vExtendedHeaderBlocks[ri.block_num_or_id - 1]->RemapField(PrimeHeader, ri);
			else throw SegD3Exception("Extended header block %d is not found", ri.block_num_or_id);
		}
		else if(ri.segd_header_type.compare("External header") == 0) {
			if(ri.block_num_or_id > 0 && ri.block_num_or_id <= vExternalHeaderBlocks.size())
				vExternalHeaderBlocks[ri.block_num_or_id - 1]->RemapField(PrimeHeader, ri);
			else throw SegD3Exception("External header block %d is not found", ri.block_num_or_id);
		}
		else if(ri.segd_header_type.compare("Demux trace header") == 0) {
			dtrh.RemapField(PrimeHeader, ri);
		}
		else if(ri.segd_header_type.compare("Trace header extension") == 0) {
			auto it = ExtTrsHeadersPos.find(ri.block_num_or_id);
			if(it != ExtTrsHeadersPos.end()) {
				GeneralBlock blk;
				memcpy(blk.get_buffer(), sTraceBuffer + it->second, SEGD_HEADER_BLOCK);
				blk.RemapField(PrimeHeader, ri);
			}
			else throw SegD3Exception("Extended trace header block %d is not found in trace %u", ri.block_num_or_id, nCurrTrace);
		}
	}

	// Чтение данных трассы
	tr.Init(sTraceBuffer + nTracePos, chset->trace_samples(), general_header1().get_format(), fDSM /* * fSensorSensitivity*/);
	float fMaxAmpl = 0;
	if(chset->trace_samples() > 0) fMaxAmpl = fabsf(*tr.get_trace());
	for(auto smp = tr.get_trace(); smp < tr.get_trace() + chset->trace_samples(); ++smp)
		if(fabsf(*smp) > fMaxAmpl) fMaxAmpl = fabsf(*smp);
	PrimeHeader.SetFMAX(fMaxAmpl);
	t = Prime::Tree::Trace(PrimeHeader.Head(), tr.get_trace(), chset->trace_samples(), nOutputTraceSamples, nOutputTraceSamples); //, chset->trace_samples(), std::max(chset->trace_samples(), nOutputTraceSamples));
//	t.Head().SetTEA(double(chset->sampling_interval_mks()) / 1E6 * (chset->trace_samples() - 1));
	auto trbody = t.GetBody();
	int tba = 0, tea = 0;
	for(tba = 0; tba < nOutputTraceSamples && (trbody[tba] == 0); ++tba) ;
	if(tba)
		t.Head().SetTBA(double(chset->sampling_interval_mks()) / 1E6 * tba);
	if(chset->trace_samples() > nOutputTraceSamples) {
//		for(tea = chset->trace_samples() - 1; tea > tba && tr.get_trace()[tea] == 0; --tea) ;
		t.Head().Set(H_TEA, (chset->sampling_interval_mks() / 1000 * (chset->trace_samples() - 1)));
	}
	else {
		for(tea = std::min(nOutputTraceSamples - 1, chset->trace_samples() - 1); tea > tba && trbody[tea] == 0; --tea) ;
		t.Head().SetTEA(double(chset->sampling_interval_mks()) / 1E6 * tea);
	}
	++nCurrTrace;
	return true;
}

SegD3FormatCode GeneralHeader1::get_format() const
{
	int fmt = BcdToInt(&sHeader[2], 0, 4);
	if(std::binary_search(&segd3_formats[0], &segd3_formats[SegD3FmtNum], fmt)) return (SegD3FormatCode)fmt;
	else throw SegD3IncorrectFormat();
}

size_t GeneralHeader1::get_format_sample_size() const
{
	int fmt = BcdToInt(&sHeader[2], 0, 4);
	const int* pFmt = std::lower_bound(&segd3_formats[0], &segd3_formats[SegD3FmtNum], fmt);
	int nFmtInd = std::distance(&segd3_formats[0], pFmt);
	if(nFmtInd < SegD3FmtNum && *pFmt == fmt) return segd3_format_size[nFmtInd];
	else throw SegD3IncorrectFormat();
}

double GeneralHeader1::get_base_scan_interval() const  // seconds
{
	if(sHeader[22] == SegDConst_FF1) throw SegD3ExtendedValue();
	return double(sHeader[22]) * (1E-3 / 16.);
}

double GeneralHeader1::get_record_length() const  // seconds
{
	if(((sHeader[25] & 0x0F) == 0x0F) && (sHeader[26] == SegDConst_FF1)) throw SegD3ExtendedValue();
	return double(BcdToInt(&sHeader[25], 1, 3)) * 0.512;
}

void GeneralHeader1::get_date(int& year, int& day, int& hour, int& minute, int& second) const
{
	year = BcdToInt(&sHeader[10], 0, 2);
	day = BcdToInt(&sHeader[11], 1, 3);
	hour = BcdToInt(&sHeader[13], 0, 2);
	minute = BcdToInt(&sHeader[14], 0, 2);
	second = BcdToInt(&sHeader[15], 0, 2);
}

unsigned SourceInfo::get_depth_cm(unsigned ver) const
{
	if(ver < 3) return 0;
	if(sHeader[31] == 0x16 || sHeader[31] == 0x17 || sHeader[31] == 0x18) { // Explosive, Airgun, watergun
		return BinaryToUint(&sHeader[14], 2);
	}
	return 0;
}

void TraceData::Init(const char* data, int samples, SegD3FormatCode format, float dsm_factor)
{
	descale_multiplier = dsm_factor;
	size_t nBufSize = size_t(samples) * sizeof(float);
	if(samples != nSamples) {
		float *pfTmp = (float*)realloc(pSamples, nBufSize);
		if(pfTmp)
			pSamples = pfTmp;
		else
			throw SegD3Exception(errno, "Fail to reallocate buffer for trace of %d samples", samples);
		nSamples = samples;
	}
	memset(pSamples, 0, nBufSize);
	convert_data(data, format);
}

void TraceData::convert_data(const char* data, SegD3FormatCode format)
{
	switch(format){
		 case FMT_20BINARY: //8015
		 f8015toieee(nSamples, data, pSamples, descale_multiplier);
		 break;
		 case FMT_8QUATERNARY: //8022
			 f8022toieee(nSamples, data, pSamples, descale_multiplier);
		 break;
		 case FMT_16QUATERNARY: //8024
			 f8024toieee(nSamples, data, pSamples, descale_multiplier);
		 break;
		 case FMT_24INTEGER: //8036
			 f8036toieee(nSamples, data, pSamples, descale_multiplier);
		 break;
		 case FMT_32INTEGER: //8038
			 f8038toieee(nSamples, data, pSamples, descale_multiplier);
		 break;
		 case FMT_8HEX: //8042
			 f8042toieee(nSamples, data, pSamples, descale_multiplier);
			 break;
		 case FMT_16HEX: //8044
			 f8044toieee(nSamples, data, pSamples, descale_multiplier);
			 break;
		 case FMT_32HEX: //8048
			 ibmtoieee(nSamples, data, pSamples, descale_multiplier);
		 break;
		 case FMT_32IEEE: //8058
			 invertieee32(nSamples, (unsigned char*)data, (unsigned char*)pSamples, descale_multiplier);
			 break;
		 case FMT_64IEEE: //8080
			 invertieee64(nSamples, (unsigned char*)data, (unsigned char*)pSamples, descale_multiplier);
		 break;
		 default:
			 throw SegD3IncorrectFormat();
	}
}

void ScanType::get_channeltype_list(std::vector<std::string>& chantypes) const
{
	int ndesc = 0;
	ChannelTypeDescrT* chan_descr = get_channel_types(version_major, &ndesc);

	if(version_major == 3) {
		for(ChannelSets3::const_iterator itCS = vChannelSets3.begin(); itCS != vChannelSets3.end(); ++itCS) {
			ChannelTypeDescrT cdt(itCS->get_type(), "");
			ChannelTypeDescrT *itChDescr = std::lower_bound(&chan_descr[0], &chan_descr[ndesc], cdt);
			if(itChDescr == &chan_descr[ndesc] || itChDescr->n != cdt.n) throw SegD3Exception("Channel type %u unrecognized", cdt.n);
			std::vector<std::string>::iterator itCT = std::lower_bound(chantypes.begin(), chantypes.end(), itChDescr->descr);
			if(itCT == chantypes.end() || *itCT != itChDescr->descr) chantypes.insert(itCT, itChDescr->descr);
		}
	}
	else if(version_major == 2) {
		for(ChannelSets2::const_iterator itCS = vChannelSets2.begin(); itCS != vChannelSets2.end(); ++itCS) {
			ChannelTypeDescrT cdt(itCS->get_type(), "");
			ChannelTypeDescrT *itChDescr = std::lower_bound(&chan_descr[0], &chan_descr[ndesc], cdt);
			if(itChDescr == &chan_descr[ndesc] || itChDescr->n != cdt.n) throw SegD3Exception("Channel type %u unrecognized", cdt.n);
			std::vector<std::string>::iterator itCT = std::lower_bound(chantypes.begin(), chantypes.end(), itChDescr->descr);
			if(itCT == chantypes.end() || *itCT != itChDescr->descr) chantypes.insert(itCT, itChDescr->descr);
		}
	}
	else
		throw SegD3InconsistentVersion();
}

unsigned DemuxTraceHeader::get_file_number() const
{
	unsigned nfile;
	try {
		nfile = BcdToInt(&trh[0], 0, 4);
	}
	catch(const SegD3ExtendedValue& e) {
		nfile = BinaryToUint(&trh[17], 3);
	}
	return nfile;
}

unsigned DemuxTraceHeader::get_channel_set() const
{
	unsigned chset;
	try {
		chset = BcdToInt(&trh[3], 0, 2);
	}
	catch(const SegD3ExtendedValue& e) {
		chset = BinaryToUint(&trh[15], 2);
	}
	return chset;
}

double DemuxTraceHeader::get_first_timing_word() const
{
	return double(BinaryToUint(&trh[6], 2)) + double(trh[8]) / 256.;

}

int TraceHeaderExtension::get_receiver_line() const
{
	if(memcmp(&sHeader[0], (unsigned char*)&SegDConst_FF3, 3))
		return BinaryToInt(&sHeader[0], 3);
	return BinaryToInt(&sHeader[10], 3);
}
int TraceHeaderExtension::get_receiver_line_fraction() const
{
	if(memcmp(&sHeader[0], (unsigned char*)&SegDConst_FF3, 3))
		return 0;
	return BinaryToInt(&sHeader[13], 2);
}

int TraceHeaderExtension::get_receiver_number() const
{
	if(memcmp(&sHeader[0], (unsigned char*)&SegDConst_FF3, 3))
		return BinaryToInt(&sHeader[3], 3);
	return BinaryToInt(&sHeader[15], 3);
}

int TraceHeaderExtension::get_receiver_number_fraction() const
{
	if(memcmp(&sHeader[3], (unsigned char*)&SegDConst_FF3, 3))
		return 0;
	return BinaryToInt(&sHeader[18], 2);
}

} // namespace SEGD3
} // namespace Prime
