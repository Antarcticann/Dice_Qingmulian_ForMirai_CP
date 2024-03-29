#include <string>
#include <iostream>
#include <map>
#include <random>
#include <chrono>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include "DataBase.h"
#include "RandomCore.h"
#include "Calculator.h"
#include "SpecialFunctionMap.h"

using namespace std;

DiceCalculatorOutput SimpleDice(unsigned int A, unsigned int B)
{
	int intResult = 0, intDiceEnum;
	string strDetail;
	if (A <= 0 || A >= 1000 || B <= 0 || B > 9999)
	{
		return { "算式格式错误",0,false };
	}
	for (int i = 1; i <= A; i++)
	{
		intDiceEnum = RandomGenerator::Randint(1, B);
		strDetail = strDetail + to_string(intDiceEnum) + " ";
		intResult += intDiceEnum;
	}
	strDetail.erase(strDetail.end() - 1);
	return { strDetail,intResult,true };
}

DiceCalculatorOutput DiceCalculator(string inputEquation, int intDice)
{
	string strEquation = inputEquation;
	//检查括号是否成对，使用是否正确——————————————————————

	int intMsgCnt = 0;
	int NestCount = 0;
	while (intMsgCnt != strEquation.length())
	{
		if (static_cast<unsigned char>(strEquation[intMsgCnt]) == '(')
			NestCount++;
		if (static_cast<unsigned char>(strEquation[intMsgCnt]) == ')')
			NestCount--;
		if (NestCount < 0)
		{
			return{ "括号用错了，这没法算嘛",0,false };
		}
		intMsgCnt++;
	}
	if (NestCount)
	{
		return { "括号不完整，没法算的",0,false };
	}


	string strDetail = strEquation;
	while ((strEquation.find('+') != string::npos || strEquation.find('-') != string::npos || strEquation.find(')') != string::npos
		|| strEquation.find('*') != string::npos || strEquation.find('d') != string::npos) && strEquation.length() != 0)
	{
		//判断一下是不是“-1”这种格式的算式
		if (strEquation.length() >= 2
			&& (static_cast<unsigned char>(strEquation[0]) == '+' || static_cast<unsigned char>(strEquation[0]) == '-') &&
			(strEquation.substr(1).find('+') == string::npos && strEquation.substr(1).find('-') == string::npos
				&& strEquation.substr(1).find('*') == string::npos && strEquation.substr(1).find('d') == string::npos))
		{
			if (strDetail.length() != 0)
			{
				return{ strDetail,stoi(strEquation),true };
			}
			else
			{
				return{ strEquation,stoi(strEquation),true };
			}
		}
		//如果括号存在则提取最内层括号内容——————————————————————————————
		string strBehind;
		string strFront;
		string strSecEquation;
		if (strEquation.find(')') != string::npos)
		{
			strBehind = strEquation.substr(strEquation.find(')'));//提取括号后的部分，开头带被提取的括号
			int intEquationCnt = strEquation.find(')') - 1;
			while (intEquationCnt != 0 && static_cast<unsigned char>(strEquation[intEquationCnt]) != '(')
			{
				strSecEquation = strEquation[intEquationCnt] + strSecEquation;
				--intEquationCnt;
			}
			strFront = strEquation.substr(0, intEquationCnt + 1);//提取括号前的部分，结尾带被提取的括号
		}
		else
		{
			strSecEquation = strEquation;
		}
		//判断一下是否括号为空
		if (strSecEquation.length() == 0)
		{
			return{ "没有找到算式或者有空的括号，但费用我就收下啦",0,false };
		}

		if (strSecEquation.length())//这一部分负责计算无括号的算式
		{
			//计算无括号子式————————————————————————————————
			//计算掷骰运算符d
			string strSecDetail = strSecEquation;
			bool boolDiced = false;//判断是否需要记录d运算符的Detail
			while (strSecEquation.find('d') != string::npos)
			{
				boolDiced = true;
				int DPos = 0;//记录一下该从d字符前面第几位开始replace
				string strDiceA, strDiceB;//d前后的两个数
				int intDiceACut = strSecEquation.find('d') - 1, intDiceBCut = strSecEquation.find('d') + 1;
				while (intDiceACut >= 0 && isdigit(static_cast<unsigned char>(strSecEquation[intDiceACut])))
				{
					strDiceA = strSecEquation[intDiceACut] + strDiceA;
					DPos++;
					intDiceACut--;
				}//此时intDiceACut在d算式（包含前后数字）的前面一位
				while (intDiceBCut != strSecEquation.length() && isdigit(static_cast<unsigned char>(strSecEquation[intDiceBCut])))
				{
					strDiceB = strDiceB + strSecEquation[intDiceBCut];
					intDiceBCut++;
				}//此时intDiceBCut在d算式（包含前后数字）的后面一位
				int DLength = intDiceBCut - intDiceACut - 1;//记录一下d算式（包含前后数字）的长度
				if (intDiceBCut != strSecEquation.length() && static_cast<unsigned char>(strSecEquation[intDiceBCut]) == 'd')
				{
					return{ "算式格式错误，但费用不退哦",0,false };
				}
				int intA, intB, intDiceResult = 0;
				intA = strDiceA.length() ? stoi(strDiceA) : 1;
				intB = strDiceB.length() ? stoi(strDiceB) : intDice;
				if (intA >= 100 || intB >= 10000)
				{
					return{ "最高只能99d9999哦！再多了就算不出来了",0,false };
				}
				if (intB == 0)
				{
					return{ GlobalMsg[EnumErrorMsg.骰子面数为0],0,false };
				}
				if (intA == 0)
				{
					return{ GlobalMsg[EnumErrorMsg.掷骰轮数无效],0,false };
				}
				if (intA <= 10 && intA != 1)
				{
					string diceRecord;
					for (int diceCut = 0; diceCut < intA; diceCut++)
					{
						int enumResult = RandomGenerator::Randint(1, intB);
						intDiceResult += enumResult;
						diceRecord += to_string(enumResult) + ",";
					}
					diceRecord.erase(diceRecord.length() - 1);
					strSecEquation = strSecEquation.replace(intDiceACut + 1, intDiceBCut - intDiceACut - 1, to_string(intDiceResult));
					strSecDetail = strSecDetail.replace(strSecDetail.find('d') - DPos, DLength, '(' + diceRecord + ')');

				}
				else
				{
					for (int diceCut = 0; diceCut < intA; diceCut++)
					{
						intDiceResult += RandomGenerator::Randint(1, intB);
					}
					strSecEquation = strSecEquation.replace(intDiceACut + 1, intDiceBCut - intDiceACut - 1, to_string(intDiceResult));
					strSecDetail = strSecDetail.replace(strSecDetail.find('d') - DPos, DLength, to_string(intDiceResult));
				}
			}
			if (boolDiced)
			{
				strDetail = strDetail + '=' + strFront + strSecDetail + strBehind;
			}
			//计算乘法运算符*——————————————————————————————
			while (strSecEquation.find('*') != string::npos)
			{
				string strMultA, strMultB;
				int intMultACut = strSecEquation.find('*') - 1, intMultBCut = strSecEquation.find('*') + 1;
				while (intMultACut >= 0 && isdigit(static_cast<unsigned char>(strSecEquation[intMultACut])))
				{
					strMultA = strSecEquation[intMultACut] + strMultA;
					intMultACut--;
				}
				if (!strMultA.length())
				{
					return{ "算式格式错误，但费用不退哦",0,false };
				}
				while (intMultBCut != strSecEquation.length() && isdigit(static_cast<unsigned char>(strSecEquation[intMultBCut])))
				{
					strMultB = strMultB + strSecEquation[intMultBCut];
					intMultBCut++;
				}
				if (!strMultB.length())
				{
					return{ "算式格式错误，但费用不退哦",0,false };
				}
				int multResult = stoi(strMultA)*stoi(strMultB);
				strSecEquation = strSecEquation.replace(intMultACut + 1, intMultBCut - intMultACut - 1, to_string(multResult));
				strSecDetail = strSecEquation;
			}
			//计算加减法————————————————————————————————————

			bool isNegative = false;



			while (strSecEquation.length() &&
				(strSecEquation.find('+') != string::npos || strSecEquation.find('-') != string::npos))
			{
				//如果开头有负号的话先纠正符号,考虑到如果加减法计算器输出是-4这种格式的话仍然会运算，则将这一部分植入计算器内
				if (strSecEquation[0] == '-' && strSecEquation.length() >= 2)
				{
					isNegative = true;
					strSecEquation.erase(0, 1);
					for (int intPlusCut = 0; intPlusCut != strSecEquation.length(); intPlusCut++)
					{
						if (static_cast<unsigned char>(strSecEquation[intPlusCut]) == '+')
						{
							strSecEquation.replace(intPlusCut, 1, "-");
						}
						else if (static_cast<unsigned char>(strSecEquation[intPlusCut]) == '-')
						{
							strSecEquation.replace(intPlusCut, 1, "+");
						}
						else
						{
							continue;
						}
					}
				}
				if (strSecEquation[0] == '+' && strSecEquation.length() >= 2)
				{
					strSecEquation.erase(0, 1);
				}
				if (strSecEquation.find('+') == string::npos && strSecEquation.find('-') == string::npos)
				{
					strSecDetail = strSecEquation;
					break;
				}

				int intPlusCut = 0;
				string strPlusA, strPlusB;
				bool boolPlusMakr = true;
				while (intPlusCut >= 0 && intPlusCut != strSecEquation.length() && isdigit(static_cast<unsigned char>(strSecEquation[intPlusCut])))
				{
					strPlusA += strSecEquation[intPlusCut];
					intPlusCut++;
				}
				if (!strPlusA.length())
				{
					return{ "算式格式错误，但费用不退哦",0,false };
				}
				if (intPlusCut >= 0 && intPlusCut != strSecEquation.length() && static_cast<unsigned char>(strSecEquation[intPlusCut] == '-'))
				{
					boolPlusMakr = false;
					intPlusCut++;
				}
				else
				{
					intPlusCut++;
				}
				while (intPlusCut >= 0 && intPlusCut != strSecEquation.length() && isdigit(static_cast<unsigned char>(strSecEquation[intPlusCut])))
				{
					strPlusB += strSecEquation[intPlusCut];
					intPlusCut++;
				}
				if (!strPlusB.length())
				{
					return{ "算式格式错误，但费用不退哦",0,false };
				}
				int intPlusResult;
				if (boolPlusMakr)
				{
					intPlusResult = stoi(strPlusA) + stoi(strPlusB);
				}
				else
				{
					intPlusResult = stoi(strPlusA) - stoi(strPlusB);
				}
				strSecEquation.replace(0, intPlusCut, to_string(intPlusResult));
				strSecDetail = to_string(intPlusResult);
			}
			//重组算式——————————————————————————————————————————

			//判断一下括号前面是不是省略了乘号
			if (strFront.length() >= 2 && isdigit(static_cast<unsigned char>(strFront[strFront.length() - 2])))
			{
				strFront = strFront.erase(strFront.length() - 1) + "*(";
			}
			//判断一下结果是不是负数
			if (isNegative)
			{
				int intNegativeCut = strFront.length() - 1;
				while (intNegativeCut >= 0 && isNegative)
				{
					if (static_cast<unsigned char>(strFront[intNegativeCut] == '+'))
					{
						strFront.replace(intNegativeCut, 1, "-");
						isNegative = false;
					}
					else if (static_cast<unsigned char>(strFront[intNegativeCut] == '-'))
					{
						strFront.replace(intNegativeCut, 1, "+");
						isNegative = false;
					}
					else
					{
						intNegativeCut--;
					}
				}
			}
			if (isNegative)
			{
				strFront = '-' + strFront;
				isNegative = false;
			}
			//判断一下这是不是括号内的式子，即strFront和strBehind是否为空
			if (strFront.length() != 0 && strBehind.length() != 0)
			{
				if (strFront == "-")
				{
					strDetail = strDetail + '=' + strFront + strSecDetail + strBehind.substr(1);
					strEquation = strFront + strSecEquation + strBehind.substr(1);
				}
				else
				{
					strDetail = strDetail + '=' + strFront.substr(0, strFront.length() - 1) + strSecDetail + strBehind.substr(1);
					strEquation = strFront.substr(0, strFront.length() - 1) + strSecEquation + strBehind.substr(1);
				}
			}
			else if (strFront.length() != 0)
			{
				if (strFront == "-")
				{
					strDetail = strDetail + '=' + strFront + strSecDetail;
					strEquation = strFront + strSecDetail;
				}
				else
				{
					strDetail = strDetail + '=' + strFront.substr(0, strFront.length() - 1) + strSecDetail;
					strEquation = strFront.substr(0, strFront.length() - 1) + strSecDetail;
				}
			}
			else if (strBehind.length() != 0)
			{
				strDetail = strDetail + '=' + strSecDetail + strBehind.substr(1);
				strEquation = strSecDetail + strBehind.substr(1);
			}
			else
			{
				strDetail = strDetail + '=' + strSecEquation;
				strEquation = strSecEquation;
			}
		}
	}
	return{ strDetail,stoi(strEquation),true };
}

