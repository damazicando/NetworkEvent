#ifndef _StringUtil_h_
#define _StringUtil_h_

#include <sstream>
#include <iomanip>
#include <stdio.h>

std::string hexStr(const std::string &s)
{
	std::stringstream ss;
	ss << std::hex;
	for (int i = 0; i < (int)s.size(); ++i)
		ss << std::setw(2) << std::setfill('0') << (int)s[i];
	return ss.str();
}

#endif
