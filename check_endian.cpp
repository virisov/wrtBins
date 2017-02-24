// check_endian.cpp : Defines the entry point for the console application.
//

//#pragma comment( lib, "../wrtBins/Debug/wrtBins.lib") 

#include <iostream>
#include "Endian.h"

int main()
{
	if (little_endian())
		std::cout << "Little endian mashine." << std::endl;
	else
		std::cout << "Big endian mashine." << std::endl;

    return 0;
}

