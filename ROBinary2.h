#pragma once

using namespace std;

#include <vector>
#include <string>

#include "dataVec.h"

typedef double TIMEtype;
const char TIMEtype_char = 'd';		// double

typedef double ADRtype;
const char ADRtype_char = 'd';		// double

typedef int16_t IQtype;
const char IQtype_char = 's';		// signed short

// numeric values are little endian

class ROBinary {

private:

	dataVec* data;
	int error = 0;

	string fNameIn;
	size_t Nobs, Ntaps;

	vector<TIMEtype> TIME_OFFS;
	vector<ADRtype> ADR;
	vector<vector<IQtype>> I;
	vector<vector<IQtype>> Q;
	
public:

	ROBinary(const string fName, int verbose);
	~ROBinary() { delete data; }
	void streamOutput(ostream& f);

	int getError() { return error; }

	string get_VSTRING();
	uint32_t get_REF_WEEK() { return data->getLE32(16); }
	uint32_t get_REF_SOW() { return data->getLE32(20); }
	double get_REF_FOS() { return data->getLEd(24);	}
	double get_REF_PRANGE() { return data->getLEd(32); }
	uint32_t get_TX_ID() { return data->getLE32(40); }
	uint32_t get_SIG_TYPE() { return data->getLE32(44); }
	uint32_t get_ANT_NUM() { return data->getLE32(48); }
	uint32_t get_TRACK_TYPE() { return data->getLE32(52); }
	uint32_t get_SAMP_FREQ() { return data->getLE32(56); }
	uint32_t get_NUM_TAPS() { return data->getLE32(60); }

	vector<TIMEtype>& get_TIME_OFFS() { return TIME_OFFS; }
	vector<ADRtype>& get_ADR() { return ADR;}
	vector<vector<IQtype>>& get_I() { return I; }
	vector<vector<IQtype>>& get_Q() { return Q; }

	// Vectors: [NOBS]
	// I & Q matrices: [NOBS,  NUM_TAPS]
	// (thanks to https://goo.gl/k9KcJj)

	void save_ascii(const string fName);
};

