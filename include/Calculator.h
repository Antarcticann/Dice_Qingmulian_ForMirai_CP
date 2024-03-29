#pragma once
#ifndef	CDs_Caclulator
#define CDs_Caclulator

#include <string>
using namespace std;

struct DiceCalculatorOutput
{
	DiceCalculatorOutput(string a, int b, bool c) :detail(a), result(b), complate(c)
	{
	}
	string detail;//计算过程，包括初始算式与结果
	int result;//数字形式的结果
	bool complate;//计算没出错则返回true
};

DiceCalculatorOutput SimpleDice(unsigned int A, unsigned int B);
DiceCalculatorOutput DiceCalculator(string inputEquation, int intDice);

#endif