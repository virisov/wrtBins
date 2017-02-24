// wrtGns.cpp : Defines the entry point for the console application.
//

//#pragma comment( lib, "../wrtBins/Debug/wrtBins.lib") 

#include <string>
#include <iostream>

#include "dataVec.h"
#include "ROBinary2.h"
#include "opnGNSS.h"

using namespace std;

int main(int narg, char* sarg[])
{
	int iverb = 0;

	if (narg < 2 || narg > 3)
	{
		cout << "Usage: wrtGns.exe fileName [-v]" << endl;
		return 1;
	}
	if (narg == 3)
	{
		string verb = string(sarg[2]);
		if (verb == "-v" || verb == "-V") iverb = 1;
	}

	string fName = string(sarg[1]);

	// cout << "Input file: " << fName << endl;

	ROBinary ro = ROBinary(fName, iverb);	// data in a file are saved as little endian
	if (ro.getError())
	{
		cout << "ROBinary error" << endl;
		return 2;
	}

	opnGNSS og = opnGNSS(ro);				// data in opnGNSS structure are saved as big endian

	if (og.saveFile(fName + ".opnGns") == -1)
	{
		cout << "opnGNSS::saveFile() error" << endl;
		return 3;
	}

	cout << "File " << fName + ".opnGns" << " is saved." << endl;

	return 0;
}
