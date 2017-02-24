#include <stdint.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "asciiROfile.h"

tableFile::tableFile(vector<vecD>& x)
{
	data = x;
	rows = data.size();
	cols = data[0].size();
}


tableFile::tableFile(const string fName, const int skip)
{
	string line, dump;

	error = 0;

	ifstream infile(fName, ios::in);
	if (!infile.is_open()) {
		rows = 0;
		cols = 0;
		error = 1;
		return;
	}

	if (skip > 0)
		for (int i = 0; i < skip; i++) getline(infile,line);

	vector<double> a(MAX_COLS);
	bool firstLine = true;
	rows = 0;
	while (!infile.eof())
	{
		getline(infile,line);
		if (line[0] == COMMENT_CHAR) continue;
		if (infile.eof()) break;
		istringstream iss(line);
		if (firstLine)
		{
			for (cols = 0; !iss.eof() && cols < MAX_COLS; cols++)
			{
				iss >> a[cols];
				if (iss.fail())
				{
					iss.clear();
					iss >> dump;
					a[cols] = BAD_VALUE;
					error = 2;
					continue;
				}
			}
			firstLine = false;
			a.resize(cols);
		}
		else
		{
			fill_n(a.begin(), cols, BAD_VALUE);
			for (int i = 0; i < cols && !iss.eof(); i++)
			{
				iss >> a[i];
				if (iss.fail())
				{
					iss.clear();
					iss >> dump;
					a[i] = BAD_VALUE;
					error = 2;
					continue;
				}
			}
		}

		if (any_of(a.begin(), a.end(), [](double x) { return x != BAD_VALUE; }))
		{
			data.push_back(a);
			rows++;
		}
	}
	data.resize(rows);

	infile.close();
}

vector<double> tableFile::getColumn(const int col)
{
	vector<double> c(0);

	if (col >= cols || col < 0) return c;

	c.reserve(rows);
	for (auto x : data) c.push_back(x[col]);
	return c;
}

int tableFile::saveFile(const string fName)
{
	ofstream outfile(fName, ios::out);
	if (outfile.fail()) {
		cerr << "Error: output file " << fName << endl;
		error = 1;
		return -1;
	}

	size_t rows = data.size();
	size_t cols = data[0].size();

	for (size_t i = 0; i < rows; i++)
	{
		for (size_t j = 0; j < cols; j++) outfile << data[i][j] << " ";
		outfile << endl;
	}
	outfile.close();

	return rows*cols;
}

// ----------------------------------------------------------------------
// not a member of a class

int saveTableFile(const string fName, double x[], int rows, int cols)
{
	ofstream outfile(fName, ios::out);
	if (outfile.fail()) {
		cerr << "Error: output file " << fName << endl;
		return -1;
	}

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++) outfile << " " << x[cols * i + j];
		outfile << endl;
	}
	outfile.close();

	return rows*cols;
}

// ----------------------------------------------------------------------

infFile::infFile(const string fName)
{
	string line;

	ifstream infile(fName, ios::in);
	if (infile.fail()) {
		content.resize(0);
		return;
	}

	while (!infile.eof())
	{
		getline(infile, line);
		if (line.find_first_of("#") != string::npos) continue;
		line = line.substr(0, line.find_first_of("!"));
		content.push_back(line);
//		cout << line << endl;
	}
}

bool infFile::coord(double x[])
{
	double sx[2] = { 1.0, 1.0 };
	string line = content[8];
	int pos = line.find("N", 0);
	if (pos == string::npos)
	{
		pos = line.find("S", 0);
		sx[0] = -1.0;
	}
	line.erase(pos, 1);
	pos = line.find("E", 0);
	if (pos == string::npos)
	{
		pos = line.find("W", 0);
		sx[1] = -1.0;
	}
	line.erase(pos, 1);
	bool ok = sscanf(line.c_str(), "%lg %lg", &x[0], &x[1]) == 2;
	x[0] *= sx[0];
	x[1] *= sx[1];
	return ok;
}

bool infFile::occDateTime(uint32_t x[])
{
	string line = content[2];
	replace(line.begin(), line.end(), '/', ' ');
	int k = sscanf(line.c_str(), "%d %d %d", &x[0], &x[1], &x[2]);
	line = content[3];
	replace(line.begin(), line.end(), ':', ' ');
	replace(line.begin(), line.end(), '.', ' ');
	k += sscanf(line.c_str(), "%d %d %d", &x[3], &x[4], &x[5]);

	return k==6;
}

string replace_all_substr(
	string pathname,			// <-- Full file name 
	const string& search,		// <-- substring to find
	const string& replace)		// <-- substring to replace
								// --> Filename with different substring
{
	size_t pos = 0;
	while ((pos = pathname.find(search, pos)) != string::npos)
	{
		pathname.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return pathname;
}


