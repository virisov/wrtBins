using namespace std;

#include "opnGNSS.h"
#include "Endian.h"

#include <iostream>
#include <fstream>
#include <cmath>

// ------------------------------------------------------------------------------------------

opnGNSS::opnGNSS(ROBinary& ro)
{
	vector<double>& T = ro.get_TIME_OFFS();							// time offset [s]
	size_t N = T.size();
	
	NUM_TAPS = ro.get_NUM_TAPS();
	
	const uint32_t WEEK_SEC = 604800;
	uint32_t GPSsec = ro.get_REF_WEEK()*WEEK_SEC + ro.get_REF_SOW();
	double REF_FOS = ro.get_REF_FOS();

	uint32_t gt0, gt1;
	vector<uint32_t> gt;	// block start time = GPSsec + gt[i]
	vector<size_t> nt;
	for (size_t i = 0; i < N; i++)
	{
		gt1 = static_cast<uint32_t>(floor(T[i]));
		if (i == 0 || gt1 > gt0)
		{
			gt.push_back(gt1);
			nt.push_back(i);
			gt0 = gt1;
		}
	}
	nt.push_back(N);
	size_t M = gt.size();
	size_t n = 0;
	for (size_t i = 0; i < M; i++) n += 12 + (nt[i + 1] - nt[i]) * (3 + 8 + (2 + 2)*NUM_TAPS);	// no low-rate data

	x = new dataVec(n + 340 + 9);		// see opnGns format description							

	long k = 0;
	for (size_t i = 0; i < M; i++)
	{
		uint16_t Nhr = static_cast<uint16_t>(nt[i + 1] - nt[i]);
		k = pushBlock(ro, GPSsec, gt[i], nt[i], Nhr, k);
		cout << i << " " << Nhr << " " << k << endl;
	}

	uint8_t PRN = ro.get_TX_ID();									// PRN
	int SIG_TYPE = ro.get_SIG_TYPE();								// 0 - GPS L1 CA, 1 - GPS L2 CL
	uint8_t TRACK_TYPE = static_cast<uint8_t>(ro.get_TRACK_TYPE());	// 0 - closed loop, 1 - open loop

	k = pushTrailer(PRN, SIG_TYPE, TRACK_TYPE, REF_FOS, k);
}

// ------------------------------------------------------------------------------------------

long opnGNSS::pushBlock(ROBinary& ro, uint32_t GPSsec, uint32_t gt1, size_t nt, uint16_t Nhr, long k)
{
	vector<double>& T = ro.get_TIME_OFFS();							// time offset [s]
	vector<double>& ADR = ro.get_ADR();								// accumulated Doppler range [cycles]
	vector<vector<IQtype>>& Ix = ro.get_I();						// I [3 taps]
	vector<vector<IQtype>>& Qx = ro.get_Q();						// Q [3 taps]

	k = x->putBE(GPSsec + gt1, k);

	uint8_t TRACK_TYPE = static_cast<uint8_t>(ro.get_TRACK_TYPE());	// 0 - closed loop, 1 - open loop
	k = x->putBE(TRACK_TYPE, k);

	k = x->putBE(static_cast<uint8_t>(ro.get_ANT_NUM()), k);		// antenna ID

	k = x->putBE(static_cast<uint8_t>('G'), k);						// GPS system

	uint16_t SVN = ro.get_TX_ID();									// PRN
	k = x->putBE(SVN, k);

	uint8_t PRN = SVN & 0xff;										// PRN
	k = x->putBE(PRN, k);

	//uint16_t Nhr = static_cast<uint16_t>(T.size());
	k = x->putBE(Nhr, k);											// N high rate								

	//double lrObs = 0;												// dummy low-rate data
	//k = x->putBE(lrObs, k);										// if we push it we need to modify obsType[16] in pushTrailer

	for (size_t i = 0; i < Nhr; i++) k = pushHRdata(T[i+nt] - gt1, ADR[i+nt], Ix[i+nt], Qx[i+nt], k);

	return k;
}

// ------------------------------------------------------------------------------------------

long opnGNSS::pushHRdata(TIMEtype t, TIMEtype adr, vector<IQtype>& I3, vector<IQtype>& Q3, long k)
{
	uint32_t ut = static_cast<uint32_t>(t*1e7);	// 1s -> 0.1us

	k = x->putBE24(ut, k);
	k = x->putBE(adr, k);

	for (size_t i = 0; i < NUM_TAPS; i++)
	{
		k = x->putBE(static_cast<uint16_t>(I3[i]), k);
		k = x->putBE(static_cast<uint16_t>(Q3[i]), k);
	}

	return k;
}

