#include "BufrStruct.h"
#include <iostream>
#include <fstream>
#include <algorithm>

// ------------------------------------------------------------------------------------------

struct idxDescription
{
	int jbeg;
	int jlen;
	string descr;
};

vector<idxDescription> vecDescr = 
{
	{ 0,  5, "RO header" },
	{ 5,  7, "Time of occEvent" },
	{ 12,  2, "RO summary quality" },
	{ 14, 14, "LEO & GNSS POD" },
	{ 28,  9, "Local Earth parameters" },
	{ 37, 12, "RO Step1b data" },
	{ 49,  7, "RO Step2a data" },
	{ 56, 11, "RO Level2b data" },
	{ 67,  7, "RO Level2c data" }
};

// ------------------------------------------------------------------------------------------

const int tableBitWidth[NF] = {
	10,11,8,8,14,
	5,12,4,6,5,
	6,16,16,7,31,
	31,31,31,31,31,
	9,17,31,31,31,
	31,31,31,18,25,
	26,31,31,31,22,
	16,15,16,25,26,
	16,8,7,22,23,
	6,20,6,7,16,
	17,19,6,14,6,
	7,16,17,14,12,
	14,6,6,6,9,
	6,7,6,17,14,
	6,6,6,7 };

const int tableScale[NF] = {
	0,0,0,0,0,
	0,0,0,0,0,
	0,3,0,0,2,
	2,2,5,5,5,
	0,0,1,1,1,
	5,5,5,3,5,
	5,2,2,2,1,
	2,2,0,5,5,
	2,0,-8,1,8,
	0,8,0,0,0,
	0,3,0,3,0,
	0,0,0,-1,1,
	5,0,-1,1,5,
	0,0,0,0,-1,
	0,-1,0,0 };

const long tableRefValue[NF] = {
	0,0,0,0,0,
	0,0,0,0,0,
	0,0,0,0,-1073741824,
	-1073741824,-1073741824,-1073741824,-1073741824,-1073741824,
	0,0,-1073741824,-1073741824,-1073741824,
	-1073741824,-1073741824,-1073741824,-4096,-9000000,
	-18000000,-1073741824,-1073741824,-1073741824,62000000,
	0,-15000,0,-9000000,-18000000,
	0,0,0,62000000,-100000,
	0,-100000,0,0,0,
	-1000,0,0,0,0,
	0,0,-1000,0,0,
	0,0,0,0,0,
	0,0,0,-1000,0,
	0,0,0,0 };

// ------------------------------------------------------------------------------------------

VBytes subVect(VBytes& a, int beg = 0, int len = -1)
{
	VBytes::const_iterator first = a.begin() + beg;
	VBytes::const_iterator last = a.begin() + beg + len;
	return VBytes(first, last);
};

// ------------------------------------------------------------------------------------------

BufrStruct::BufrStruct()
{
	for (int i = 0; i < NF; i++)
	{
		s[i].width = tableBitWidth[i];
		s[i].scale = tableScale[i];
		s[i].refValue = tableRefValue[i];
		if (i == 0) s[i].begin = 0;
		else s[i].begin = s[i - 1].begin + s[i - 1].width;
		s[i].value = 0ul;
		s[i].dvalue = 0.0;
	}

	sect0.resize(8);
	sect5.resize(4);
};

// ------------------------------------------------------------------------------------------

void BufrStruct::breakSect(VBytes& bStr, VBytes& sect0, VBytes& sect1, VBytes& sect3, VBytes& sect4, VBytes& sect5, size_t NN[])
{
	sect0 = subVect(bStr, 0, 8);
	size_t N0 = uInt24(bStr, 4);
	size_t N1 = uInt24(bStr, 8);
	sect1 = subVect(bStr, 8, N1);
	size_t N3 = uInt24(bStr, 8 + N1);
	sect3 = subVect(bStr, 8 + N1, N3);
	size_t N4 = uInt24(bStr, 8 + N1 + N3);
	sect4 = subVect(bStr, 8 + N1 + N3, N4);
	sect5 = subVect(bStr, 8 + N1 + N3 + N4, 4);
	NN[0] = N0;
	NN[1] = N1;
	NN[2] = N3;
	NN[3] = N4;
};

// ------------------------------------------------------------------------------------------

