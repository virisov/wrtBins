// wrtBufr.cpp : Defines the entry point for the console application.
//

//#pragma comment( lib, "../wrtBins/Debug/wrtBins.lib") 

#include <iostream>
#include <fstream>

#include "BufrStruct.h"
#include "asciiROfile.h"

/*
// for reference only, defined in BufrStruct.cpp
const int NDescr = 9;
const idxDescription idxDescr[NDescr] =
{
{  0,  5, "RO header" },
{  5,  7, "Time of occEvent" },
{ 12,  2, "RO summary quality" },
{ 14, 14, "LEO & GNSS POD" },
{ 28,  9, "Local Earth parameters" },
{ 37, 12, "RO Step1b data" },
{ 49,  7, "RO Step2a data" },
{ 56, 11, "RO Level2b data" },
{ 67,  7, "RO Level2c data" }
};
*/

const double      Pi = 3.141592653589793238;

inline double convertAzimuth2(const double az)
{
	return (az < 0 ? az * (180.0 / Pi) + 360.0 : az * (180.0 / Pi));
}
inline double convertAzimuth(const double az)
{
	return (az < 0 ? az + 360.0 : az);
}

const uint16_t ascendingOcc = 0x2000;
const uint16_t openLoop = 0x0100;
const uint16_t L2Cused = 0x0040;
const int MAX_X_SIZE = 50000;

