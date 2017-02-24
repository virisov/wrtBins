#pragma once

using namespace std;

#include "ROBinary2.h"
#include "dataVec.h"

#include <vector>
#include <string>

// all numerical data must be saved in big-endian format

class opnGNSS 
{
private:

	dataVec* x;
	size_t NUM_TAPS;

	uint16_t makeObsType(char obsType, uint16_t freqBand, char att, char metaData);

	long pushHRdata(TIMEtype T, TIMEtype adr, vector<IQtype>& I3, vector<IQtype>& Q3, long k);

	long pushTrailer(int PRN, int SIG_TYPE, int TRACK_TYPE, double REF_FOS, long k);

	long pushBlock(ROBinary& ro, uint32_t GPSsec, uint32_t gt1, size_t i0, uint16_t Nhr, long k);

public:

	opnGNSS(ROBinary& ro);
	~opnGNSS() { delete x; }

	long saveFile(const string fName) { return x->saveFile(fName); }
};