int BufrStruct::loadFile(string fName, const int kBufr)
{
	ifstream fin;
	fin.open(fName, ios::binary);
	if (!fin.is_open())
	{
		cerr << "Error code: " << errno << endl;
		return -1;
	}

	fin.seekg(0, ios::end);
	size_t fSize = static_cast<size_t>(fin.tellg());
	fin.seekg(0, ios::beg);
	string strFile = string{ istreambuf_iterator<char>(fin), istreambuf_iterator<char>() };
	fin.close();

	int iBUFR = 0;
	int i7777;
	for (int i = 0; i < kBufr; i++)
	{
		if(i==0) iBUFR = strFile.find("BUFR", 0);
		else iBUFR = strFile.find("BUFR", iBUFR + 1);
		if (iBUFR == string::npos) return -1;
	}
	if((i7777 = strFile.find("7777", iBUFR)) == string::npos) return -1;
	string bStr = strFile.substr(iBUFR, i7777 - iBUFR + 4);
	VBytes bytes = VBytes{ bStr.begin(), bStr.end() };

	size_t NN[4];
	breakSect(bytes, sect0, sect1, sect3, sect40, sect5, NN);

	sect4 = subVect(sect40, 4, NN[3] - 4);
	for (int i = 0; i<4; i++) sect4.push_back(0);	// add 4 zeros to use uInt32() in the very tail
	sect40 = subVect(sect40, 0, 4);					// just 4-byte header

	return bytes.size();
};

// ------------------------------------------------------------------------------------------

int BufrStruct::saveFile(string fName)
{
	size_t i;
	ofstream fout;

	fout.open(fName, ios::binary);
	if (fout.fail())
	{
		cerr << "Error code: " << errno << endl;
		return -1;
	}

	for (i = 0; i < sect0.size(); i++) fout << sect0[i];
	for (i = 0; i < sect1.size(); i++) fout << sect1[i];
	for (i = 0; i < sect3.size(); i++) fout << sect3[i];
	for (i = 0; i < sect40.size(); i++) fout << sect40[i];
	for (i = 0; i < sect4.size() - 4; i++) fout << sect4[i];
	for (i = 0; i < sect5.size(); i++) fout << sect5[i];

	fout.close();

	return sect0.size() + sect1.size() + sect3.size() + sect40.size() + sect4.size() - 4 + sect5.size();
};

// ------------------------------------------------------------------------------------------

long BufrStruct::getS4bitValue(int i, const long begin)
{
	int i0 = begin / 8;
	int jl = begin % 8;
	uint32_t mask = 0xfffffffful >> (32 - s[i].width);
	int m;

	uint32_t w = uInt32(sect4, i0);
	if (jl + s[i].width < 32)
	{
		m = 32 - s[i].width;
		w = (w << jl) >> m;
	}
	else
	{
		uint32_t v = uInt32(sect4, i0 + 1);
		m = (jl + s[i].width) % 8;
		w = (w << m) | (v >> (8 - m));
	}

	s[i].value = w & mask;
	s[i].dvalue = (static_cast<long>(s[i].value) + s[i].refValue)*pow(10.0, -s[i].scale);

	return begin + s[i].width;
};

long BufrStruct::getS4bitValue(int i)
{
	return getS4bitValue(i, s[i].begin);
};

// ------------------------------------------------------------------------------------------

void BufrStruct::setBitValue(const uint32_t w0, const long beg, const int width, VBytes& a)
{
	int i0 = beg / 8;
	int jl = beg % 8;
	uint32_t mask = 0xfffffffful >> (32 - width);
	uint8_t msk;
	uint32_t w, wMax;
	
	wMax = (uint32_t(0x01) << width) - 1;
	if (w0 > wMax) w = wMax;
	else w = w0;

	int m = width - (8 - jl);
	int i = 0;
	while (m >= 0)
	{
		msk = (mask >> m) & 0xff;
		a[i + i0] = (a[i + i0] & ~msk) | ((w >> m) & 0xff);
		m -= 8;
		i++;
	}
	if ((m < 0) && (m > -8))
	{
		msk = (mask << (-m)) & 0xff;
		a[i + i0] = (a[i + i0] & ~msk) | ((w << (-m)) & 0xff);
	}
};

long BufrStruct::setBitUValue(const bitField& w, const long beg, VBytes& a)
{
	setBitValue(w.value, beg, w.width, a);
	return beg + w.width;
};

