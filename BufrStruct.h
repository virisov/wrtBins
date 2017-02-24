#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <cmath>

using namespace std;

typedef vector<uint8_t> VBytes;

const int NF = 74;

struct bitField
{
	long begin;
	int width;
	int scale;
	long refValue;
	uint32_t value;
	double dvalue;
};

inline double bit2double(uint32_t x, bitField& w)
{
	return (static_cast<long int>(x) + w.refValue)*pow(10.0, -w.scale);
};

inline uint32_t uInt8(const VBytes& a, int idx)
{
	return static_cast<uint32_t>(a[idx]);
};

inline uint32_t uInt16(const VBytes& a, int idx)
{
	return static_cast<uint32_t>(a[idx]) * 256 +
		static_cast<uint32_t>(a[idx + 1]);
};

inline uint32_t uInt24(const VBytes& a, int idx)
{
	return static_cast<uint32_t>(a[idx]) * 256 * 256 +
		static_cast<uint32_t>(a[idx + 1]) * 256 +
		static_cast<uint32_t>(a[idx + 2]);
};

inline uint32_t uInt32(const VBytes& a, int idx)
{
	return static_cast<uint32_t>(a[idx]) * 256 * 256 * 256 +
		static_cast<uint32_t>(a[idx + 1]) * 256 * 256 +
		static_cast<uint32_t>(a[idx + 2]) * 256 +
		static_cast<uint32_t>(a[idx + 3]);
};

class BufrStruct
{
private:
	// nominal values for COSMIC product
	const uint32_t N0 = 1;
	uint32_t N1 = 300;
	uint32_t N2 = 300;
	uint32_t N3 = 200;

	VBytes sect0, sect1, sect3, sect40, sect4, sect5;
	bitField s[NF];

	void breakSect(VBytes& bStr, VBytes& sect0, VBytes& sect1, VBytes& sect3, VBytes& sect4, VBytes& sect5, size_t NN[]);

	void setBitValue(const uint32_t w, const long beg, const int width, VBytes& a);
	long setBitUValue(const bitField& w, const long beg, VBytes& a);
	long setBitDValue(bitField& w, const long beg, VBytes& a);
	long setByteValue(const uint32_t v, const long bbeg, const int bwidth, VBytes& a);

	uint32_t getBitValue(const long beg, const int width, VBytes& a);
	long getBitUValue(bitField& w, const long beg, VBytes& a);
	long getBitDValue(bitField& w, const long beg, VBytes& a);
	uint32_t getByteValue(const long bbeg, const int bwidth, VBytes& a);

	inline long setDValue(bitField& s, const long beg, double x)
	{
		s.dvalue = x;
		return setBitDValue(s, beg, sect4);
	}

	inline long setUValue(bitField& s, const long beg, uint32_t n)
	{
		s.value = n;
		return setBitUValue(s, beg, sect4);
	}

	inline long getDValue(bitField& s, const long beg, double& x)
	{
		long b = getBitDValue(s, beg, sect4);
		x = s.dvalue;
		return b;
	}

	inline long getUValue(bitField& s, const long beg, uint32_t& n)
	{
		long b = getBitUValue(s, beg, sect4);
		n = s.value;
		return b;
	}

	long readS4blockABC(const long beg0, const int jv, uint32_t& N, double x[]);
	long writeS4blockABC(const long beg0, const int jv, const uint32_t N, const double x[]);

public:

	BufrStruct();
	~BufrStruct() {};
	int loadFile(string fName, const int kBufr = 1);
	int saveFile(string fName);

	size_t initSection1(const uint32_t dt[]);
	size_t initSection3();
	size_t initSection4(const int n1, const int n2, const int n3);
	size_t initSection05();
	void setSection0BufrLength(uint32_t m);
	void setSection4Length(uint32_t m);

	long getS4bitValue(int i);
	long getS4bitValue(int i, const long begin);
	uint32_t getUValue(int i) { return s[i].value;  }
	double getDValue(int i) { return s[i].dvalue; }

	VBytes& getSect4() { return sect4; }
	bitField& getS(int i) { return s[i]; }

	long readS4block(const string blockName, double x[]);
	long readS4block(const string blockName, uint32_t x[]);

	long writeS4block(const string blockName, const double x[]);
	long writeS4block(const string blockName, const uint32_t x[]);

	long writeS4block1a(const uint32_t w[], const double x[]);
	long writeS4block1a(const double x[]);
	long writeS4block1b(const double x[]);
	long writeS4block2a(const long beg0, const double x[]);
	long writeS4block2b(const long beg0, const double x[]);
	long writeS4block2c(const long beg0, const double x[]);
	
	long readS4block1a(double x[], uint32_t& m);
	long readS4block1b(double x[], uint32_t& n1);
	long readS4block2a(const long beg0, double x[], uint32_t& n2);
	long readS4block2b(const long beg0, double x[], uint32_t& n3);
	long readS4block2c(const long beg0, double x[]);
};

