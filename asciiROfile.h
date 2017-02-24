#pragma once

#include <vector>
#include <string>

using namespace std;

typedef vector<double> vecD;

const double BAD_VALUE = -999.0;
const int MAX_COLS = 256;
const char COMMENT_CHAR = '#';

class tableFile
{
private:
	int cols;
	int rows;
	vector<vecD> data;
	int error = 0;

public:
	tableFile(vector<vecD>& x);
	tableFile(const string fName, const int skip = 0);
	vector<double> getColumn(const int col);
	vector<double> getRow(const int row) { return data[row]; }
	double getElement(int col, int row) { return data[row][col]; }
	int getNCols() { return cols; }
	int getNRows() { return rows; }
	int getError() { return error; }
	
	int saveFile(const string fName);
};

int saveTableFile(const string fName, double x[], int rows, int cols);

class infFile
{
private:
	vector<string> content;

public:
	infFile(const string fName);
	int size() { return content.size();  }
	bool sizeOK() { return size() >= 24; }

	string version() { return content[0]; }
	string procTime() { return content[1]; }

	bool occDateTime(uint32_t x[]);
	string mission() { return content[4]; }
	int Tsatt() { return stoi(content[5]); }
	int Rsatt() { return stoi(content[6]); }
	string calibSourse() { return content[7]; }

	bool coord(double x[]);
	bool LCC(double x[]) { return sscanf(content[9].c_str(), "%lg %lg %lg", &x[0], &x[1], &x[2]) == 3; }
	double LCR() { return stod(content[10]); }
	double geoidUnd() { return stod(content[11]); }
	bool setting() { return content[12] == "setting"; }
	bool azim(double x[]) {
		x[0] = stod(content[13]); 
		x[1] = stod(content[14]); 
		return x[0]!=0 && x[1]!=0;
	}

	double occTime() { return stod(content[15]); }
	bool occTx(double x[]) { return sscanf(content[16].c_str(), "%lg %lg %lg", &x[0], &x[1], &x[2]) == 3; }
	bool occRx(double x[]) { return sscanf(content[17].c_str(), "%lg %lg %lg", &x[0], &x[1], &x[2]) == 3; }
	bool occTv(double x[]) { return sscanf(content[18].c_str(), "%lg %lg %lg", &x[0], &x[1], &x[2]) == 3; }
	bool occRv(double x[]) { return sscanf(content[19].c_str(), "%lg %lg %lg", &x[0], &x[1], &x[2]) == 3; }

	bool badness(double x[]) {
		x[0] = stod(content[20]);
		x[1] = stod(content[21]);
		x[2] = stod(content[22]);
		return x[0] != 0 && x[1] != 0 && x[2]!=0;
	}
	bool profOK() { return content[23] == "OK"; }
};

string replace_all_substr(
	string pathname,			// <-- Full file name 
	const string& search,		// <-- substring to find
	const string& replace);		// <-- substring to replace
								// --> Filename with different substring

