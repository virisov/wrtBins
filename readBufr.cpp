// readBufr.cpp : reads binary BUFR file and saves values in ASCII tables.
//

//#pragma comment( lib, "../wrtBins/Debug/wrtBins.lib") 

#include <iostream>
#include <fstream>

#include "BufrStruct.h"
#include "asciiROfile.h"

/*
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

const int MAX_X_SIZE = 50000;

int main(int narg, char* sarg[])
{
	long k;
	double x[MAX_X_SIZE];
	uint32_t m;

	int iverb = 0;
	if (narg < 2 || narg > 3)
	{
		cout << "Usage: wrtBufr.exe fileName [-v]" << endl;
		return 1;
	}
	if (narg == 3)
	{
		string verb = string(sarg[2]);
		if (verb == "-v" || verb == "-V") iverb = 1;
	}

	string fName = string(sarg[1]);

	cout << "Input file: " << fName << endl;

	BufrStruct bS = BufrStruct{};
	
	if(bS.loadFile(fName) <=0)
	{
		cout << "BufrStruct::loadFile() error." << endl;
		return 1;
	}

	k = bS.readS4block1a(x, m);
	if(saveTableFile(fName + "_1a.txt", x, m, 1) <= 0) return 2;

	k = bS.readS4block1b(x, m);
	if(saveTableFile(fName + "_1b.txt", x, m, 11) <= 0) return 2;

	k = bS.readS4block2a(k, x, m);
	if(saveTableFile(fName + "_2a.txt", x, m, 6) <= 0) return 2;

	k = bS.readS4block2b(k, x, m);
	if(saveTableFile(fName + "_2b.txt", x, m, 10) <= 0) return 2;

	k = bS.readS4block2c(k, x);
	if(saveTableFile(fName + "_2c.txt", x, 7, 1) <= 0) return 2;

    return 0;
}
