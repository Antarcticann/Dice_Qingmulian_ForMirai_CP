﻿#include <string>
#include <set>
#include <map>
#include <time.h>/*以下为计时模块*/

using namespace std;

unsigned long long MasterQQId = 429189622;//骰主QQ
bool boolRunCirculate = false;//主循环默认开关
bool boolRunLmt = false;//闲置群清退开关
int intLmtDayTime = 21;//闲置群退出阈值
string strFileLoc = ".\\";

set<unsigned long long> DisabledGroup;//关闭群列表
map<unsigned long long, string>WelcomeMsg;//欢迎信息存储
map<unsigned long long, unsigned int>RoomRule;//群规存储
map<unsigned long long, unsigned int >DefaultDice;//默认骰存储
map<unsigned long long, unsigned int>JrrpMap;//jrrp存储

map<unsigned long long, time_t>LmtGroupList;//闲置群列表
set<unsigned long long> LMTWhiteList;//闲置白名单

using InitListType = map<string, int>;
map<unsigned long long, InitListType>InitList;

map<unsigned long long, string>GlobalNickName;//<map>qqid->昵称
using GroupNickNameType = map<unsigned long long, string>;//<map>群号->昵称
map<unsigned long long, GroupNickNameType>GroupNickNameList;//qqid-><map>群号

using PropType = map<string, int>;//<map>技能名->技能值
map<unsigned long long, PropType> CharacterProp;//默认卡 qq-><map>技能名

using CharPropType = map<string, PropType>;/*多卡存储，<map>角色-><map>属性*/
map<unsigned long long, CharPropType> MultCharProp;/*多卡存储，qqid-><map>角色*/

using GroupCardList = map<unsigned long long, string>;/*群卡绑定，<map>群号->角色*/
map<unsigned long long, GroupCardList> PlayerGroupList;/*群卡绑定，<map>QQID->群号*/

string format(string str, const initializer_list<const string>& replace_str)//Dice的标准化字符串
{
	auto counter = 0;
	for (const auto& element : replace_str)
	{
		auto replace = "{" + std::to_string(counter) + "}";
		auto replace_pos = str.find(replace);
		while (replace_pos != std::string::npos)
		{
			str.replace(replace_pos, replace.length(), element);
			replace_pos = str.find(replace);
		}
		counter++;
	}
	return str;
}

//自带检测白名单功能
void IdleTimer(unsigned long long groupId)
{
	if (!groupId || LMTWhiteList.count(groupId))
	{
		return;
	}
	time_t nowtime;
	time(&nowtime);
	LmtGroupList[groupId] = nowtime;
	return;
}