int makeBufrRec(const string fNameInf, BufrStruct& bS)
{
	const uint16_t qualityFlag = 0;
	const int percConf = 100;
	const double minZConf = 400.0;	// m (below put conf. to 0)
	const int GNSSseries = 401;	// GPS

	double x[MAX_X_SIZE];
	vector<double> w;
	int i, j, k, N2, N3;
	long begBit;

	// ----------------------------------------------
	// load files

	infFile f1 = infFile(fNameInf);
	if (f1.size() == 0) return -1;
	if (!f1.sizeOK()) return -2;
	j = fNameInf.rfind(".inf");

	string fName = fNameInf.substr(0, j) + ".rni";
	tableFile f2 = tableFile(fName);
	N2 = f2.getNRows();
	if (N2 == 0 || f2.getError() != 0) return -3;

	fName = fNameInf.substr(0, j) + ".nn";
	tableFile f3 = tableFile(fName);
	N3 = f3.getNRows();
	if (N3 == 0 || f3.getError() != 0) return -4;

	// ----------------------------------------------
	// init BufrStruct sections	

	uint32_t dt[6];
	f1.occDateTime(dt);

	size_t s = 0;
	s += bS.initSection05();
	s += bS.initSection1(dt);
	s += bS.initSection3();
	size_t s4 = bS.initSection4(N2, N3, N3);
	bS.setSection0BufrLength(s + s4);
	bS.setSection4Length(s4);

	// ----------------------------------------------
	// write 1a data to bS structure (5 fixed length units)

//	const uint32_t z1[5] = { 998, 999, 255, 2, 1 };	// sat.ID, instr.ID, cent.ID, prod., proc.alg.
	const uint32_t z1[5] = { 1023, 530, 178, 2, 1 };	// sat.ID, instr.ID, cent.ID, prod., proc.alg.
	bS.writeS4block("RO header", z1);

	const uint32_t z2[7] = { 17, dt[0], dt[1], dt[2], dt[3], dt[4], dt[5] };
	bS.writeS4block("Time of occEvent", z2);

	const uint32_t z3[2] = { qualityFlag, percConf };
	bS.writeS4block("RO summary quality", z3);

	double z4[14];
	f1.occRx(&z4[0]);
	f1.occRv(&z4[3]);
	z4[6] = GNSSseries;
	z4[7] = f1.Tsatt();		// PRN
	f1.occTx(&z4[8]);
	f1.occTv(&z4[11]);
	for (int i = 0; i < 3; i++)
	{
		z4[i] *= 1e3;		// km -> m
		z4[i + 8] *= 1e3;
	}
	bS.writeS4block("LEO & GNSS POD", z4);

	double z5[9];
	z5[0] = f1.occTime();
	f1.coord(&z5[1]);		// lat, lon [deg]
	f1.LCC(&z5[3]);
	z5[6] = f1.LCR();
	for (int i = 3; i < 7; i++) z5[i] *= 1e3;	// km -> m
	f1.azim(&z5[7]);
	z5[7] = z5[8];
	z5[8] = f1.geoidUnd();
	bS.writeS4block("Local Earth parameters", z5);

	// ----------------------------------------------
	// write 1b data to bS structure 

	k = 0;
	for (i = 0; i < N2; i++)
	{
		w = f2.getRow(i);
		x[k++] = w[5];	// lat
		x[k++] = w[6];	// lon
		x[k++] = convertAzimuth(w[7]);	// azim (convert to deg, must be 0-360)
		x[k++] = 0;		// corrected freq.
		x[k++] = w[0] * 1e3 + z5[6];	// IP [m]
		x[k++] = w[1];	// BA [rad]
		x[k++] = 13;	// RMS statistics
		x[k++] = w[3];	// STD(BA)
		x[k++] = 63;
		x[k++] = (w[0] * 1e3 < minZConf ? 0 : percConf);
		if (k > MAX_X_SIZE - 10) return -5;
	}
	begBit = bS.writeS4block1b(x);

	// ----------------------------------------------
	// write 2a data to bS structure 

	k = 0;
	for (i = 0; i < N3; i++)
	{
		w = f3.getRow(i);
		x[k++] = w[0] * 1e3;	// height [m] 
		x[k++] = w[1];			// N
		x[k++] = 13;
		x[k++] = 0;				// STD(N)
		x[k++] = 63;
		x[k++] = (w[0]*1e3 < minZConf ? 0 : percConf);
		if (k > MAX_X_SIZE - 6) return -5;
	}
	begBit = bS.writeS4block2a(begBit, x);

	// ----------------------------------------------
	// write 2b data to bS structure 

	double h0, h1, P0, P1, Psurf;
	bool noPsurf = true;
	k = 0;
	for (i = 0; i < N3; i++)
	{
		w = f3.getRow(i);

		// extrapolation of P-profile to the surface (h=0)
		h1 = w[0] * 1e3;		// h [m]
		P1 = w[3] * 100;		// P [Pa]
		if (noPsurf && (i > 0) && (h1 > 0))
		{
			Psurf = exp( log(P0) - h0*(log(P1) - log(P0)) / (h1 - h0) );	// [Pa]
			noPsurf = false;
		}
		h0 = h1;
		P0 = P1;
		// end-of-extrapolation

		x[k++] = h1;			// height [m] 
		x[k++] = P1;			// P [Pa]
		x[k++] = w[2];			// T [K]
		x[k++] = 0;				// spec.humidity
		x[k++] = 13;
		x[k++] = 0;				// STD(P)
		x[k++] = 0;				// STD(T)
		x[k++] = 0;				// STD(SH)
		x[k++] = 63;
		x[k++] = (w[0] * 1e3 < minZConf ? 0 : percConf);
		if (k > MAX_X_SIZE - 9) return -5;
	}
	begBit = bS.writeS4block2b(begBit, x);

	// ----------------------------------------------
	// write 2c data to bS structure
	// we don't output Psurf because extrapolation of hydrostatic integration is inaccurate

	x[0] = 0;		// surface
	x[1] = z5[8];	// geoid undulation
	x[2] = 0;		// Psurf [Pa] (no value)
	x[3] = 13;
	x[4] = 0;		// STD(Psurf)
	x[5] = 63;
	x[6] = 0;		// perc.Conf.
	begBit = bS.writeS4block2c(begBit, x);

	// ----------------------------------------------

	return 0;
}

int main(int narg, char* sarg[])
{
	int iverb = 0;
	if (narg < 2 || narg > 3)
	{
		cout << "Usage: wrtBufr.exe INFfileName [-v]" << endl;
		return 1;
	}
	if (narg == 3)
	{
		string verb = string(sarg[2]);
		if (verb == "-v" || verb == "-V") iverb = 1;
	}

	string fNameInf = string(sarg[1]);

	cout << "Input file: " << fNameInf << endl;

	BufrStruct bS = BufrStruct{};

	int err = makeBufrRec(fNameInf, bS);
	if (err != 0)
	{
		cout << "makeBufrRec error: " << err << endl;
		return 2;
	}

	int j = fNameInf.rfind(".inf");
	string fNameOut = fNameInf.substr(0, j) + ".bufr";
	if(bS.saveFile(fNameOut) <= 0)
	{
		cout << "BufrStruct::saveFile() error" << endl;
		return 3;
	}

	cout << "Output file " << fNameOut << " is done." << endl;
    return 0;
}
