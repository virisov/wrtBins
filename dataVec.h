#pragma once

#include <stdint.h>
#include <vector>

using namespace std;

size_t getFileSize(ifstream& f);

class dataVec
{
private:

	bool LITTLE_ENDIAN1 = true;
	int error = 0;

	vector<uint8_t> data;

	long putBEm(uint64_t w, int m, long k);

public:

	dataVec(const string fName);
	dataVec(const size_t m);

	int getError() { return error; }
	size_t getSize() { return data.size(); }

	uint8_t& operator[](int i) { return data[i]; };

	long putBE(uint8_t w, long k) { data[k] = w; return k + 1; }
	long putBE(uint16_t w, long k) { return putBEm(static_cast<uint16_t>(w), 2, k); }
	long putBE(uint32_t w, long k) { return putBEm(static_cast<uint32_t>(w), 4, k); }
	long putBE24(uint32_t w, long k) { return putBEm(static_cast<uint32_t>(w), 3, k); }
	long putBE(uint64_t w, long k) { return putBEm(w, 8, k); }
	long putBE(double w, long k);

	long getLEm(uint64_t& w, int m, long k);
	long getLEms(int64_t& w, int m, long k);
	long getLEd(double& w, long k);

	uint32_t getLE32(long k) 
	{
		uint64_t w;
		getLEm(w, 4, k);
		return static_cast<uint32_t>(w);
	}

	double getLEd(long k)
	{
		double w;
		getLEd(w, k);
		return w;
	}

	long saveFile(const string fName);
};