long BufrStruct::setBitDValue(bitField& w, const long beg, VBytes& a)
{
	w.value = static_cast<uint32_t>(static_cast<long>(round(w.dvalue*pow(10.0, w.scale))) - w.refValue);
	return setBitUValue(w, beg, a);
};

long BufrStruct::setByteValue(const uint32_t v, const long bbeg, const int bwidth, VBytes& a)
{
	for (int i = 0; i < bwidth; i++)
	{
		a[bbeg + i] = (v >> 8 * (bwidth - i - 1)) & 0xFF;
	}
	return bbeg + bwidth;
};

// ------------------------------------------------------------------------------------------

uint32_t BufrStruct::getBitValue(const long beg, const int width, VBytes& a)
{
	int i0 = beg / 8;
	int jl = beg % 8;
	uint32_t mask = 0xfffffffful >> (32 - width);
	int m;

	uint32_t w = uInt32(a, i0);
	if (jl + width < 32)
	{
		m = 32 - width;
		w = (w << jl) >> m;
	}
	else
	{
		uint32_t v = uInt32(a, i0 + 1);
		m = (jl + width) % 8;
		w = (w << m) | (v >> (8 - m));
	}

	return w & mask;
};

long BufrStruct::getBitUValue(bitField& w, const long beg, VBytes& a)
{
	w.value = getBitValue(beg, w.width, a);
	return beg + w.width;
};

long BufrStruct::getBitDValue(bitField& w, const long beg, VBytes& a)
{
	w.value = getBitValue(beg, w.width, a);
	w.dvalue = (static_cast<long>(w.value) + w.refValue)*pow(10.0, -w.scale);
	return beg + w.width;
};

uint32_t BufrStruct::getByteValue(const long bbeg, const int bwidth, VBytes& a)
{
	uint32_t v = 0;
	uint32_t m = 1;
	for (int i = 0; i < bwidth; i++)
	{
		v += static_cast<uint32_t>(a[bbeg + bwidth - i - 1]) * m;
		m *= 256;
	}
	return v;
};


// ------------------------------------------------------------------------------------------

size_t BufrStruct::initSection1(const uint32_t x[])
{
	bitField uSect1[17] = {
		{ 0, 3 * 8, 0, 0, 22, 0 },	// [0]
		{ 3 * 8,     8, 0, 0,  0, 0 },
		{ 4 * 8, 2 * 8, 0, 0, 253, 0 },	// other center
		{ 6 * 8, 2 * 8, 0, 0,  0, 0 },
		{ 8 * 8,     8, 0, 0,  0, 0 },
		{ 9 * 8,     8, 0, 0,  0, 0 },	// [5]
		{ 10 * 8,     8, 0, 0,  3, 0 },
		{ 11 * 8,     8, 0, 0, 50, 0 },
		{ 12 * 8,     8, 0, 0, 14, 0 },
		{ 13 * 8,     8, 0, 0, 12, 0 },
		{ 14 * 8,     8, 0, 0,  0, 0 },	// [10]
		{ 15 * 8, 2 * 8, 0, 0, 2016, 0 },
		{ 17 * 8,     8, 0, 0,  1, 0 },
		{ 18 * 8,     8, 0, 0, 30, 0 },
		{ 19 * 8,     8, 0, 0, 23, 0 },
		{ 20 * 8,     8, 0, 0, 30, 0 },	// [15]
		{ 21 * 8,     8, 0, 0,  0, 0 }
	};

	sect1.resize(uSect1[0].value);
	for (int i = 0; i < 6; i++) uSect1[11 + i].value = x[i];		// year, month, day,  hour, minute, second
	for (int i = 0; i < 17; i++) setByteValue(uSect1[i].value, uSect1[i].begin / 8, uSect1[i].width / 8, sect1);
	return sect1.size();
};

size_t BufrStruct::initSection3()
{
	bitField uSect3[6] = {
		{ 0,     3 * 8, 0, 0, 10, 0 },
		{ 3 * 8,     8, 0, 0,  0, 0 },
		{ 4 * 8, 2 * 8, 0, 0,  1, 0 },
		{ 6 * 8,     8, 0, 0, 0x80, 0 },
		{ 7 * 8, 2 * 8, 0, 0, 0xCA1A, 0 },
		{ 9 * 8,     8, 0, 0,  0, 0 }
	};

	sect3.resize(uSect3[0].value);
	for (int i = 0; i < 6; i++) setByteValue(uSect3[i].value, uSect3[i].begin / 8, uSect3[i].width / 8, sect3);
	return sect3.size();
};

