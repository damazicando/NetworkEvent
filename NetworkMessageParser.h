#ifndef _NetworkMessageParser_h_
#define _NetworkMessageParser_h_

#include "StringFormat.h"

class NetworkMessage
{
public:
	template<typename T, typename std::enable_if<std::is_integral<T>::value>::type>
	void set(int seq, const T &v)
	{
		T vmod = 0;
		vmod = ~vmod;
		vmod <<= 8;
		vmod = ~vmod;

		int iSize = sizeof(v);
		std::string s;
		for (int i = 0; i < iSize; i++)
		{
			uint8_t temp = (v >> ((iSize - 1 - i) * 8)) & vmod;
			s.append(1, temp);
		}

		vs_[seq] = s;
	}

	std::string encode()
	{
		std::string sDecoded;
		int iref = 1;
		for (auto itr = vs_.begin(); itr != vs_.end(); itr++,iref++)
		{
			if (itr->first != iref)
			{
				throw std::runtime_error(FormatI("missing the {0}th field", iref).c_str());
			}
			sDecoded += itr->second;
		}
	}
	
	template<T, std::enable_if<std::is_integral<T>::value>::type>
	T get(int seq)
	{

	}

	void encode(const std::string &)
	{

	}
private:
	std::map<int, std::string> vs_;
};

#endif