// ------------------------------------------------------------------------------------------

uint16_t opnGNSS::makeObsType(char obsType, uint16_t freqBand, char att, char metaData)
{
	uint16_t jO, jA, jM;

	// freqBand = {0 or 1} (+1 to interpret)
	// Obs.type:
	// 0 -- C (code pseudo-range)
	// l -- L (phase) (STRATOS)
	// 2 -- D (Doppler)
	// 3 -- S (SNR)
	// att: 'C' for L1C(M) or L


	switch(obsType)
	{
	case 'C': jO = 0; break;
	case 'L': jO = 1; break;
	case 'D': jO = 2; break;
	case 'S': jO = 3; break;
	default: jO = 0;
	}

	if (att == 0) jA = 0;
	else jA = att - 'A' + 1;

	if (metaData == 0) jM = 0;
	else jM = metaData - 'A' + 1;

	return ((jO << 13) & 0xE000) | ((freqBand << 10) & 0x1C00) | ((jA << 5) & 0x03E0) | (jM & 0x001F);
}

// ------------------------------------------------------------------------------------------

long opnGNSS::pushTrailer(int PRN, int SIG_TYPE, int TRACK_TYPE, double REF_FOS, long k)
{
	//	closed loop: 'L#C ', 'L#I ', 'L#I ', 'L#I ', 'L#Q ', 'L#Q ', 'L#Q '
	//	open   loop: 'L#CM', 'L#I ', 'L#I ', 'L#I ', 'L#Q ', 'L#Q ', 'L#Q '
	uint16_t hrType, lrType;
	char metaData[5];
	int i, m, i0;

	const char CODE[2] = { 'C','L' };	// L1C or L2L

	metaData[0] = 0;
	metaData[1] = 'M' - 'A' + 1;		// Model
	metaData[2] = 'E' - 'A' + 1;		// Early
	metaData[3] = 'P' - 'A' + 1;		// Prompt
	metaData[4] = 'L' - 'A' + 1;		// Late

	// ---------------------------------------------------------------------
	// if we push low rate obs. data we need to put its type here

	lrType = 0;
	for (i = 0; i < 16; i++) k = x->putBE(lrType, k);

	// ---------------------------------------------------------------------

//	hrType = makeObsType('L',FREQ_BAND,'C',metaData[TRACK_TYPE]);	// L1C or L1CM for Model (open loop) (L1C)
	hrType = makeObsType('L',SIG_TYPE,CODE[SIG_TYPE],metaData[TRACK_TYPE]);	// best for Accum.Doppler Range
	k = x->putBE(hrType, k);
	k = x->putBE(static_cast<uint8_t>(TIMEtype_char), k);			// double
	i0 = 1;

	if (NUM_TAPS == 3) m = 2;
	else m = 3;

	for(i = 0; i < NUM_TAPS; i++)
	{
		hrType = makeObsType('L', SIG_TYPE, 'I', metaData[i + m]);	// L(1,2)I (Early, Prompt, Late)
		k = x->putBE(hrType, k);									// uint16_t
		k = x->putBE(static_cast<uint8_t>(IQtype_char), k);			// unsigned short
		i0++;

		hrType = makeObsType('L', SIG_TYPE, 'Q', metaData[i + m]);	// L(1,2)Q (Early, Prompt, Late)
		k = x->putBE(hrType, k);									// uint16_t
		k = x->putBE(static_cast<uint8_t>(IQtype_char), k);			// unsigned short
		i0++;
	}

	for (i = i0; i < 16; i++)
	{
		k = x->putBE(static_cast<uint16_t>(0), k);
		k = x->putBE(static_cast<uint8_t>(0), k);
	}

	// ---------------------------------------------------------------------

	int32_t hrDataOffset = static_cast<int32_t>(REF_FOS*1e9);		// sec -> nsec
	k = x->putBE(static_cast<uint32_t>(hrDataOffset), k);

	// ---------------------------------------------------------------------

	uint64_t byteOffset1 = 0xffffffffffffffff;
	uint64_t byteOffset0 = 0;
	for (i = 0; i < 32; i++)
	{
		if (i + 1 == PRN) k = x->putBE(byteOffset0, k);
		else k = x->putBE(byteOffset1, k);
	}

	// ---------------------------------------------------------------------

	uint8_t version = 1;
	k = x->putBE(version, k);
	k = x->putBE(static_cast<uint8_t>('G'), k);						// GPS
	for (i = 0; i < 7; i++) k = x->putBE(static_cast<uint8_t>(' '), k);	// none

	// ---------------------------------------------------------------------

	return k;
}

// ------------------------------------------------------------------------------------------

