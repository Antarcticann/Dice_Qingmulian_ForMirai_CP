#include <string>
#include <algorithm>
#include <windows.h>
#include "UpperToLowerByCD.h"

#ifdef DEBUG
std::string tolower_inclideChinese(std::string s)
{
	std::string outPut;
	for (int i = 0; i != s.length(); i++)
	{
		if (s[i] > 0x80 || s[i] < 0)
		{
			char out[3] = { s[i], s[i + 1],'\0' };
			outPut += out;
			i++;
		}
		else
			outPut += tolower(s[i]);
	}
	return outPut;
}
#endif // 奇妙的bug增加了，特殊字节如“龙舌兰”被处理后存入map时会出bug

std::string CDs_tolower(std::string sInput)
{
	std::string s = sInput;
	int len = s.size();
	for (int i = 0; i < len; i++) {
		if (s[i] >= 'A' && s[i] <= 'Z') {
			s[i] += 32;
		}
	}
	return s;
}