size_t BufrStruct::initSection05()
{
	bitField uSect0[6] = {
		{ 0,     8, 0, 0, 0x42, 0 },	// B
		{ 1 * 8, 8, 0, 0, 0x55, 0 },	// U
		{ 2 * 8, 8, 0, 0, 0x46, 0 },	// F
		{ 3 * 8, 8, 0, 0, 0x52, 0 },	// R
		{ 4 * 8, 3 * 8, 0, 0, 0, 0 },	// length of BUFR message
		{ 7 * 8, 8, 0, 0, 4, 0 }		// ver.4
	};
	sect0.resize(8);
	for (int i = 0; i < 6; i++) setByteValue(uSect0[i].value, uSect0[i].begin / 8, uSect0[i].width / 8, sect0);

	bitField uSect5[4] = {
		{ 0,     8, 0, 0, 0x37, 0 },	// 7
		{ 1 * 8, 8, 0, 0, 0x37, 0 },	// 7
		{ 2 * 8, 8, 0, 0, 0x37, 0 },	// 7
		{ 3 * 8, 8, 0, 0, 0x37, 0 }		// 7
	};
	sect5.resize(4);
	for (int i = 0; i < 4; i++) setByteValue(uSect5[i].value, uSect5[i].begin / 8, uSect5[i].width / 8, sect5);

	return sect0.size() + sect5.size();
};

void BufrStruct::setSection0BufrLength(uint32_t m)
{
	setByteValue(m, 4, 3, sect0);
};

void BufrStruct::setSection4Length(uint32_t m)
{
	setByteValue(m, 0, 3, sect40);
};

size_t BufrStruct::initSection4(const int n1, const int n2, const int n3)
{
	int i, m, n, jbeg, jlen;
	size_t len;

	N1 = n1;
	N2 = n2;
	N3 = n3;

	bitField uSect40[2] = {
		{ 0,     3 * 8, 0, 0, 4, 0 },
		{ 3 * 8,     8, 0, 0, 0, 0 }
	};

	// header of Section 4 -- sect40
	sect40.resize(uSect40[0].value);
	for (int i = 0; i < 2; i++) setByteValue(uSect40[i].value, uSect40[i].begin / 8, uSect40[i].width / 8, sect40);

	// Section 4
	jbeg = vecDescr[5].jbeg;

	// Step 1a (0)
	len = 0;
	for (i = 0; i < jbeg; i++) len += s[i].width;

	// add Step 1b
	m = 0;
	for (i = jbeg + 5; i < jbeg + 11; i++) m += s[i].width;
	n = 0;
	for (i = jbeg + 1; i < jbeg + 5; i++) n += s[i].width;
	len += s[jbeg].width + N1*(n + N0*m + s[jbeg + 11].width);

	// add Step 2a
	jbeg = vecDescr[6].jbeg;
	jlen = vecDescr[6].jlen;
	m = 0;
	for (i = jbeg + 1; i < jbeg + jlen; i++) m += s[i].width;
	len += s[jbeg].width + N2*m;

	// add Step 2b
	jbeg = vecDescr[7].jbeg;
	jlen = vecDescr[7].jlen;
	m = 0;
	for (i = jbeg + 1; i < jbeg + jlen; i++) m += s[i].width;
	len += s[jbeg].width + N3*m;

	// add Step 2c
	jbeg = vecDescr[8].jbeg;
	jlen = vecDescr[8].jlen;
	m = 0;
	for (i = jbeg; i < jbeg + jlen; i++) m += s[i].width;
	len += m;

	// convert to number of bytes
	len = len / 8;
	if (len % 8 != 0) len++;

	sect4.resize(len + 4);						// add 4 more bytes to the tail to process the last bits

	return sect40.size() + sect4.size() - 4;	// size of setc40 + sect4 (actial size of setc4 is 4 bytes more);
};

// ------------------------------------------------------------------------------------------

long BufrStruct::readS4block(const string blockName, double x[])
{
	long beg;

	vector<idxDescription>::iterator it = find_if(vecDescr.begin(), vecDescr.end(), [blockName](idxDescription const& a) { return a.descr == blockName; });
	
	int k = 0;
	for (int i = it->jbeg; i < it->jbeg + it->jlen; i++)
	{
		beg = getDValue(s[i], s[i].begin, x[k++]);
	}

	return beg;
};

