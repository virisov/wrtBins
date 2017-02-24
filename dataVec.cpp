#include <iostream>
#include <iterator>
#include <fstream>
#include <string>

#include "dataVec.h"
#include "Endian.h"

// -----------------------------------------------------------------------------------

size_t getFileSize(ifstream& f)
{
	streampos curr = f.tellg();

	// Get beginning location of files
	f.seekg(0, ios::beg);		// seek begining of file
	streampos beg = f.tellg();	// gets position
								// Get end location of files
	f.seekg(0, ios::end);		// seek end of file
	streampos end = f.tellg();	// gets position

								// Go back to the file
	f.seekg(curr, ios::beg);

	return end - beg;
}

// -----------------------------------------------------------------------------------


dataVec::dataVec(const string fName)
{
	LITTLE_ENDIAN1 = little_endian();	// determine the local mashine
	error = 0;

	// Open the file:
	ifstream f;					// Input file stream
	f.open(fName, ios::in | ios::binary);
	if (f.fail())
	{
		cerr << "Cannot open file " << fName << endl;
		error = 1;
		return;
	}

	size_t size = getFileSize(f);
	data.resize(size);
	f.read(reinterpret_cast<char*>(&data[0]), size);
	f.close();
}

dataVec::dataVec(const size_t m)
{
	LITTLE_ENDIAN1 = little_endian();	// determine the local mashine
	data.resize(m);
}

// -----------------------------------------------------------------------------------

long dataVec::saveFile(const string fName)
{
	ofstream fout;

	fout.open(fName, ios::binary);
	if (fout.fail())
	{
		cerr << "Cannot open file: " << errno << endl;
		error = 1;
		return -1;
	}

	for (size_t i = 0; i < data.size(); i++) fout << data[i];

	fout.close();

	return static_cast<long>(data.size());
}

// -----------------------------------------------------------------------------------

long dataVec::putBEm(uint64_t w, int m, long k)
{
	for (int i = 0; i < m; i++) data[k + i] = (w >> (8 * (m - i - 1))) & 0xff;

	return k + m;
}

long dataVec::putBE(double w, long k)
{
	// we use LITTLE_ENDIAN flag only with reinterpret_cast(double)
	// here we push double 'w' as big endian sequence of bytes
	double v = w;
	uint64_t* u = reinterpret_cast<uint64_t*>(&v);

	if(LITTLE_ENDIAN1) 
		for (int i = 0; i < 8; i++) data[k + i] = ((*u) >> (8 * (7 - i))) & 0xff;
	else
		for (int i = 0; i < 8; i++) data[k + i] = ((*u) >> (8 * i)) & 0xff;

	// double* p = reinterpret_cast<double*>(&data[k]);	// debug only

	return k + 8;
}

// -----------------------------------------------------------------------------------

long dataVec::getLEm(uint64_t& w, int m, long k)
{
	// here we retrieve m-byte 'w' from little endian sequence of bytes
	const uint64_t a[8] = { 
			0x1ul, 
			0x100ul, 
			0x10000ul, 
			0x1000000ul, 
			0x100000000ul, 
			0x10000000000ul, 
			0x1000000000000ul, 
			0x100000000000000ul };

	w = 0;
	for (int i = 0; i < m; i++)	w += data[k + i] * a[i];

	return k + m;
}

long dataVec::getLEms(int64_t& w, int m, long k)
{
	uint64_t v;
	long k1 = getLEm(v, m, k);
	w = static_cast<int64_t>(v << ((8 - m) * 8)) >> ((8 - m) * 8);
	return k1;
}


long dataVec::getLEd(double& w, long k)
{
	// we use LITTLE_ENDIAN flag only with reinterpret_cast(double)
	// here we retrieve double 'w' from little endian sequence of bytes
	double v = 0;
	uint64_t* u = reinterpret_cast<uint64_t*>(&v);

	if (LITTLE_ENDIAN1)
		for (int i = 0; i < 8; i++)	*u = ((*u) << 8) | data[k + 7 - i];
	else
		for (int i = 0; i < 8; i++)	*u = ((*u) << 8) | data[k + i];

	w = v;

	return k + 8;
}

// -----------------------------------------------------------------------------------
