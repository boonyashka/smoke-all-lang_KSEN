#include <stdio.h>
#include <errno.h>
#include <vector>
#include <string>
#include <exception>
#include <algorithm>
#include "segd3.h"
#include "Seism/Trace.h"
#include "trshead/TraceHead.h"

using namespace Prime::SEGD3;

int main(int argc, char *argv[])
{
	FILE *pPrimeFile = NULL;
	SegDSource src;
	SegDFile f;
	std::vector<RemapItem> vRemap;
	RemapItem ri;
	ri.segd_header_type = "Demux trace header";
//	ri.format = SEGD_FORMAT_BCD;
//	ri.offset = 8;
//	ri.length = 4;
	ri.format = SEGD_FORMAT_BINARY;
	ri.start_bit = 17 * 8;
	ri.end_bit = ri.start_bit + 3 * 8 - 1;
//	ri.offset = 17;
//	ri.length = 3;
	ri.prime_field = Prime::HdrField(H_RES3);
	vRemap.push_back(ri);

	ri.segd_header_type = "Trace header extension";
	ri.block_num_or_id = 0x40;
	ri.format = SEGD_FORMAT_BINARY;
	ri.start_bit = 15 * 8;
	ri.end_bit = ri.start_bit + 3 * 8 - 1;
//	ri.offset = 15;
//	ri.length = 3;
	ri.prime_field = Prime::HdrField(H_RES4);
	vRemap.push_back(ri);

	ri.segd_header_type = "General header";
	ri.block_num_or_id = 0x1;
	ri.format = SEGD_FORMAT_BCD;
	ri.start_bit = 10 * 8;
	ri.end_bit = ri.start_bit + 8 - 1;
	ri.prime_field = Prime::HdrField(H_YEAR);
	vRemap.push_back(ri);


	ri.segd_header_type = "General header";
	ri.block_num_or_id = 0x1;
	ri.format = SEGD_FORMAT_BCD;
	ri.start_bit = 11 * 8 + 4;
	ri.end_bit = ri.start_bit + 3*4 - 1;
	ri.prime_field = Prime::HdrField(H_DAY);
	vRemap.push_back(ri);

	ri.segd_header_type = "General header";
	ri.block_num_or_id = 0x16;
	ri.format = SEGD_FORMAT_BINARY;
	ri.start_bit = 3 * 8;
	ri.end_bit = ri.start_bit + 3*8 - 1;
	ri.prime_field = Prime::HdrField(H_SLN);
	vRemap.push_back(ri);

	ri.segd_header_type = "General header";
	ri.block_num_or_id = 0x16;
	ri.format = SEGD_FORMAT_BINARY;
	ri.start_bit = 8 * 8;
	ri.end_bit = ri.start_bit + 3*8 - 1;
	ri.prime_field = Prime::HdrField(H_NSP);
	vRemap.push_back(ri);

	try {
		std::vector<std::string> vChanTypes(1, "Seis");
		//src.prepare_files("/data/moosefs/slip_swip_moav/Datasets/test/Sources/00249587.segd", vChanTypes); // /home/data/segd3/Sources
		src.prepare_files("/data/moosefs/update_4_2_test3d/Extra/Segd3/00000001.segd", vChanTypes);
//		pPrimeFile = fopen("/home/ppatrikeev/dev/OUTPUT_SEGD3", "w"); // /home/ppv/dev/data/segd3/OUT_SEGD3", "w");
//		if(!pPrimeFile) throw SegD3Exception(errno, "Fail to open file for writing");
//		std::vector<std::string> vChanTypes = std::vector<std::string>(1, "Seis"); //src.get_channeltype_list();
		int nchtype = 0;
		std::string sSelectedChSet = "Seis";
//		if(vChanTypes.size() > 1) {
//			std::cout << "Select channel type:" << std::endl;
//			for(int i = 0; i < vChanTypes.size(); ++i) {
//				std::cout << i + 1 << ") " << vChanTypes[i] << std::endl;
//			}
//			std::cout << "Or type 0 to exit" << std::endl << "Choose: ";
//			scanf("%d", &nchtype);
//			if(nchtype < 1 || nchtype > vChanTypes.size()) return -1;
//			sSelectedChSet = vChanTypes[nchtype - 1];
//		}
//		else sSelectedChSet = vChanTypes[0];
		std::vector<std::string>::iterator itSeisChan = std::find(vChanTypes.begin(), vChanTypes.end(), sSelectedChSet);
		if(itSeisChan != vChanTypes.end()) {
			src.start_file_iteration( std::vector<std::string>(1, sSelectedChSet));
			unsigned sampling_rate_mks = 0, trace_samples = 0;
			size_t approx_traces;
			src.get_channels_info(vChanTypes, &sampling_rate_mks, &trace_samples, &approx_traces);
			while(src.get_next_file(f, trace_samples, false)) {
				std::cout << "File " << f.path() << std::endl;
				std::cout << "General header 1:" << std::endl;
				//f.record().general_header1().test();
				std::cout << f.record().general_header1().dump_raw() << std::endl << f.record().general_header1().dump() << std::endl;

				std::cout << "General header 2:" << std::endl;
				std::cout << f.record().general_header2().dump_raw() << std::endl << f.record().general_header2().dump() << std::endl;

				if(f.get_version_major() > 2 && f.record().general_header3().is_valid()) {
					std::cout << "General header 3:" << std::endl;
					std::cout << f.record().general_header3().dump_raw() << std::endl << f.record().general_header3().dump() << std::endl;
				}

				if(f.record().is_source_info_valid() && f.record().source_info().is_valid(f.get_version_major())) {
					std::cout << "Source info:" << std::endl;
					std::cout << f.record().source_info().dump_raw() << std::endl << f.record().source_info().dump() << std::endl;
				}

				//f.get_records_num();
				Prime::Tree::Trace t;
				bool bFirst = true;
				while(f.get_next_trace(t, vRemap)) {
					if(pPrimeFile) {
						if(fwrite(t.Head().Head(), CGG_TR_HEAD, 1, pPrimeFile) != 1) throw SegD3Exception(errno, "Fail to write trace header to file");
						if(fwrite(t.GetBody(), 4, t.Head().GetLTR(), pPrimeFile) != t.Head().GetLTR()) throw SegD3Exception(errno, "Fail to write trace body to file");
					}
					if(bFirst) {
						std::cout << "Trace length=" << t.length() << " samples." << std::endl;
						bFirst = false;
					}
				}
				std::cout << std::endl << std::endl << std::endl;
			}
		}
		if(pPrimeFile) { fclose(pPrimeFile); pPrimeFile = NULL; }
	}
	catch(const std::exception& e) {
		if(pPrimeFile) { fclose(pPrimeFile); pPrimeFile = NULL; }
		fprintf(stderr, "Exception caught: %s", e.what());
	}
	return 0;
}




