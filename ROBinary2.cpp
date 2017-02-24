using namespace std;

#include "ROBinary2.h"

#include <iostream>
#include <fstream>
#include <math.h>

const double Pi = 3.141592653589793238;    // Pi = Circle length / Circle diameter 

void ROBinary::streamOutput(ostream& f)
{
	f << "input file = " << fNameIn << endl;
	f << "VSTRING    = " << get_VSTRING() << endl;
	f << "REF_WEEK   = " << get_REF_WEEK() << endl;
	f << "REF_SOW    = " << get_REF_SOW() << endl;
	f << "REF_FOS    = " << get_REF_FOS() << endl;
	f << "REF_PRANGE = " << get_REF_PRANGE() << endl;
	f << "TX_ID      = " << get_TX_ID() << endl;
	f << "SIG_TYPE   = " << get_SIG_TYPE() << endl;
	f << "ANT_NUM    = " << get_ANT_NUM() << endl;
	f << "TRACK_TYPE = " << get_TRACK_TYPE() << endl;
	f << "SAMP_FREQ  = " << get_SAMP_FREQ() << endl;
	f << "NUM_TAPS   = " << get_NUM_TAPS() << endl;
}

ROBinary::ROBinary(const string fName, int verbose)
{
	// Info:
	//   Instantiation of ROBinary class.
	//   Here, we read 'myfile'  according to the specification
	//   listed at "STRATOS v1.0 Instrument Overview & Files 
	//   Format Specification." (https://goo.gl/rqPe5i),
	//   under heading "Radio Occultation (RO) file."
	//

	fNameIn = fName;
	data = new dataVec(fNameIn);
	error = data->getError();
	if (getError() != 0)
	{
		cerr << "Cannot open file " << fNameIn << endl;
		Nobs = Ntaps = 0;
		return;
	}

	size_t size = data->getSize();

	Ntaps = get_NUM_TAPS();
	bool AmpPhase;
	if (get_VSTRING() == string("PPRX_OBS_1_00_0"))
	{
		Nobs = static_cast<size_t>((size - 64) / (11 + 4 * Ntaps));
		AmpPhase = false;		// I and Q are written in the file
	}
	else
	{
		Nobs = static_cast<size_t>((size - 64) / (11 + 3 * Ntaps));
		AmpPhase = true;		// amplitude sqrt(I^2 + Q^2) and phase atan2(Q,I)/(2*pi) are written in the file
	}
	if (verbose)
	{
		cout << "File size: " << size << " bytes" << endl;
		cout << "Number of samples: " << Nobs << endl;
		streamOutput(cout);
	}

	// Adjust vectors/matricies based on NOBS and NUM_TAPS_:

	TIME_OFFS.resize(Nobs);
	ADR.resize(Nobs);
	I.resize(Nobs, vector<IQtype>(Ntaps,0));
	Q.resize(Nobs, vector<IQtype>(Ntaps,0));

	// coefficients for TIME_OFFS and ADR

	const double a38 = pow(2, -38);
	const double a8 = pow(2, -8);
	uint64_t w;
	int64_t ws;
	long k = 64;
	double amp, phase;
	for (size_t i = 0; i < Nobs; i++)
	{
		k = data->getLEm(w, 6, k);		// unsigned 6-byte
		TIME_OFFS[i] = w * a38;
		k = data->getLEms(ws, 5, k);	// signed 5-byte
		ADR[i] = ws * a8;
//		k = data->getLEm(w, 5, k);		// unsigned 5-byte
//		ADR[i] = w * a8;
		for (size_t j = 0; j < Ntaps; j++)
		{
			if (AmpPhase)
			{
				k = data->getLEm(w, 2, k);		// unsigned 2-byte
				amp = w;
				k = data->getLEm(w, 1, k);		// unsigned 1-byte
				phase = w * 2 * Pi / 256.0;
				I[i][j] = static_cast<IQtype>(amp*cos(phase));
				Q[i][j] = static_cast<IQtype>(amp*sin(phase));
			}
			else
			{
				k = data->getLEm(w, 2, k);		// signed 2-byte
				I[i][j] = static_cast<IQtype>(w);
				k = data->getLEm(w, 2, k);		// signed 2-byte
				Q[i][j] = static_cast<IQtype>(w);
			}
		}
	}
}

string ROBinary::get_VSTRING()
{
	char s[16];
	for (int i = 0; i < 16; i++) s[i] = (*data)[i];
	return string(s);
}


void ROBinary::save_ascii(const string outfile) 
{
	if (getError() != 0)
	{
		cerr << "There is no valid data to output" << endl;
		Nobs = Ntaps = 0;
		return;
	}

	ofstream f;
	
	f.open(outfile, ios::out);
	if (f.fail())
	{
		cerr << "Cannot open output file " << outfile << endl;
		error = 1;
		return;
	}

	streamOutput(f);
		
	for (size_t i = 0; i < Nobs; i++) 
	{
		f << "TIME_OFFS   = " << TIME_OFFS[i] << endl;
		f << "ADR         = " << ADR[i]       << endl;

		f << "I           = ";
		for (size_t j = 0; j < Ntaps; j++) f << I[i][j] << " " ;
		f << endl;

		f << "Q           = ";
		for (size_t j = 0; j < Ntaps; j++) f << Q[i][j] << " ";
		f << endl;
	}

	f.close();
	return;
}

//
// References
//  [1] http://stackoverflow.com/questions/1673445/how-to-convert-unsigned-char-to-stdstring-in-c
//	[2] http://stackoverflow.com/questions/1301353/converting-byte-array-to-double-c
//
//
//  sign extension: http://stackoverflow.com/questions/7496657/when-printing-hex-values-using-x-why-is-ffffff-printed-after-each-value
//  
//
//	http://stackoverflow.com/questions/9694838/how-to-implement-2d-vector-array
//