long BufrStruct::readS4block(const string blockName, uint32_t x[])
{
	long beg;

	vector<idxDescription>::iterator it = find_if(vecDescr.begin(), vecDescr.end(), [blockName](idxDescription const& a) { return a.descr == blockName; });

	int k = 0;
	for (int i = it->jbeg; i < it->jbeg + it->jlen; i++)
	{
		beg = getUValue(s[i], s[i].begin, x[k++]);
	}

	return beg;
};

// ------------------------------------------------------------------------------------------

long BufrStruct::writeS4block(const string blockName, const double x[])
{
	long beg;

	vector<idxDescription>::iterator it = find_if(vecDescr.begin(), vecDescr.end(), [blockName](idxDescription const& a) { return a.descr == blockName; });

	for (int i = it->jbeg; i < it->jbeg + it->jlen; i++)
	{
		s[i].dvalue = x[i - it->jbeg];
		beg = setBitDValue(s[i], s[i].begin, sect4);
	}

	return beg;
};

long BufrStruct::writeS4block(const string blockName, const uint32_t x[])
{
	long beg;

	vector<idxDescription>::iterator it = find_if(vecDescr.begin(), vecDescr.end(), [blockName](idxDescription const& a) { return a.descr == blockName; });

	for (int i = it->jbeg; i < it->jbeg + it->jlen; i++)
	{
		s[i].value = x[i - it->jbeg];
		beg = setBitUValue(s[i], s[i].begin, sect4);
	}

	return beg;
};

// ------------------------------------------------------------------------------------------

long BufrStruct::writeS4blockABC(const long beg0, const int jv, const uint32_t N, const double x[])
{
	int jbeg = vecDescr[jv].jbeg;
	int jlen = vecDescr[jv].jlen;
	long beg = beg0;

	int k = 0;

	s[jbeg].value = N;
	beg = setBitUValue(s[jbeg], beg, sect4);
	for (uint32_t i = 0; i < N; i++)
	{
		for (int j = 1; j < jlen; j++)
		{
			beg = setDValue(s[jbeg + j], beg, x[k++]);
		}
	}

	return beg;
};


long BufrStruct::writeS4block1b(const double x[])
{
	int jbeg = vecDescr[5].jbeg;
	long beg = s[jbeg].begin;

	int k = 0;

	beg = setUValue(s[jbeg], beg, N1);
	for (uint32_t i = 0; i < N1; i++)
	{
		beg = setDValue(s[jbeg + 1], beg, x[k++]);		// Lat
		beg = setDValue(s[jbeg + 2], beg, x[k++]);		// Lon
		beg = setDValue(s[jbeg + 3], beg, x[k++]);		// Az
		beg = setUValue(s[jbeg + 4], beg, N0);			// N0
		for (uint32_t j = 0; j < N0; j++)
		{
			beg = setDValue(s[jbeg + 5], beg, x[k++]);	// Freq
			beg = setDValue(s[jbeg + 6], beg, x[k++]);	// IP
			beg = setDValue(s[jbeg + 7], beg, x[k++]);	// BA
			beg = setDValue(s[jbeg + 8], beg, x[k++]);	// stat1
			beg = setDValue(s[jbeg + 9], beg, x[k++]);	// BAerr
			beg = setDValue(s[jbeg + 10], beg, x[k++]);	// stat2
		}
		beg = setDValue(s[jbeg + 11], beg, x[k++]);		// perc.conf.
	}

	return beg;
};

long BufrStruct::writeS4block2a(const long beg0, const double x[])
{
	// must get 'beg' value from 'writeS4block1b()' 
	// [H, N, stat1, Nerr, stat2, perc.conf.]

	return writeS4blockABC(beg0, 6, N2, x);
};

long BufrStruct::writeS4block2b(const long beg0, const double x[])
{
	// must get 'beg' value from 'writeS4block1b() and writeS4block2a()' 
	// [gpH, P, T, SH, stat1, Perr, Terr, SHerr, stat2, perc.conf.]

	return writeS4blockABC(beg0, 7, N3, x);
};

long BufrStruct::writeS4block2c(const long beg0, const double x[])
{
	// must get 'beg' value from 'writeS4block1b() and writeS4block2a()' 
	// [vert.signif., surf.gpHeihgt, surf.P, stat1, surf.Perr, stat2, perc.conf.]

	int jbeg = vecDescr[8].jbeg;
	int jlen = vecDescr[8].jlen;
	long beg = beg0;

	int k = 0;
	for (int i = 0; i < jlen; i++)
	{
		beg = setDValue(s[jbeg + i], beg, x[k++]);
	}
	return beg;
};

// ------------------------------------------------------------------------------------------

long BufrStruct::readS4blockABC(const long beg0, const int jv, uint32_t& N, double x[])
{
	int jbeg = vecDescr[jv].jbeg;
	int jlen = vecDescr[jv].jlen;
	long beg = beg0;

	int k = 0;

	beg = getUValue(s[jbeg], beg, N);
	for (uint32_t i = 0; i < N; i++)
	{
		for (int j = 1; j < jlen; j++)
		{
			beg = getDValue(s[jbeg + j], beg, x[k++]);
		}
	}

	return beg;
};

long BufrStruct::readS4block1b(double x[], uint32_t& n1)
{
	// N1, [Lat, Lon, Az, N0, [Freq, IP, BA, stat1, BAerr, stat2]]

	int jbeg = vecDescr[5].jbeg;
	long beg;
	uint32_t n0;
	int k = 0;

	beg = getUValue(s[jbeg], s[jbeg].begin, N1);
	n1 = N1;
	for (uint32_t i = 0; i < N1; i++)
	{
		for (int m = 1; m <= 3; m++)
		{
			beg = getDValue(s[jbeg + m], beg, x[k++]);
		}
		beg = getUValue(s[jbeg + 4], beg, n0);			// N0
		x[k++] = n0;
		for (uint32_t j = 0; j < n0; j++)
		{
			for (int m = 5; m <= 10; m++)
			{
				beg = getDValue(s[jbeg + m], beg, x[k++]);
			}
		}
		beg = getDValue(s[jbeg + 11], beg, x[k++]);		// perc.conf.
	}

	return beg;
};

long BufrStruct::readS4block2a(const long beg0, double x[], uint32_t& n2)
{
	// must get 'beg' value from 'writeS4block1b()' 
	// [H, N, stat1, Nerr, stat2, perc.conf.]

	long beg = readS4blockABC(beg0, 6, N2, x);
	n2 = N2;
	return beg;
};

long BufrStruct::readS4block2b(const long beg0, double x[], uint32_t& n3)
{
	// must get 'beg' value from 'writeS4block1b() and writeS4block2a()' 
	// [gpH, P, T, SH, stat1, Perr, Terr, SHerr, stat2, perc.conf.]

	long beg = readS4blockABC(beg0, 7, N3, x);
	n3 = N3;
	return beg;
};

long BufrStruct::readS4block2c(const long beg0, double x[])
{
	// must get 'beg' value from 'writeS4block1b() and writeS4block2a()' 
	// [vert.signif., surf.gpHeihgt, surf.P, stat1, surf.Perr, stat2, perc.conf.]

	int jbeg = vecDescr[8].jbeg;
	int jlen = vecDescr[8].jlen;
	long beg = beg0;

	int k = 0;
	for (int i = 0; i < jlen; i++)
	{
		beg = getDValue(s[jbeg + i], beg, x[k++]);
	}
	return beg;
};

long BufrStruct::readS4block1a(double x[], uint32_t& m)
{
	uint32_t w[20];
	long beg = 0;

	m = 0;
	for (int i = 0; i < 3; i++)
	{
		beg = readS4block(vecDescr[i].descr, w);
		for (int j = 0; j < vecDescr[i].jlen; j++) x[m++] = w[j];
	}
	for (int i = 3; i < 5; i++)
	{
		beg = readS4block(vecDescr[i].descr, x + m);
		m += vecDescr[i].jlen;
	}

	return beg;
};

long BufrStruct::writeS4block1a(const uint32_t w[], const double x[])
{
	long beg = 0;

	int m = 0;
	for (int i = 0; i < 3; i++)
	{
		beg = writeS4block(vecDescr[i].descr, w + m);
		m += vecDescr[i].jlen;
	}
	m = 0;
	for (int i = 3; i < 5; i++)
	{
		beg = writeS4block(vecDescr[i].descr, x + m);
		m += vecDescr[i].jlen;
	}

	return beg;
}

long BufrStruct::writeS4block1a(const double x[])
{
	uint32_t w[40];

	int m = 0;
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < vecDescr[i].jlen; j++) w[m++] = static_cast<uint32_t>(x[m++]);
	}
	return writeS4block1a(w, x + m);
}