// Copyright (C) 2020-2021 Eritque arcus and contributors.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or any later version(in your opinion).
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

// MiraiCP依赖文件(只需要引入这一个)
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <time.h>/*计时模块*/

#include <miraiCP.hpp>

#include <RandomCore.h>
#include <UpperToLowerByCD.h>
#include <SpecialFunctionMap.h>
#include <RD.h>
#include <DataBase.h>
#include <Calculator.h>
#include <NameGenerator.h>


using namespace std;
using namespace MiraiCP;

#ifdef 多线程示例

// 多线程示例
void func(unsigned long long i, unsigned long long botid) {
	// 执行操作
	Friend(i, botid).sendMsg("hi");
	manager->detach();
}

#endif // 多线程示例

// 多线程示例
void MainCirculate(unsigned long long Masterid, unsigned long long botid) {
	// 执行操作
	bool isNewWeek = true;
	SYSTEMTIME LastTime;
	GetLocalTime(&LastTime);
	LastTime.wMinute--;
	LastTime.wHour--;
	LastTime.wDay--;
	LastTime.wMonth--;
	Friend(Masterid, botid).sendMsg("主循环已启动");
	while (boolRunCirculate)
	{
		SYSTEMTIME NowTime;
		GetLocalTime(&NowTime);
		if (NowTime.wMinute != LastTime.wMinute)/*每分钟一次的事件*/
		{
			LastTime.wMinute = NowTime.wMinute;
		}
		if (NowTime.wHour != LastTime.wHour)/*每小时一次的事件*/
		{
			LastTime.wHour = NowTime.wHour;
			if (boolRunLmt)//闲置列表监视
			{
				time_t nowtime;
				time(&nowtime);
				vector<unsigned long long>GroupList = Bot(botid).getGroupList();
				int lmtRefrCut = 0;
				for (vector<unsigned long long>::iterator GroupListCut = GroupList.begin(); GroupListCut != GroupList.end(); GroupListCut++)
				{
					string strInfo;
					if (LmtGroupList.count(*GroupListCut))
					{
						if (LMTWhiteList.count(*GroupListCut))
						{
							continue;
						}
						if ((nowtime - LmtGroupList[*GroupListCut]) >= (intLmtDayTime * 24 * 60 * 60))
						{
							Group(*GroupListCut, botid).sendMsg("已经在这里摸鱼" + to_string(intLmtDayTime) + "天了，马上就会离开这里。仍有需要的话可以再次邀请");
							Group(*GroupListCut, botid).quit();
							Friend(Masterid, botid).sendMsg("在" + to_string(*GroupListCut) + Group(*GroupListCut, botid).nickOrNameCard() + "里摸鱼" + to_string(intLmtDayTime) + "天,已退出该群");
							Sleep(1000);
						}
					}
					else
					{
						lmtRefrCut++;
						if (!LMTWhiteList.count(*GroupListCut))
						{
							
							IdleTimer(*GroupListCut);
							if (lmtRefrCut <= 8)
							{
								Friend(Masterid, botid).sendMsg("发现漏网之鱼:" + to_string(*GroupListCut) + Group(*GroupListCut, botid).nickOrNameCard());
								Sleep(1000);
							}
							logger->info("发现漏网之鱼:" + to_string(*GroupListCut));
						}					
					}
				}
				if (lmtRefrCut > 0)
				{
					Friend(Masterid, botid).sendMsg("发现" + to_string(lmtRefrCut) + "条漏网之鱼,已全部更新");
				}
				Bot(botid).refreshInfo();
				//刷新LMT白名单
				GroupList = Bot(botid).getGroupList();
				set<unsigned long long>TempLMPWhiteList;
				for (set<unsigned long long>::iterator LMTWhiteListCut = LMTWhiteList.begin(); LMTWhiteListCut != LMTWhiteList.end(); LMTWhiteListCut++)
				{
					vector<unsigned long long>::iterator result = find(GroupList.begin(), GroupList.end(), *LMTWhiteListCut);
					if (result == GroupList.end())//检测群列表里是否存在该群
					{
						Friend(Masterid, botid).sendMsg("白名单无效群：" + to_string(*LMTWhiteListCut) + "，已删除该群");
						Sleep(1000);
						continue;
					}
					TempLMPWhiteList.insert(*LMTWhiteListCut);
				}
				LMTWhiteList = TempLMPWhiteList;
				//刷新LMT列表
				map<unsigned long long, time_t>TemLMTGroupList;
				for (map<unsigned long long, time_t>::iterator LMTListCut = LmtGroupList.begin(); LMTListCut != LmtGroupList.end(); LMTListCut++)
				{
					vector<unsigned long long>::iterator result = find(GroupList.begin(), GroupList.end(), LMTListCut->first);
					if (result == GroupList.end())//检测群列表里是否存在该群
					{
						Friend(Masterid, botid).sendMsg("LMT无效群：" + to_string(LMTListCut->first) + "，已删除该群");
						Sleep(1000);
						continue;
					}
					if (LMTWhiteList.count(LMTListCut->first))//检测白名单里是否存在该群
					{
						Friend(Masterid, botid).sendMsg("LMT白名单群：" + to_string(LMTListCut->first) + "，已移除该群");
						Sleep(1000);
						continue;
					}
					TemLMTGroupList[LMTListCut->first] = LMTListCut->second;
				}
				LmtGroupList = TemLMTGroupList;
			}
			if (true)//备份数据
			{
				ofstream ofstreamLMTWhite(strFileLoc + "LMTWhite.RDconf", ios::out | ios::trunc);
				for (auto it = LMTWhiteList.begin(); it != LMTWhiteList.end(); ++it)
				{
					ofstreamLMTWhite << *it << std::endl;
				}
				ofstreamLMTWhite.close();
				logger->info("LMT白名单存储完成");
				ofstream ofstreamLMTList(strFileLoc + "LMTList.RDconf", ios::out | ios::trunc);
				for (auto it = LmtGroupList.begin(); it != LmtGroupList.end(); ++it)
				{
					ofstreamLMTList << it->first << " " << it->second << std::endl;
				}
				ofstreamLMTList.close();
				logger->info("LMT列表存储完成");
				ofstream ofstreamJrrpMap(strFileLoc + "JrrpMap.RDconf", ios::out | ios::trunc);
				for (auto it = JrrpMap.begin(); it != JrrpMap.end(); ++it)
				{
					ofstreamJrrpMap << it->first << " " << it->second << std::endl;
				}
				ofstreamJrrpMap.close();
				logger->info("Jrrp存储完成");
				ofstream ofstreamGlobalNickName(strFileLoc + "GlobalNickName.RDconf", ios::out | ios::trunc);
				for (auto it = GlobalNickName.begin(); it != GlobalNickName.end(); ++it)
				{
					ofstreamGlobalNickName << it->first << " " << it->second << std::endl;
				}
				ofstreamGlobalNickName.close();
				logger->info("全局昵称存储完成");
				ofstream ofstreamGroupNickName(strFileLoc + "GroupNickName.RDconf", ios::out | ios::trunc);
				for (auto it = GroupNickNameList.begin(); it != GroupNickNameList.end(); ++it)
				{
					for (auto it1 = it->second.begin(); it1 != it->second.end(); ++it1)
					{
						ofstreamGroupNickName << it->first << " " << it1->first << " " << it1->second << std::endl;
					}
				}
				ofstreamGroupNickName.close();
				logger->info("群组昵称存储完成");
				ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::out | ios::trunc);
				for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); ++it)
				{
					ofstreamDisabledGroup << *it << std::endl;
				}
				ofstreamDisabledGroup.close();
				logger->info("关闭群组存储完成");
				ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
				for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
				{
					while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
					while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
					while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
					while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
					ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
				}
				ofstreamWelcomeMsg.close();
				logger->info("群组欢迎信息存储完成");
				ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
				for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
				{
					for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
					{
						ofstreamCharacterProp << it->first << " " << it1->first << " " << it1->second << std::endl;
					}
				}
				ofstreamCharacterProp.close();
				logger->info("默认卡存储完成");
				ofstream ofstreamMultCard(strFileLoc + "MultCard.RDconf", ios::out | ios::trunc);
				for (auto QQID = MultCharProp.begin(); QQID != MultCharProp.end(); ++QQID)
				{
					map<string, PropType> CharList = QQID->second;
					for (auto CharCount = CharList.begin(); CharCount != CharList.end(); ++CharCount)
					{
						map<string, int> PropList = CharCount->second;
						for (auto PropCount = PropList.begin(); PropCount != PropList.end(); ++PropCount)
						{
							ofstreamMultCard << QQID->first << " " << CharCount->first << " " << PropCount->first << " " << PropCount->second << std::endl;
						}
					}
				}
				ofstreamMultCard.close();
				logger->info("角色卡存储完成");
				ofstream ofstreamGroupCard(strFileLoc + "GroupCard.RDconf", ios::out | ios::trunc);
				for (auto PlayerGroup = PlayerGroupList.begin(); PlayerGroup != PlayerGroupList.end(); ++PlayerGroup)
				{
					map<unsigned long long, string> GroupChar = PlayerGroup->second;
					for (auto GroupCount = GroupChar.begin(); GroupCount != GroupChar.end(); ++GroupCount)
					{
						ofstreamGroupCard << PlayerGroup->first << " " << GroupCount->first << " " << GroupCount->second << std::endl;
					}
				}
				ofstreamGroupCard.close();
				logger->info("角色卡绑定存储完成");
				ofstream ofstreamRoomRule(strFileLoc + "RoomRule.RDconf", ios::out | ios::trunc);
				for (auto it = RoomRule.begin(); it != RoomRule.end(); ++it)
				{
					ofstreamRoomRule << it->first << " " << it->second << std::endl;
				}
				ofstreamRoomRule.close();
				logger->info("村规存储完成");
				/*插件结束*/
			}
		}
		if (NowTime.wDay != LastTime.wDay)/*每天执行一次的事件*/
		{
			if (!NowTime.wHour)/*仅在0点执行（防止重启导致的运行*/
			{

			}
			isNewWeek = true;
			LastTime.wDay = NowTime.wDay;
		}
		if (NowTime.wDayOfWeek == 1 && isNewWeek)/*每周一执行一次的事件*/
		{
			isNewWeek = false;
		}
		if (NowTime.wMonth != LastTime.wMonth)/*每月执行一次的事件*/
		{
			LastTime.wMonth = NowTime.wMonth;
		}
		Sleep(10 * 1000);/*等待10s*/
	}
	Friend(Masterid, botid).sendMsg("主循环已停止");
	manager->detach();
}

// 插件实例
class Main:public CPPPlugin {
public:
    // 配置插件信息
    Main() : CPPPlugin(PluginConfig("Dice!_CD.ver （per-alpha）", "0.2", "测不准柴刀|QQ 429189622", "基于Dice!by 溯洄制作，有大量魔改", "2021")) {}
    void onEnable() override {
        /*插件启动, 请勿在此函数运行前执行操作mirai的代码*/
        /*
        logger - 日志组件
            logger->info(string)发送消息级日志
            logger->warning(string)发送警告级日志
            logger->error(string)发送错误级日志
        一共有3种logger
         1. 是最外层的logger指针，为MiraiCP插件级logger, 标识符为MiraiCP: [content], 不建议用这个logger输出，该logger通常在MiraiCP内部使用
         2. 是CPPPlugin下的pluginLogger，为插件级logger，即当前MiraiCP加载的插件，标识符为[name]: [content], 建议用于调试信息
         3. 是每个事件的botLogger, 如:e.botLogger, 为每个机器人账号独有的logger，建议日常使用，标识符为[botid]: [content]
        procession 广播源
            procession->registerEvent<EventType>(lambda) 注册监听
            procession->registerEvent<GroupMessageEvent>([](GroupMessageEvent param){ \*处理*\});是监听群消息
            procession->registerEvent<PrivateMessageEvent>([](PrivateMessageEvent param){ \*处理*\});是监听私聊消息
            ...
        参数都在param变量里，在lambda块中使用param.xxx来调用
        */
		procession->registerEvent<BotOnlineEvent>([](BotOnlineEvent e) {

			ifstream ifstreamLMTWhite(strFileLoc + "LMTWhite.RDconf");
			if (ifstreamLMTWhite)//存储关闭群
			{
				unsigned long long Group;
				while (ifstreamLMTWhite >> Group)
				{
					LMTWhiteList.insert(Group);
				}
			}
			ifstreamLMTWhite.close();
			e.botlogger.info("LMT白名单读取完成");
			ifstream ifstreamLMTList(strFileLoc + "LMTList.RDconf");
			if (ifstreamLMTList)
			{
				unsigned long long GroupID, LMTtime;
				while (ifstreamLMTList >> GroupID >> LMTtime)
				{
					LmtGroupList[GroupID] = LMTtime;
				}
			}
			ifstreamLMTList.close();
			e.botlogger.info("LMT列表读取完成");
			ifstream ifstreamJrrpMap(strFileLoc + "JrrpMap.RDconf");
			if (ifstreamJrrpMap)
			{
				unsigned long long QQID;
				unsigned int Jrrp;
				while (ifstreamJrrpMap >> QQID >> Jrrp)
				{
					JrrpMap[QQID] = Jrrp;
				}
			}
			ifstreamJrrpMap.close();
			e.botlogger.info("Jrrp读取完成");
			ifstream ifstreamGlobalNickName(strFileLoc + "GlobalNickName.RDconf");
			if (ifstreamGlobalNickName)
			{
				unsigned long long QQID;
				string NickName;
				while (ifstreamGlobalNickName >> QQID >> NickName)
				{
					GlobalNickName[QQID] = NickName;
				}
			}
			ifstreamGlobalNickName.close();
			e.botlogger.info("全局昵称读取完成");
			ifstream ifstreamGroupNickName(strFileLoc + "GroupNickName.RDconf");
			if (ifstreamGroupNickName)
			{
				unsigned long long GroupID, QQID;
				string NickName;
				while (ifstreamGroupNickName >> QQID >> GroupID >> NickName)
				{
					GroupNickNameList[QQID][GroupID] = NickName;
				}
			}
			ifstreamGroupNickName.close();
			e.botlogger.info("群组昵称读取完成");
			ifstream ifstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf");
			if (ifstreamDisabledGroup)//存储关闭群
			{
				unsigned long long Group;
				while (ifstreamDisabledGroup >> Group)
				{
					DisabledGroup.insert(Group);
				}
			}
			ifstreamDisabledGroup.close();
			e.botlogger.info("关闭群列表读取完成");
			ifstream ifstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf");
			if (ifstreamWelcomeMsg)//存储欢迎信息
			{
				unsigned long long GroupID;
				string Msg;
				while (ifstreamWelcomeMsg >> GroupID >> Msg)
				{
					while (Msg.find("{space}") != string::npos)Msg.replace(Msg.find("{space}"), 7, " ");
					while (Msg.find("{tab}") != string::npos)Msg.replace(Msg.find("{tab}"), 5, "\t");
					while (Msg.find("{endl}") != string::npos)Msg.replace(Msg.find("{endl}"), 6, "\n");
					while (Msg.find("{enter}") != string::npos)Msg.replace(Msg.find("{enter}"), 7, "\r");
					WelcomeMsg[GroupID] = Msg;
				}
			}
			ifstreamWelcomeMsg.close();
			e.botlogger.info("群欢迎信息读取完成");
            ifstream ifstreamCharacterProp(strFileLoc + "CharacterProp.RDconf");
            if (ifstreamCharacterProp)
            {
                unsigned long long QQ;
                unsigned int Value;
                string SkillName;
                while (ifstreamCharacterProp >> QQ >> SkillName >> Value)
                {
                    CharacterProp[QQ][SkillName] = Value;
                }
            }
            ifstreamCharacterProp.close();
			e.botlogger.info("默认卡信息读取完成");
            ifstream ifstreamMultCard(strFileLoc + "MultCard.RDconf");
            if (ifstreamMultCard)
            {
                unsigned long long QQID;
                unsigned int Value;
                string SkillName, CharacterName;
                while (ifstreamMultCard >> QQID >> CharacterName >> SkillName >> Value)
                {
                    MultCharProp[QQID][CharacterName][SkillName] = Value;
                }
            }
            ifstreamMultCard.close();
			e.botlogger.info("角色卡信息读取完成");
            ifstream ifstreamGroupCard(strFileLoc + "GroupCard.RDconf");
            if (ifstreamGroupCard)
            {
                unsigned long long QQID, GroupID;
                string CharName;
                while (ifstreamGroupCard >> QQID >> GroupID >> CharName)
                {
                    PlayerGroupList[QQID][GroupID] = CharName;
                }
            }
            ifstreamGroupCard.close();
			e.botlogger.info("角色卡绑定读取完成");
            ifstream ifstreamRoonRule(strFileLoc + "RoomRule.RDconf");
            if (ifstreamRoonRule)
            {
                unsigned long long GroupID;
                unsigned int RoonRuleNum;
                while (ifstreamRoonRule >> GroupID >> RoonRuleNum)
                {
                    RoomRule[GroupID] = RoonRuleNum;
                }
            }
            ifstreamRoonRule.close();
			e.botlogger.info("村规读取完成");
            e.botlogger.info("目前以名称检索的酒品名单上的酒品分别有这么多：" + BackCocktailList());
        });
        // 邀请事件
        // 好友申请
        procession->registerEvent<NewFriendRequestEvent>([](NewFriendRequestEvent e) {
			e.accept();
			Friend(MasterQQId, e.bot.id).sendMsg("收到好友申请，已同意，邀请者：" + to_string(e.fromid) + e.nick);
        });
        // 邀请加群
        procession->registerEvent<GroupInviteEvent>([](GroupInviteEvent e) {
			e.accept();
			Friend(MasterQQId, e.bot.id).sendMsg("收到加群邀请，群" + to_string(e.groupid) + e.groupName + "，已同意，邀请者：" + to_string(e.inviterid) + e.inviterNick);
        });
        // 消息事件
        // 监听私聊
        procession->registerEvent<PrivateMessageEvent>([](PrivateMessageEvent e) {
#ifdef privateExamble



            unsigned long long id = e.bot.id;
            e.botlogger.info(std::to_string(id));
            e.message.source.quoteAndSendMsg("HI");
            std::thread func1(func, e.sender.id(), e.bot.id);
            e.sender.sendMsg(e.message.content);
            func1.detach();
            // 多线程测试,线程应该在lambda中决定要detach还是join, 否则会报错
            // 测试取图片
            std::vector<std::string> temp = Image::GetImgIdsFromMiraiCode(e.message.content);
            for (const std::string &a : temp) {
                e.sender.sendMsg(a);
            }
            // 发送图片
            Image tmp = e.sender.uploadImg(R"(C:\Users\19308\Desktop\a.jpg)");
            e.sender.sendMsg(tmp.toMiraiCode());
            e.sender.sendMiraiCode(tmp.toMiraiCode());
            e.message.source.recall();



#endif // privateExamble
			string msg = e.message.content.toString();
			while (isspace(static_cast<unsigned char>(msg[0])))
				msg.erase(msg.begin());
			string FullStop = "。";
			if (msg.substr(0, FullStop.length()) == FullStop)
			{
				msg.replace(0, FullStop.length(), ".");
			}
			if (msg[0] != '.')
			{
				return;
			}
			int intMsgCnt = 1;
			while (isspace(static_cast<unsigned char>(msg[intMsgCnt])))
				intMsgCnt++;
			string strNickName;
			if (GlobalNickName.count(e.sender.id()))
			{
				strNickName = GlobalNickName[e.sender.id()];
			}
			else
			{
				strNickName = e.sender.nickOrNameCard();
			}
			int permission = 0;
			if (e.sender.id() == MasterQQId)
			{
				permission = 3;
			}
			string strLowerMessage = msg;
			strLowerMessage = CDs_tolower(strLowerMessage);
			if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
			{				
				e.sender.sendMsg(GlobalMsg[EnumInfoMsg.BOT]);
			}
			else if (strLowerMessage.substr(intMsgCnt, 4) == "help")
			{
				Image helptmp = e.sender.uploadImg(R"(.\help.jpg)");
				e.sender.sendMiraiCode(helptmp.toMiraiCode());
			}
			else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
			{
				int intJrrpKey;
				SYSTEMTIME Time;
				GetLocalTime(&Time);
				if (JrrpMap.count(0))
				{
					if (JrrpMap[0] != Time.wDay)
					{
						JrrpMap.clear();
						JrrpMap[0] = Time.wDay;
					}
				}
				else
				{
					JrrpMap[0] = Time.wDay;
				}
				if (JrrpMap.count(e.sender.id()))
				{
					intJrrpKey = JrrpMap[e.sender.id()];
				}
				else
				{
					intJrrpKey = RandomGenerator::Randint(1, 100);
					JrrpMap[e.sender.id()] = intJrrpKey;
				}
				string Sortilege = "（晃动签桶）......那么今天神明给" + strNickName + "的指引是.....\n" + "第" + std::to_string(intJrrpKey) + "签  " + SensojiTempleDivineSign[intJrrpKey];
				e.sender.sendMsg(Sortilege);
			}
			else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
			{
				intMsgCnt += 3;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				int intCocCreater;
				string strCocCreater;
				while (intMsgCnt != strLowerMessage.length() && isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strCocCreater += strLowerMessage[intMsgCnt];
					if (strCocCreater.length() >= 3)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
						return;
					}
					intMsgCnt++;
				}
				if (strCocCreater.empty())
				{
					intCocCreater = 1;
				}
				else
				{
					intCocCreater = stoi(strCocCreater);
					if (intCocCreater > 10)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
						return;
					}
					if (intCocCreater == 0)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量为0]);
						return;
					}
				}
				string strReply = "那么当调查员" + strNickName + "醒来，发现自己可能是这个样子";
				for (int i = 1; i <= intCocCreater; i++)
				{
					string strOneCreat;
					int intVal, intSum = 0, intSumLuck = 0;
					intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "力量:" + to_string(intVal) + " ";
					intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "体质:" + to_string(intVal) + " ";
					intVal = SimpleDice(2, 6).result * 5 + 30; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "体型:" + to_string(intVal) + " ";
					intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "敏捷:" + to_string(intVal) + " ";
					intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "外貌:" + to_string(intVal) + " ";
					intVal = SimpleDice(2, 6).result * 5 + 30; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "智力:" + to_string(intVal) + " ";
					intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "意志:" + to_string(intVal) + " ";
					intVal = SimpleDice(2, 6).result * 5 + 30; intSum += intVal; intSumLuck += intVal;
					strOneCreat = strOneCreat + "教育:" + to_string(intVal) + " ";
					intVal = SimpleDice(3, 6).result * 5; intSumLuck += intVal;
					strOneCreat = strOneCreat + "幸运:" + to_string(intVal) + " 共计:" + to_string(intSumLuck) + "(" + to_string(intSum) + ")";
					strReply += "\n" + strOneCreat;
			}
				e.sender.sendMsg(strReply);
			}
			else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
			{
				intMsgCnt += 3;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				int intCocCreater;
				string strCocCreater;
				while (intMsgCnt != strLowerMessage.length() && isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strCocCreater += strLowerMessage[intMsgCnt];
					if (strCocCreater.length() >= 3)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
						return;
					}
					intMsgCnt++;
				}
				if (strCocCreater.empty())
				{
					intCocCreater = 1;
				}
				else
				{
					intCocCreater = stoi(strCocCreater);
					if (intCocCreater > 10)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
						return;
					}
					if (intCocCreater == 0)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量为0]);
						return;
					}
				}
				string strReply = "那么当冒险者" + strNickName + "醒来，发现自己可能是这个样子";
				for (int i = 1; i <= intCocCreater; i++)
				{
					string strOneCreat;
					int intSum = 0;
					int intVal[4] = { RandomGenerator::Randint(1, 6), RandomGenerator::Randint(1, 6), RandomGenerator::Randint(1, 6), RandomGenerator::Randint(1, 6) };
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "力量:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "敏捷:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "体质:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "智力:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "感知:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "魅力:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " 总计" + to_string(intSum + intVal[1] + intVal[2] + intVal[3]);
					strReply += "\n" + strOneCreat;
				}
				e.sender.sendMsg(strReply);
			}
			else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
			{
				IdleTimer(e.sender.id());
				intMsgCnt += 2;
				string strReply = "好像有人疯了，" + strNickName + "的疯狂发作临时症状:D10=";
				int intLi = RandomGenerator::Randint(1, 10);
				strReply = strReply + to_string(intLi) + "\n";
				string strLiTime = "D10=" + to_string(RandomGenerator::Randint(1, 10));
				if (intLi == 9)
				{
					int intLi9 = RandomGenerator::Randint(1, 100);
					strReply = strReply + format(TempInsanity[intLi], strLiTime, "D100=" + to_string(intLi9), strFear[intLi9]);
				}
				else if (intLi == 10)
				{
					int intLi10 = RandomGenerator::Randint(1, 100);
					strReply = strReply + format(TempInsanity[intLi], strLiTime, "D100=" + to_string(intLi10), strPanic[intLi10]);
				}
				else
				{
					strReply = strReply + format(TempInsanity[intLi], strLiTime);
				}
				e.sender.sendMsg(strReply);
			}
			else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
			{
				intMsgCnt += 2;
				string strReply = "与一位调查员失去联系，" + strNickName + "疯狂发作总结症状:D10=";
				int intLi = RandomGenerator::Randint(1, 10);
				strReply = strReply + to_string(intLi) + "\n";
				string strLiTime = "D10=" + to_string(RandomGenerator::Randint(1, 10));
				if (intLi == 9)
				{
					int intLi9 = RandomGenerator::Randint(1, 100);
					strReply = strReply + format(LongInsanity[intLi], strLiTime, "D100=" + to_string(intLi9), strFear[intLi9]);
				}
				else if (intLi == 10)
				{
					int intLi10 = RandomGenerator::Randint(1, 100);
					strReply = strReply + format(LongInsanity[intLi], strLiTime, "D100=" + to_string(intLi10), strPanic[intLi10]);
				}
				else
				{
					strReply = strReply + format(LongInsanity[intLi], strLiTime);
				}
				e.sender.sendMsg(strReply);
			}
			else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
		   {
				intMsgCnt += 2;
				string strDetail;//若涉及属性修改，则使用这一项
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				if (intMsgCnt == strLowerMessage.length())/*.st后面什么都没有*/
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
					return;
				}
				if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
				{
					if (CharacterProp.count(e.sender.id()))
					{
						CharacterProp.erase(e.sender.id());
					}
					e.sender.sendMsg(GlobalMsg[EnumInfoMsg.属性删除完成]);
					return;
				}
				if (strLowerMessage.substr(intMsgCnt, 3) == "del")
				{
					intMsgCnt += 3;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						intMsgCnt++;
					string strSkillName;
					while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
						intMsgCnt] == '|'))
					{
						strSkillName += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
						if (SkillNameReplace.count(strSkillName))
						{
							strSkillName = SkillNameReplace[strSkillName];
						}
						if (CharacterProp.count(e.sender.id()) && CharacterProp[e.sender.id()].count(strSkillName))
						{
							CharacterProp[e.sender.id()].erase(strSkillName);
							e.sender.sendMsg(GlobalMsg[EnumInfoMsg.技能删除完成]);
						}
						else
						{
							e.sender.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
						}
						return;
				}
				if (strLowerMessage.substr(intMsgCnt, 4) == "show")
				{
					intMsgCnt += 4;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						intMsgCnt++;
					string strSkillName;
					while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
						intMsgCnt] == '|'))
					{
						strSkillName += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
						if (SkillNameReplace.count(strSkillName))
							strSkillName = SkillNameReplace[strSkillName];
						if (CharacterProp.count(e.sender.id())
							&& CharacterProp[e.sender.id()].count(strSkillName))
						{
							e.sender.sendMsg(format(GlobalMsg[EnumInfoMsg.属性查找成功], { strNickName + "的[默认卡]", strSkillName,
								to_string(CharacterProp[e.sender.id()][strSkillName]) }));
						}
						else if (SkillDefaultVal.count(strSkillName))
						{
							e.sender.sendMsg(format(GlobalMsg[EnumInfoMsg.属性查找成功], { strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName]) }));
						}
						else if (!strSkillName.length())/*mark*/
						{
							string strReply = "根据客户档案上的记录，" + strNickName + "的[默认卡]上的属性如下：";
							map<string, int> AllSkill = CharacterProp[e.sender.id()];
							if (AllSkill.empty())
							{
								e.sender.sendMsg(strNickName + "的[默认卡]好像没有录入过和默认值不一样的信息诶");
								return;
							}
							map<string, int>::iterator SkillCount = AllSkill.begin();
							while (!(SkillCount == AllSkill.end()))
							{
								strReply = strReply + " " + SkillCount->first + to_string(SkillCount->second);
								SkillCount++;
							}
							e.sender.sendMsg(strReply);
						}
						else
						{
							e.sender.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
						}
						return;
				}
				bool boolError = false;
				/*多卡存储（下方*/
				if (strLowerMessage.substr(intMsgCnt, 3) == "add")
				{
					intMsgCnt += 3;
					if (MultCharProp.count(e.sender.id()))
					{
						map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
						if ((Allcharacter.size() >= 10) && (e.sender.id() != MasterQQId))
						{
							e.sender.sendMsg(GlobalMsg[EnumErrorMsg.存储卡数量过多]);
							return;
						}
					}

					string strCharName;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
						intMsgCnt++;
					if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
						return;
					}
					while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
						isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
						!= ':')
					{
						strCharName += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
						intMsgCnt++;


					while (intMsgCnt != strLowerMessage.length())
					{
						string strSkillName;
						while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
							isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
							!= ':'
							&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '+'
							&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '-' && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '*'
							)
						{
							strSkillName += strLowerMessage[intMsgCnt];
							intMsgCnt++;
						}
						if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];/*技能名标准化*/
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
						] == ':')intMsgCnt++;
						string strSkillVal;
						/*加入-+*d的判定 向下*/
						while (intMsgCnt != strLowerMessage.length() && (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
							|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
							|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'))//修改了技能名获取的逻辑
						{
							strSkillVal += strLowerMessage[intMsgCnt];
							intMsgCnt++;
						}
						if (strSkillName.empty() || strSkillVal.empty())//移动大小判断到下方
						{
							boolError = true;
							break;
						}
						if (strSkillVal.find("+") != string::npos || strSkillVal.find("-") != string::npos
							|| strSkillVal.find("*") != string::npos || strSkillVal.find("d") != string::npos)
						{
							if (MultCharProp.count(e.sender.id()) && MultCharProp[e.sender.id()].count(strCharName) && MultCharProp[e.sender.id()][strCharName].count(strSkillName))
							{
								strSkillVal = to_string(MultCharProp[e.sender.id()][strCharName][strSkillName]) + strSkillVal;
							}
							else if (SkillDefaultVal.count(strSkillName))
							{
								strSkillVal = to_string(SkillDefaultVal[strSkillName]) + strSkillVal;
							}
							else
							{
								strSkillVal = strSkillVal;
							}
							DiceCalculatorOutput Result = DiceCalculator(strSkillVal, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
							if (!Result.complate)
							{
								boolError = true;
								break;
							}
							if (Result.result < 0)
							{
								Result.result = 0;
							}
							strDetail = strDetail + strSkillName + ":" + strSkillVal + "=" + to_string(Result.result) + "\n";//记录修改过程
							strSkillVal = to_string(Result.result);
						}
						if (strSkillVal.length() > 3)
						{
							boolError = true;
							break;
						}
						/*加入-+*d的判定 向上*/
						if (!(stoi(strSkillVal) == SkillDefaultVal[strSkillName]))
							MultCharProp[e.sender.id()][strCharName][strSkillName] = stoi(strSkillVal);
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
					}
					if (boolError)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
						return;
					}
					if(strDetail.empty())
						e.sender.sendMsg("已储存[角色卡]：" + strCharName);
					else
					{
						e.sender.sendMsg("已储存[角色卡]：" + strCharName + "\n以下属性被修改\n" + strDetail.substr(0, strDetail.length() - 1));
					}
					return;
				}
				if (strLowerMessage.substr(intMsgCnt, 3) == "rmv")
				{
					if (!MultCharProp.count(e.sender.id()))
					{
						e.sender.sendMsg("您还没有在这里储存过[角色卡]！");
						return;
					}
					map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
					if (!Allcharacter.size())
					{
						e.sender.sendMsg("您还没有在这里储存过[角色卡]！");
						MultCharProp.erase(e.sender.id());
						return;
					}
					intMsgCnt += 3;
					string strCharName;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
						intMsgCnt++;
					if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
						return;
					}
					if (strLowerMessage.substr(intMsgCnt, 3) == "all")
					{
						MultCharProp.erase(e.sender.id());
						if (PlayerGroupList.count(e.sender.id()))
						{
							PlayerGroupList.erase(e.sender.id());
						}
						e.sender.sendMsg("已销毁所有[角色卡]！");
						return;
					}
					while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
						isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
						!= ':')
					{
						strCharName += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					if (!Allcharacter.count(strCharName))
					{
						e.sender.sendMsg("该[角色卡]不存在！");
						return;
					}
					MultCharProp[e.sender.id()].erase(strCharName);
					if (PlayerGroupList.count(e.sender.id()))
					{
						if (!PlayerGroupList[e.sender.id()].empty())
						{
							map<unsigned long long, string>::iterator GroupListCount = PlayerGroupList[e.sender.id()].begin();
							map<unsigned long long, string> TemGroupList;
							while (GroupListCount != PlayerGroupList[e.sender.id()].end())
							{
								if (GroupListCount->second != strCharName)
								{
									TemGroupList[GroupListCount->first] = GroupListCount->second;
								}
								GroupListCount++;
							}
							PlayerGroupList[e.sender.id()] = TemGroupList;
						}
						else
							PlayerGroupList.erase(e.sender.id());
					}
					e.sender.sendMsg("已销毁[角色卡]：" + strCharName);
					return;
				}
				if (strLowerMessage.substr(intMsgCnt, 4) == "card")
				{
					if (!MultCharProp.count(e.sender.id()))
					{
						e.sender.sendMsg("您还没有在这里储存过[角色卡]！");
						return;
					}
					map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
					if (!Allcharacter.size())
					{
						e.sender.sendMsg("您还没有在这里储存过[角色卡]！");
						MultCharProp.erase(e.sender.id());
						return;
					}
					intMsgCnt += 4;
					string strCharName;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
						intMsgCnt++;
					if (strLowerMessage.substr(intMsgCnt, 3) == "all")
					{
						string strReply;
						map<string, PropType>::iterator CharCount = Allcharacter.begin();
						while (!(CharCount == Allcharacter.end()))
						{
							strReply = strReply + " " + CharCount->first;
							CharCount++;
						}
						e.sender.sendMsg(strNickName + "在这里存储了的[角色卡]有：" + strReply);
						return;
					}
					if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
						return;
					}
					while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
						isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
						!= ':')
					{
						strCharName += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					if (!Allcharacter.count(strCharName))
					{
						e.sender.sendMsg("该[角色卡]不存在！");
						return;
					}
					string strReply = "根据客户档案上的记录，[角色卡]" + strCharName + "的属性如下：";
					map<string, int> CharacterProp = Allcharacter[strCharName];
					map<string, int>::iterator SkillCount = CharacterProp.begin();
					while (!(SkillCount == CharacterProp.end()))
					{
						strReply = strReply + " " + SkillCount->first + to_string(SkillCount->second);
						SkillCount++;
					}
					e.sender.sendMsg(strReply);
					return;
				}
				/*多卡存储（上方*/
				while (intMsgCnt != strLowerMessage.length())
				{
					string strSkillName;
					while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
						isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
						!= ':'
						&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '+'
						&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '-' && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '*'
						)
					{
						strSkillName += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					if (SkillNameReplace.count(strSkillName))
						strSkillName = SkillNameReplace[strSkillName];
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
					] == ':')intMsgCnt++;
					string strSkillVal;
					/*加入-+*d的判定 向下*/
					while (intMsgCnt != strLowerMessage.length() && (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'))
					{
						strSkillVal += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					if (strSkillName.empty() || strSkillVal.empty())//移动大小判断到下方
					{
						boolError = true;
						break;
					}
					if (strSkillVal.find("+") != string::npos || strSkillVal.find("-") != string::npos
						|| strSkillVal.find("*") != string::npos || strSkillVal.find("d") != string::npos)
					{
						if (CharacterProp[e.sender.id()].count(strSkillName))
						{
							strSkillVal = to_string(CharacterProp[e.sender.id()][strSkillName]) + strSkillVal;
						}
						else if (SkillDefaultVal.count(strSkillName))
						{
							strSkillVal = to_string(SkillDefaultVal[strSkillName]) + strSkillVal;
						}
						else
						{
							strSkillVal = strSkillVal;
						}
						DiceCalculatorOutput Result = DiceCalculator(strSkillVal, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (!Result.complate)
						{
							boolError = true;
							break;
						}
						if (Result.result < 0)
						{
							Result.result = 0;
						}
						strDetail = strDetail + strSkillName + ":" + strSkillVal + "=" + to_string(Result.result) + "\n";//记录修改过程
						strSkillVal = to_string(Result.result);
					}
					if (strSkillVal.length() > 3)
					{
						boolError = true;
						break;
					}
					/*加入-+*d的判定 向上*/
					if (stoi(strSkillVal) != SkillDefaultVal[strSkillName])
						CharacterProp[e.sender.id()][strSkillName] = stoi(strSkillVal);
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
				}
				if (boolError)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
				}
				else
				{
					if(strDetail.empty())
						e.sender.sendMsg(GlobalMsg[EnumInfoMsg.属性存储完成]);
					else
						e.sender.sendMsg(GlobalMsg[EnumInfoMsg.属性存储完成] + "\n以下属性被修改\n" + strDetail.substr(0, strDetail.length() - 1));
				}
				return;
		   }
		    else if (strLowerMessage.substr(intMsgCnt, 2) == "tz")/*随机特性的部分*/
			{
				const int intcharacteristic = RandomGenerator::Randint(1, 120);
				string CharacterReply = "经查证，在某个梦境中的" + strNickName + "的同位体拥有的特性为：\n" + Characteristic[intcharacteristic];
				e.sender.sendMsg(CharacterReply);
			}
		    else if (strLowerMessage.substr(intMsgCnt, 2) == "ct")/*mark.ct的部分*/
			{
				intMsgCnt += 2;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				int intcocktail;
				if (intMsgCnt == strLowerMessage.length())/*.ct后面什么都没有*/
				{
					intcocktail = RandomGenerator::Randint(1, (*CocktailList).size());
					string RandomCockReply = "品味生活！已为" + strNickName + "调好最合适的饮品：\n" + CocktailList->at(intcocktail).CocktailENName + "（" + CocktailList->at(intcocktail).CockTailCNName + "）" + CocktailList->at(intcocktail).CocktailData + "\n描述仅供参考";
					e.sender.sendMsg(RandomCockReply);
					return;
				}
				string strCocktailName;
				while (intMsgCnt != strLowerMessage.length())
				{
					strCocktailName += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				while (strCocktailName.find(" ") != string::npos)
					strCocktailName.erase(strCocktailName.find(" "), 1);
				string TemCocktailName;
				if (CNCockList.count(strCocktailName))
				{
					intcocktail = CNCockList[strCocktailName];
					string RandomCockReply = "品味生活！已为" + strNickName + "调好最合适的饮品：\n" + CocktailList->at(intcocktail).CocktailENName + "（" + CocktailList->at(intcocktail).CockTailCNName + "）" + CocktailList->at(intcocktail).CocktailData + "\n描述仅供参考";
					e.sender.sendMsg(RandomCockReply);
					return;
				}
				if (ENCockList.count(strCocktailName))
				{
					intcocktail = ENCockList[strCocktailName];
					string RandomCockReply = "品味生活！已为" + strNickName + "调好最合适的饮品：\n" + CocktailList->at(intcocktail).CocktailENName + "（" + CocktailList->at(intcocktail).CockTailCNName + "）" + CocktailList->at(intcocktail).CocktailData + "\n描述仅供参考";
					e.sender.sendMsg(RandomCockReply);
					return;
				}
				map<const int, CocktailType>::iterator CocktailCount = CocktailList->begin();
				while (CocktailCount != CocktailList->end())
				{
					TemCocktailName = CDs_tolower(CocktailCount->second.CocktailENName) + CocktailCount->second.CockTailCNName;
					if (TemCocktailName.find(strCocktailName) != string::npos)
					{
						string RandomCockReply = "品味生活！为" + strNickName + "调好已点单的饮品：\n" + CocktailCount->second.CocktailENName + "（" + CocktailCount->second.CockTailCNName + "）" + CocktailCount->second.CocktailData + "\n描述仅供参考";
						e.sender.sendMsg(RandomCockReply);
						return;
					}
					CocktailCount++;
				}
				e.sender.sendMsg("该酒品不存在！");
			}
		    else if (strLowerMessage.substr(intMsgCnt, 2) == "so")/*浅草寺100签*/
		   {
			  const int intsortilege = RandomGenerator::Randint(1, 100);
			   string Sortilege = "（晃动签桶）......那么神明给" + strNickName + "的指引是.....\n" + "第" + std::to_string(intsortilege) + "签  " + SensojiTempleDivineSign[intsortilege];
			   e.sender.sendMsg(Sortilege);
		   }
			else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
		   {
				intMsgCnt += 2;
				bool setporb = 0, isPunish = 1;/*下面是修改的部分mark(包括本行*/
				string strpbNum;
				int intpbNum = 1;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b')
				{
					setporb = 1;
					if (strLowerMessage[intMsgCnt] == 'b')
						isPunish = 0;
					intMsgCnt++;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						intMsgCnt++;
					while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					{
						strpbNum += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					if (strpbNum.length())
						intpbNum = stoi(strpbNum);
					if (intpbNum > 10)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.掷骰次数过多]);
						return;
					}
					if (!intpbNum)
						setporb = 0;
				}/*上面是修改的部分mark(包括本行*/
				string strSkillName;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
					isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
					':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')/*添加了+-判定*/
				{
					strSkillName += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];/*取得标准技能名称*/
				signed int intcorrection = 0;/*补正数据读取*/
				if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
				{
					if (strLowerMessage[intMsgCnt] == '+')
					{
						intcorrection = 1;
					}
					else
					{
						intcorrection = -1;
					}
					intMsgCnt++;
					string strCorrection;
					while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					{
						strCorrection += strLowerMessage[intMsgCnt];
						intMsgCnt++;
					}
					if (strCorrection.empty())
						intcorrection = 0;
					else if (strCorrection.length() > 2)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
						return;
					}
					else
					{
						intcorrection *= stoi(strCorrection);
					}
				}/*补正数据读取结束*/
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')
					intMsgCnt++;
				string strSkillVal;
				while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strSkillVal += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					intMsgCnt++;
				}
				string strReason = msg.substr(intMsgCnt);
				int intSkillVal;
				if (strSkillVal.empty())
				{					
					if (CharacterProp.count(e.sender.id()) && CharacterProp[e.sender.id()].count(strSkillName))
					{
						intSkillVal = CharacterProp[e.sender.id()][strSkillName];
					}
					else if (SkillDefaultVal.count(strSkillName))
					{
						intSkillVal = SkillDefaultVal[strSkillName];
					}
					else
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
						return;
					}
					
				}
				else if (strSkillVal.length() > 3)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
					return;
				}
				else
				{
					intSkillVal = stoi(strSkillVal);
				}
				if (intcorrection)/*数据补正*/
				{
					intSkillVal += intcorrection;
				}
				int intD100Res = RandomGenerator::Randint(1, 100);
				string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res);/*下面是修改的部分mark(包括本行*/
				if (setporb)
				{
					int pbRandom, single_figures;
					string pbShow = "";
					if (intD100Res == 100)
						single_figures = 0;
					else
						single_figures = intD100Res % 10;
					if (isPunish)
					{
						pbShow = "（惩罚骰：";
						for (int pbCunt = 0; pbCunt < intpbNum; pbCunt++)
						{
							pbRandom = RandomGenerator::Randint(0, 9);
							pbShow = pbShow + " " + to_string(pbRandom);
							if ((pbRandom == 0) && (single_figures == 0))
								pbRandom = 10;
							pbRandom = pbRandom * 10;
							if (pbRandom > intD100Res)
								intD100Res = pbRandom + single_figures;
						}
					}
					else
					{
						pbShow = "（奖励骰：";
						for (int pbCunt = 0; pbCunt < intpbNum; pbCunt++)
						{
							pbRandom = RandomGenerator::Randint(0, 9);
							pbShow = pbShow + " " + to_string(pbRandom);
							if ((pbRandom == 0) && (single_figures == 0))
								pbRandom = 10;
							pbRandom = pbRandom * 10;
							if (pbRandom < intD100Res)
								intD100Res = pbRandom + single_figures;
						}
					}
					pbShow = pbShow + "），最终结果是：" + to_string(intD100Res);
					strReply += pbShow + "/" + to_string(intSkillVal) + " ";
				}
				else
					strReply += "/" + to_string(intSkillVal) + " ";
				int RoomRuleNum = 5;/*自定义房规部分（包括此行向下*/
				/*上面是修改的部分mark(包括本行*/

				if (RoomRuleNum == 101)//公式书规则检定
				{
					if (intD100Res == 1 && intD100Res <= intSkillVal)
					{
						strReply += GlobalMsg[EnumInfoMsg.大成功];
					}
					else if (intD100Res <= (intSkillVal / 5))
					{
						strReply += GlobalMsg[EnumInfoMsg.极难成功];
					}
					else if (intD100Res <= (intSkillVal / 2))
					{
						strReply += GlobalMsg[EnumInfoMsg.困难成功];
					}
					else if (intD100Res <= intSkillVal)
					{
						strReply += GlobalMsg[EnumInfoMsg.成功];
					}
					else
					{
						if (50 <= intSkillVal)/*技能大于50*/
						{
							if (intD100Res <= 99)
							{
								strReply += GlobalMsg[EnumInfoMsg.失败];
							}
							else
							{
								strReply += GlobalMsg[EnumInfoMsg.大失败];
							}
						}
						else
						{
							if (intD100Res <= 95)
							{
								strReply += GlobalMsg[EnumInfoMsg.失败];
							}
							else
							{
								strReply += GlobalMsg[EnumInfoMsg.大失败];
							}
						}
					}
				}
				else
				{
					if (intD100Res <= intSkillVal)/*成功*/
					{
						if (intD100Res <= RoomRuleNum)
						{
							strReply += GlobalMsg[EnumInfoMsg.大成功];
						}
						else if (intD100Res <= intSkillVal / 5)
						{
							strReply += GlobalMsg[EnumInfoMsg.极难成功];
						}
						else if (intD100Res <= intSkillVal / 2)
						{
							strReply += GlobalMsg[EnumInfoMsg.困难成功];
						}
						else
						{
							strReply += GlobalMsg[EnumInfoMsg.成功];
						}
					}
					else/*失败*/
					{
						if (intD100Res >= (101 - RoomRuleNum))
						{
							strReply += GlobalMsg[EnumInfoMsg.大失败];
						}
						else
						{
							strReply += GlobalMsg[EnumInfoMsg.失败];
						}
					}
				}

				if (!strReason.empty())
				{
					strReply = "由于" + strReason + " " + strReply;
				}
				e.sender.sendMsg(strReply);
		   }
		    else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
		   {
			  intMsgCnt += 3;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			  string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
			  while (strDefaultDice[0] == '0')
				   strDefaultDice.erase(strDefaultDice.begin());
			   if (strDefaultDice.empty())
				   strDefaultDice = "100";
			   for (auto charNumElement : strDefaultDice)
				   if (!isdigit(static_cast<unsigned char>(charNumElement)))
				   {
					   e.sender.sendMsg(GlobalMsg[EnumErrorMsg.默认骰面数为0]);
					   return;
				   }
			   if (strDefaultDice.length() > 5)
			   {
				   e.sender.sendMsg(GlobalMsg[EnumErrorMsg.默认骰面数过多]);
				   return;
			   }
			   const int intDefaultDice = stoi(strDefaultDice);
			   if (intDefaultDice == 100)
				   DefaultDice.erase(e.sender.id());
			    else
				   DefaultDice[e.sender.id()] = intDefaultDice;
			   const string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
			   e.sender.sendMsg(strSetSuccessReply);
		   }
			else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
			{
				intMsgCnt += 4;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;

				string type;
				while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					type += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}

				auto nameType = NameGenerator::Type::UNKNOWN;
				if (type == "cn")
					nameType = NameGenerator::Type::CN;
				else if (type == "en")
					nameType = NameGenerator::Type::EN;
				else if (type == "jp")
					nameType = NameGenerator::Type::JP;

				while (isspace(static_cast<unsigned char>(msg[intMsgCnt])))
					intMsgCnt++;

				string strNum;
				while (isdigit(static_cast<unsigned char>(msg[intMsgCnt])))
				{
					strNum += msg[intMsgCnt];
					intMsgCnt++;
				}
				if (strNum.size() > 2)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.命名人数过多]);
					return;
				}
				int intNum = stoi(strNum.empty() ? "1" : strNum);
				if (intNum > 10)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.命名人数过多]);
					return;
				}
				if (intNum == 0)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量为0]);
					return;
				}
				vector<string> TempNameStorage;
				while (TempNameStorage.size() != intNum)
				{
					string name = NameGenerator::getRandomName(nameType);
					if (find(TempNameStorage.begin(), TempNameStorage.end(), name) == TempNameStorage.end())
					{
						TempNameStorage.push_back(name);
					}
				}
				string strReply = "既然你让我起名了，那么 " + strNickName + " 的随机名称就这些吧:\n";
				for (auto i = 0; i != TempNameStorage.size(); i++)
				{
					strReply.append(TempNameStorage[i]);
					if (i != TempNameStorage.size() - 1)strReply.append(", ");
				}
				e.sender.sendMsg(strReply);
			}
			else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
			{
				intMsgCnt += 2;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				string strSuccessSc, strFailSc, strSan;
				int intSan;
				while (intMsgCnt != strLowerMessage.length() &&
					(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'))
				{
					strSuccessSc += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strSuccessSc.length() == 0)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
					return;
				}
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				if (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '/')
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
					return;
				}
				else
				{
					intMsgCnt++;
				}
				while (intMsgCnt != strLowerMessage.length() &&
					(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'))
				{
					strFailSc += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strFailSc.length() == 0)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
					return;
				}
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strSan += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				//用于判断从哪里取得的san值，0为直接取得，1为在角色卡中取得，2为在默认卡中取得
				int intWhereComesSan = 0;
				if (strSan.length() != 0)
				{
					if (strSan.length() > 3)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SCsan值错误]);
						return;
					}
					intSan = stoi(strSan);
				}
				if (CharacterProp.count(e.sender.id())
					&& CharacterProp[e.sender.id()].count("理智"))
				{
					intSan = CharacterProp[e.sender.id()]["理智"];
					intWhereComesSan = 2;
				}
				else
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SCsan值错误]);
					return;
				}
				string strReply = strNickName + "的Sancheck:\n1D100=";
				int intD100Res = RandomGenerator::Randint(1, 100);
				strReply = strReply + to_string(intD100Res) + "/" + to_string(intSan);

				int RoomRuleNum = 5;/*自定义房规部分（包括此行向下*/
				/*上面是修改的部分mark(包括本行*/

				if (RoomRuleNum == 101)//公式书规则检定
				{
					if (intD100Res <= intSan)
					{
						DiceCalculatorOutput OutputReply = DiceCalculator(strSuccessSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (OutputReply.complate == false)
						{
							e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
							return;
						}
						intSan -= OutputReply.result;
						if (intSan < 0)
						{
							intSan = 0;
						}
						strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc成功] +
							"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
					}
					else
					{
						if (50 <= intSan)/*技能大于50*/
						{
							if (intD100Res <= 99)
							{
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
							else
							{
								while (strFailSc.find("d") != string::npos)
								{
									int intdPos = strFailSc.find("d");
									if (intdPos != 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, "*");
										}
										else if (!isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && !isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos == 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos != 0 && intdPos == strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else
									{
										strFailSc = DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100";
									}
								}
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc大失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
						}
						else
						{
							if (intD100Res <= 95)
							{
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
							else
							{
								while (strFailSc.find("d") != string::npos)
								{
									int intdPos = strFailSc.find("d");
									if (intdPos != 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, "*");
										}
										else if (!isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && !isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos == 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos != 0 && intdPos == strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else
									{
										strFailSc = DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100";
									}
								}
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc大失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
						}
					}
				}
				else
				{
					if (intD100Res <= intSan)/*成功*/
					{
						DiceCalculatorOutput OutputReply = DiceCalculator(strSuccessSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (OutputReply.complate == false)
						{
							e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
							return;
						}
						intSan -= OutputReply.result;
						if (intSan < 0)
						{
							intSan = 0;
						}
						strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc成功] +
							"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
					}
					else/*失败*/
					{
						if (intD100Res >= (101 - RoomRuleNum))
						{
							while (strFailSc.find("d") != string::npos)
							{
								int intdPos = strFailSc.find("d");
								if (intdPos != 0 && intdPos != strFailSc.length() - 1)
								{
									if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.replace(intdPos, 1, "*");
									}
									else if (!isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.erase(intdPos, 1);
									}
									else if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && !isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
									}
									else
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
									}
								}
								else if (intdPos == 0 && intdPos != strFailSc.length() - 1)
								{
									if (isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.erase(intdPos, 1);
									}
									else
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
									}
								}
								else if (intdPos != 0 && intdPos == strFailSc.length() - 1)
								{
									if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])))
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
									}
									else
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
									}
								}
								else
								{
									strFailSc = DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100";
								}
							}
							DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
							if (OutputReply.complate == false)
							{
								e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
								return;
							}
							intSan -= OutputReply.result;
							if (intSan < 0)
							{
								intSan = 0;
							}
							strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc大失败] +
								"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
						}
						else
						{
							DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
							if (OutputReply.complate == false)
							{
								e.sender.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
								return;
							}
							intSan -= OutputReply.result;
							if (intSan < 0)
							{
								intSan = 0;
							}
							strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc失败] +
								"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
						}
					}
				}
			    if (intWhereComesSan == 2)
				{
					CharacterProp[e.sender.id()]["理智"] = intSan;
				}
				else
				{
					//啥都不干
				}
				e.sender.sendMsg(strReply);
			}
			else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
			{
				intMsgCnt += 2;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				string strSkillName;
				while (intMsgCnt != strLowerMessage.length() &&
					!isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
					&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '+'
					&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '-' && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '*')
				{
					strSkillName += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strSkillName.length() == 0)
				{
					e.sender.sendMsg(GlobalMsg[EnumErrorMsg.EN指令格式错误]);
					return;
				}
				if (SkillNameReplace.count(strSkillName))
					strSkillName = SkillNameReplace[strSkillName];
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				string strSkillVal;
				while (intMsgCnt != strLowerMessage.length() &&
					isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strSkillVal += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				//用于判断从哪里取得的技能值，0为直接取得，1为在角色卡中取得，2为在默认卡中取得
				int intWhereComesSkill = 0;
				int intSkillVal;
				if (strSkillVal.empty())
				{
					intWhereComesSkill = 2;
					if (CharacterProp.count(e.sender.id()) && CharacterProp[e.sender.id()].count(strSkillName))
					{
						intSkillVal = CharacterProp[e.sender.id()][strSkillName];
					}
					else if (SkillDefaultVal.count(strSkillName))
					{
						intSkillVal = SkillDefaultVal[strSkillName];
					}
					else
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
						return;
					}
				}
				else
				{
					if (strSkillVal.length() > 3)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
						return;
					}
					intSkillVal = stoi(strSkillVal);
				}
				string strEnEquation;
				while (intMsgCnt != strLowerMessage.length() &&
					(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
						|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'))
				{
					strEnEquation += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				int intdD100Res = RandomGenerator::Randint(1, 100);
				string strAns = strNickName + "的" + strSkillName + "增强或成长检定:1D100=" + to_string(intdD100Res) + "/" + to_string(intSkillVal) + "\n";
				if (intdD100Res > intSkillVal || intdD100Res >= 95)
				{
					int intENresult = 0;
					if (strEnEquation.empty())
					{
						intENresult = RandomGenerator::Randint(1, 10);
						intSkillVal += intENresult;
					}
					else
					{
						DiceCalculatorOutput ENRecult = DiceCalculator(strEnEquation, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (!ENRecult.complate)
						{
							e.sender.sendMsg(GlobalMsg[EnumErrorMsg.EN指令格式错误]);
							return;
						}
						intENresult = ENRecult.result;
						intSkillVal += intENresult;
					}
					strAns = strAns + GlobalMsg[EnumInfoMsg.en成功] + ",你的" + strSkillName + "成长了" + to_string(intENresult) + "点，当前值为" + to_string(intSkillVal) + "点";
					if (intWhereComesSkill == 2)
					{
						if (SkillDefaultVal.count(strSkillName) && SkillDefaultVal[strSkillName] == intSkillVal)
						{
							CharacterProp[e.sender.id()].erase(strSkillName);
						}
						else
						{
							CharacterProp[e.sender.id()][strSkillName] = intSkillVal;
						}
					}
					e.sender.sendMsg(strAns);
				}
				else
				{
					strAns = strAns + GlobalMsg[EnumInfoMsg.en失败];
					e.sender.sendMsg(strAns);
				}
			}
			else if (strLowerMessage.substr(intMsgCnt, 6) == "master")
			{
				if (e.sender.id() != MasterQQId)
				{
					e.sender.sendMsg("前面是工作区域，请顾客止步哦");
					return;
				}
				intMsgCnt += 6;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				if (strLowerMessage.substr(intMsgCnt, 3) == "cir")
				{
					intMsgCnt += 3;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						intMsgCnt++;
					if (strLowerMessage.substr(intMsgCnt, 2) == "on")
					{
						if (boolRunCirculate)
						{
							e.sender.sendMsg("时钟已经在运行了");
							return;
						}
						else
						{
							boolRunCirculate = true;
							e.sender.sendMsg("时钟已启动");
							thread thrMainCirculate(MainCirculate, MasterQQId, e.bot.id);
							thrMainCirculate.join();
							return;
						}
					}
					else if (strLowerMessage.substr(intMsgCnt, 3) == "off")
					{
						if (!boolRunCirculate)
						{
							e.sender.sendMsg("时钟已经关闭了");
							return;
						}
						else
						{
							boolRunCirculate = false;
							e.sender.sendMsg("时钟已关闭");
							return;
						}
					}
					else
					{
						return;
					}
				}
				else if (strLowerMessage.substr(intMsgCnt, 4) == "list")
				{
					logger->info("——————————群组列表——————————");
					vector<unsigned long long>ReplyGroupList = e.bot.getGroupList();
					for (vector<unsigned long long>::iterator i = ReplyGroupList.begin(); i != ReplyGroupList.end(); i++)
					{
						logger->info(to_string(*i) + "\t" + Group(*i, e.bot.id).nickOrNameCard());
					}
					logger->info("——————————好友列表——————————");
					vector<unsigned long long>ReplyFriendList = e.bot.getFriendList();
					for (vector<unsigned long long>::iterator i = ReplyFriendList.begin(); i != ReplyFriendList.end(); i++)
					{
						logger->info(to_string(*i) + "\t" + Friend(*i, e.bot.id).nickOrNameCard());
					}
					logger->info("——————————列表结束——————————");
				}
				else if (strLowerMessage.substr(intMsgCnt, 3) == "lmt")
				{
					intMsgCnt += 3;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						intMsgCnt++;
					if (strLowerMessage.substr(intMsgCnt, 2) == "on")
					{
						if (boolRunLmt)
						{
							e.sender.sendMsg("LMT已经在运行了");
							return;
						}
						else
						{
							boolRunLmt = true;
							e.sender.sendMsg("LMT已启动");
							return;
						}
					}
					else if (strLowerMessage.substr(intMsgCnt, 3) == "off")
					{
						if (!boolRunLmt)
						{
							e.sender.sendMsg("LMT已经关闭了");
							return;
						}
						else
						{
							boolRunLmt = true;
							e.sender.sendMsg("LMT已停用");
							return;
						}
					}
					else if (strLowerMessage.substr(intMsgCnt, 3) == "add")
					{
						intMsgCnt += 3;
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
							intMsgCnt++;
						string strGroupNum;
						while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						{
							strGroupNum += strLowerMessage[intMsgCnt];
							intMsgCnt++;
						}
						if (strGroupNum.empty())
						{
							e.sender.sendMsg("试图加入LMT白名单的群号为空");
							return;
						}
						int intGroupNum = stoi(strGroupNum);
						if (LMTWhiteList.count(intGroupNum))
						{
							e.sender.sendMsg("该群已存在在LMT白名单中");
							return;
						}
						LMTWhiteList.insert(intGroupNum);
						if (LmtGroupList.count(intGroupNum))
						{
							LmtGroupList.erase(intGroupNum);
						}
						e.sender.sendMsg("已将" + strGroupNum + "加入白名单");
						return;
					}
					else if (strLowerMessage.substr(intMsgCnt, 3) == "rmv")
					{
						intMsgCnt += 3;
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
							intMsgCnt++;
						string strGroupNum;
						while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						{
							strGroupNum += strLowerMessage[intMsgCnt];
							intMsgCnt++;
						}
						if (strGroupNum.empty())
						{
							e.sender.sendMsg("试图移除LMT白名单的群号为空");
							return;
						}
						int intGroupNum = stoi(strGroupNum);
						if (!LMTWhiteList.count(intGroupNum))
						{
							e.sender.sendMsg("该群不在LMT白名单中");
							return;
						}
						LMTWhiteList.erase(intGroupNum);
						IdleTimer(intGroupNum);
						e.sender.sendMsg("已将" + strGroupNum + "从白名单移除");
						return;
					}
					else
					{
						return;
					}
				}
				else if (strLowerMessage.substr(intMsgCnt, 2) == "to")
			    {
					intMsgCnt += 2;
					while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						intMsgCnt++;
					if (strLowerMessage.substr(intMsgCnt, 5) == "group")
					{
						intMsgCnt += 5;
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
							intMsgCnt++;
						string strGroupId;
						while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						{
							strGroupId += strLowerMessage[intMsgCnt];
							intMsgCnt++;
						}
						if (strGroupId.empty())
						{
							e.sender.sendMsg("未找到群号");
							return;
						}
						unsigned long long GroupId = stoi(strGroupId);
						vector<unsigned long long>GroupList = e.bot.getGroupList();
						vector<unsigned long long>::iterator result = find(GroupList.begin(), GroupList.end(), GroupId);
						if (result == GroupList.end())//检测群列表里是否存在该群
						{
							e.sender.sendMsg("您拨打的群号是空号，请查证后再拨");
							return;
						}
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
							intMsgCnt++;
						if (intMsgCnt != msg.length())
						{
							Group(GroupId, e.bot.id).sendMsg("来自Master的消息：" + msg.substr(intMsgCnt));
							e.sender.sendMsg("消息已发送往" + Group(GroupId, e.bot.id).nickOrNameCard() + strGroupId);
						}
						else
						{
							e.sender.sendMsg("留言为空");
						}
					}
					else if (strLowerMessage.substr(intMsgCnt, 6) == "friend")
					{
						intMsgCnt += 6;
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
							intMsgCnt++;
						string strQQId;
						while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						{
							strQQId += strLowerMessage[intMsgCnt];
							intMsgCnt++;
						}
						if (strQQId.empty())
						{
							e.sender.sendMsg("未找到好友ID");
							return;
						}
						unsigned long long QQId = stoi(strQQId);
						vector<unsigned long long>FriendList = e.bot.getFriendList();
						vector<unsigned long long>::iterator result = find(FriendList.begin(), FriendList.end(), QQId);
						if (result == FriendList.end())//检测群列表里是否存在该群
						{
							e.sender.sendMsg("您拨打的好友是空号，请查证后再拨");
							return;
						}
						while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
							intMsgCnt++;
						if (intMsgCnt != msg.length())
						{
							Friend(QQId, e.bot.id).sendMsg("来自Master的消息：" + msg.substr(intMsgCnt));
							e.sender.sendMsg("消息已发送往" + Friend(QQId, e.bot.id).nickOrNameCard() + strQQId);
						}
						else
						{
							e.sender.sendMsg("留言为空");
						}
					}
					else
					 {

					 }
				}
				else
				{
					return;
				}

			}
			else if (strLowerMessage.substr(intMsgCnt, 8) == "tomaster")
		   {
			 intMsgCnt += 8;
			  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			  if (intMsgCnt != msg.length())
			  {
				   Friend(MasterQQId, e.bot.id).sendMsg("来自群" + e.sender.nickOrNameCard() + to_string(e.sender.id()) + "的消息\n" + msg.substr(intMsgCnt));
				   e.sender.sendMsg("好的，话我已经传达给柴刀了");
			   }
			   else
			   {
				   e.sender.sendMsg("我不知道你想告诉柴刀什么");
			   }
			}
			else if (strLowerMessage[intMsgCnt] == 'n')
			{
				intMsgCnt++;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				if (intMsgCnt != strLowerMessage.length())
				{
					string strNickName;
					strNickName = msg.substr(intMsgCnt);
					if (strNickName.length() >= 50)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.昵称太长]);
						return;
					}
					GlobalNickName[e.sender.id()] = strNickName;
					e.sender.sendMsg(GlobalMsg[EnumInfoMsg.全局昵称设置成功] + strNickName);
				}
				else
				{
					if (GlobalNickName.count(e.sender.id()))
					{
						GlobalNickName.erase(e.sender.id());
						e.sender.sendMsg(GlobalMsg[EnumInfoMsg.全局昵称删除成功]);
					}
					else
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.删除昵称不存在]);
					}
				}
					
				
			}
			else if (strLowerMessage[intMsgCnt] == 'r')
			{
				intMsgCnt++;
				bool isHiden = false, showDetail = false;
				while (intMsgCnt != strLowerMessage.length()
					&& (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'h' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 's'))
				{
					if (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'h')
					{
						isHiden = true;
					}
					if (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 's')
					{
						showDetail = true;
					}
					intMsgCnt++;
				}
				int intMultDice = 1, intFindMultDiceCut = intMsgCnt;
				string strMultDice;
				while (intFindMultDiceCut != strLowerMessage.length()
					&& (static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut]) != '#' || isdigit(static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut])))
					&& intFindMultDiceCut <= intMsgCnt + 3)
				{
					if (isdigit(static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut])))
					{
						strMultDice += strLowerMessage[intFindMultDiceCut];
					}
					intFindMultDiceCut++;
				}
				if (intFindMultDiceCut != strLowerMessage.length() && static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut]) == '#')
				{
					if (strMultDice.length() == 0)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.掷骰轮数无效]);
						return;
					}
					intMultDice = stoi(strMultDice);
					if (intMultDice > 10)
					{
						e.sender.sendMsg(GlobalMsg[EnumErrorMsg.掷骰轮数过多]);
						return;
					}
					intMsgCnt = intFindMultDiceCut;
					intMsgCnt++;
				}

				//替换中文括号——————————————————————————
				string strEquation, strReason;
				string leftBrackens = "（", rightBrackens = "）";
				while (strLowerMessage.find("（") != string::npos)
					strLowerMessage.replace(strLowerMessage.find(leftBrackens), leftBrackens.length(), "(");
				while (strLowerMessage.find("）") != string::npos)
					strLowerMessage.replace(strLowerMessage.find(rightBrackens), rightBrackens.length(), ")");
				while (isspace(static_cast<unsigned char>(msg[intMsgCnt])))
					intMsgCnt++;
				//取得算式与原因,并判断算式是否存在——————————————————

				while (intMsgCnt != strLowerMessage.length() && (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '('
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == ')'))
				{
					strEquation += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strEquation.length() == 0)
				{
					if (DefaultDice.count(e.sender.id()))
					{
						strEquation = "d" + to_string(DefaultDice[e.sender.id()]);
					}
					else
						strEquation = "d100";
				}
				if (intMsgCnt != strLowerMessage.length())
					strReason = msg.substr(intMsgCnt);



				string strReply;
				if (strReason.length() != 0)
				{
					strReply = "由于" + strReason + ",";
				}
				strReply = strReply + strNickName + "骰出了：";

				if (showDetail)
				{
					for (int MultDiceCut = 1; MultDiceCut <= intMultDice; MultDiceCut++)
					{
						DiceCalculatorOutput OutputReply = DiceCalculator(strEquation, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (!OutputReply.complate)
						{
							e.sender.sendMsg(OutputReply.detail);
							return;
						}
						strReply = strReply + "\n" + OutputReply.detail;
					}
				}
				else
				{
					for (int MultDiceCut = 1; MultDiceCut <= intMultDice; MultDiceCut++)
					{
						DiceCalculatorOutput OutputReply = DiceCalculator(strEquation, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (!OutputReply.complate)
						{
							e.sender.sendMsg(OutputReply.detail);
							return;
						}
						strReply = strReply + "\n" + strEquation + "=" + to_string(OutputReply.result);
					}
				}
				if (isHiden)
				{
					e.sender.sendMsg(strReply);
				}
				else
				{
					e.sender.sendMsg(strReply);
				}
			}
			else
		    {
				 return;
		    }
		    return;
        });

        // 监听群信息
        procession->registerEvent<GroupMessageEvent>([=](GroupMessageEvent e) {
#ifdef groupExample



            e.group.sendMsg("--开始测试--");
           //  e.group.sendMsg(e.sender.at() + "发送纯文本MiraiCode");
           //  e.group.sendMiraiCode(e.sender.at() + "发送MiraiCode");
           //  e.group.sendMsg("禁言测试");
           //  try{
           //      e.sender.mute(60*2);
           //      e.sender.mute(0);
           //  }catch(BotException& b){e.group.sendMsg("权限不足");}
           //  if(!e.group.announcements.empty())
           //      e.group.sendMsg("第一个群公告内容: " + e.group.announcements[0].content);
           //  e.group.sendMsg("消息中全部at对象: ");
           //  try{
           //      e.group.sendMsg(e.message.content.filter(MiraiCP::MiraiCode::at));
           //  }catch(IllegalArgumentException& i){}
           //  e.group.sendMsg("群成员列表");
           //  e.group.sendMsg(e.group.MemberListToString());
           //  e.group.sendMsg("发送一个群公告并删除");
           //  Group::OfflineAnnouncement("Helloooooooo!", Group::AnnouncementParams()).publishTo(e.group).deleteThis();
           //  logger->info("Global全局日志");
           //  Main::pluginLogger->info("Plugin插件日志");
           //  e.botlogger.info("bot机器人日志");
           //  e.botlogger.info("上一条信息:"+e.message.content.toString());
           //  e.botlogger.info("上一条信息:"+e.message.content.toMiraiCode());
           //  e.group.sendMiraiCode(MiraiCode("[mirai:service:1,<?xml version=\"1.0\" encoding=\"utf-8\"?>\\n<msg templateID=\"12345\" action=\"web\" brief=\"简介 没点进来看见的样子\" serviceID=\"1\" url=\"https://github.com/\"><item layout=\"2\"><picture cover=\"https://avatars.githubusercontent.com/u/35139537?s=400&u=c7e3794d25a2e0f27f15caf5ba7a57c7346962f0&v=4\"/><title>标题</title><summary>描述文字</summary></item><source/></msg>\\n]"));
           //  e.group.sendMiraiCode(MiraiCode(new ServiceMessage(URLSharer())));
           //  e.group.sendMsg("上下文测试，在每接收一个'a'就加1");
           // if(e.message.content.toString() == "a"){
           //     if(!e.getContext().content.contains("count"))
           //         e.getContext().content["count"] = 1;
           //     else
           //         e.getContext().content["count"] = e.getContext().content["count"].get<int>() + 1;
           // }
           // if(e.getContext().content.contains("count"))
           //     e.group.sendMsg(e.getContext().content["count"].get<int>());
           // e.group.sendMsg("发送语音测试:");
           // e.group.sendVoice(R"(D:\下载缓存\test.amr)");
           // e.group.sendMsg("UTF8 emoji测试: ☺");
           // e.group.sendMsg("群名称，并改名为'x':"+e.group.setting.name);
           // e.group.setting.name = "x";
           // try{
           //     e.group.updateSetting();
           // }catch(BotException& b){e.group.sendMsg("没有权限");}
           // e.message.source.quoteAndSendMsg("引用测试");
           // e.botlogger.info("messageSource: "+e.message.source.serializeToString());
           // e.group.sendMsg("撤回测试:");
           // e.group.sendMsg("撤回测试").recall();
           e.group.sendMsg("发送卡片:");
           e.group.sendMiraiCode(new LightApp(LightAppStyle1()));
           e.group.sendMiraiCode(LightApp(LightAppStyle2()).toMiraiCode());
           e.group.sendMiraiCode(new LightApp(LightApp(LightAppStyle3())));
           e.group.sendMiraiCode(new LightApp(LightAppStyle4()));
           schedule(10000, "aaa");
           // e.group.sendMsg("转发测试:");
           // ForwardMessage(&e.group,
           //                {
           //     ForwardNode(1930893235, "Eritque arcus", "hahaha", 1),
           //     ForwardNode(1930893235, "Eritque arcus", "hahaha", -100)
           //                }).sendTo(&e.group);
           // e.group.sendMsg("bot属性");
           // e.sender.sendMsg(e.bot.nick());
           // e.sender.sendMsg(e.bot.FriendListToString());
           // e.sender.sendMsg(e.bot.GroupListToString());
#endif // groupExample

		   string msg = e.message.content.toString();
		   while (isspace(static_cast<unsigned char>(msg[0])))
			   msg.erase(msg.begin());
		   string strAt = "[mirai:at:" + to_string(e.bot.id) + "]";
		   bool botAt = false;
		   if (msg.substr(0, 10) == "[mirai:at:")
		   {
			   if (msg.substr(0, strAt.length()) != strAt)
			   {
				   return;
			   }
			   botAt = true;
			   msg = msg.substr(strAt.length());
			   while (isspace(static_cast<unsigned char>(msg[0])))
				   msg.erase(msg.begin());
		   }
		   string FullStop = "。";
		   if (msg.substr(0, FullStop.length()) == FullStop)
		   {
			   msg.replace(0, FullStop.length(), ".");
		   }
		   if (msg[0] != '.')
		   {
			   return;
		   }
		   int intMsgCnt = 1;
		   while (isspace(static_cast<unsigned char>(msg[intMsgCnt])))
			   intMsgCnt++;
		   string strNickName;
		   if (GroupNickNameList.count(e.sender.id()) && GroupNickNameList[e.sender.id()].count(e.group.id()))
		   {
			   strNickName = GroupNickNameList[e.sender.id()][e.group.id()];
		   }
		   else if (PlayerGroupList.count(e.sender.id()) && PlayerGroupList[e.sender.id()].count(e.group.id()))
		   {
			   strNickName = PlayerGroupList[e.sender.id()][e.group.id()];
		   }
		   else if (GlobalNickName.count(e.sender.id()))
		   {
			   strNickName = GlobalNickName[e.sender.id()];
		   }
		   else
		   {
			   strNickName = e.sender.nickOrNameCard();
		   }
		   if (e.sender.id() == MasterQQId)
		   {
			   e.sender.permission = 3;
		   }
		   string strLowerMessage = msg;
		   logger->info("指令：" + msg + "\t处理开始");
		   strLowerMessage = CDs_tolower(strLowerMessage);
		   if (strLowerMessage.substr(intMsgCnt, 3) == "bot")
		   {
			   intMsgCnt += 3;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   string Command;
			   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
				   static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			   {
				   Command += strLowerMessage[intMsgCnt];
				   intMsgCnt++;
			   }
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   string QQNum = strLowerMessage.substr(intMsgCnt, msg.find(' ', intMsgCnt) - intMsgCnt);
			   if (Command == "on")
			   {
				   if (QQNum.empty() || QQNum == to_string(e.bot.id))
				   {
					   if (e.sender.permission >= 1)
					   {
						   if (DisabledGroup.count(e.group.id()))
						   {
							   DisabledGroup.erase(e.group.id());
							   e.group.sendMsg(GlobalMsg[EnumInfoMsg.开启骰子]);
						   }
						   else
						   {
							   e.group.sendMsg(GlobalMsg[EnumErrorMsg.已在开启状态]);
						   }
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.权限不足]);
					   }
				   }
			   }
			   else if (Command == "off")
			   {
				   if (QQNum.empty() || QQNum == to_string(e.bot.id))
				   {
					   if (e.sender.permission >= 1)
					   {
						   if (!DisabledGroup.count(e.group.id()))
						   {
							   DisabledGroup.insert(e.group.id());
							   e.group.sendMsg(GlobalMsg[EnumInfoMsg.关闭骰子]);
						   }
						   else
						   {
							   e.group.sendMsg(GlobalMsg[EnumErrorMsg.已在关闭状态]);
						   }
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.权限不足]);
					   }
				   }
			   }
			   else
			   {
				   if (QQNum.empty() || QQNum == to_string(e.bot.id))
				   {
					   e.group.sendMsg(GlobalMsg[EnumInfoMsg.BOT]);
				   }
			   }
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 7) == "dismiss")
		   {
			   intMsgCnt += 7;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   string QQNum;
			   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			   {
				   QQNum += strLowerMessage[intMsgCnt];
				   intMsgCnt++;
			   }
			   if (QQNum.empty() || QQNum == to_string(e.bot.id))
			   {
				   if (e.sender.permission >= 1)
				   {
					   e.group.quit();
				   }
				   else
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.权限不足]);
				   }
			   }
		   }
		   else if (DisabledGroup.count(e.group.id())&&!botAt)
		   {
			   return;
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 4) == "help")
		   {
			   IdleTimer(e.group.id());
			   Image helptmp = e.group.uploadImg(R"(.\help.jpg)");
			   e.group.sendMiraiCode(helptmp.toMiraiCode());
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 7) == "welcome")
		   {
				IdleTimer(e.group.id());
			   intMsgCnt += 7;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   if (e.sender.permission >= 1)
			   {
				   string strWelcomeMsg = msg.substr(intMsgCnt);
				   if (strWelcomeMsg.empty())
				   {
					   if (WelcomeMsg.count(e.group.id()))
					   {
						   WelcomeMsg.erase(e.group.id());
						   e.group.sendMsg(GlobalMsg[EnumInfoMsg.欢迎信息清除]);
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.欢迎信息为空]);
					   }
				   }
				   else
				   {
					   WelcomeMsg[e.group.id()] = strWelcomeMsg;
					   e.group.sendMsg(GlobalMsg[EnumInfoMsg.设置欢迎信息]);
				   }
			   }
			   else
			   {
				   e.group.sendMsg(GlobalMsg[EnumErrorMsg.权限不足]);
			   }
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 4) == "jrrp")
		   {
			   int intJrrpKey;
			   SYSTEMTIME Time;
			   GetLocalTime(&Time);
			   if (JrrpMap.count(0))
			   {
				   if (JrrpMap[0] != Time.wDay)
				   {
					   JrrpMap.clear();
					   JrrpMap[0] = Time.wDay;
				   }
			   }
			   else
			   {
				   JrrpMap[0] = Time.wDay;
			   }
			   if (JrrpMap.count(e.sender.id()))
			   {
				   intJrrpKey = JrrpMap[e.sender.id()];
			   }
			   else
			   {
				   intJrrpKey = RandomGenerator::Randint(1, 100);
				   JrrpMap[e.sender.id()] = intJrrpKey;
			   }
			   string Sortilege = "（晃动签桶）......那么今天神明给" + strNickName + "的指引是.....\n" + "第" + std::to_string(intJrrpKey) + "签  " + SensojiTempleDivineSign[intJrrpKey];
			   e.group.sendMsg(Sortilege);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 3) == "coc")
		   {
		      IdleTimer(e.group.id());
			  intMsgCnt += 3;
			  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				  intMsgCnt++;
			  int intCocCreater;
			  string strCocCreater;
			  while (intMsgCnt != strLowerMessage.length() && isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			  {
				  strCocCreater += strLowerMessage[intMsgCnt];
				  if (strCocCreater.length() >= 3)
				  {
					  e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
					  return;
				  }
				  intMsgCnt++;
			  }
			  if (strCocCreater.empty())
			  {
				  intCocCreater = 1;
			  }
			  else
			  {
				  intCocCreater = stoi(strCocCreater);
				  if (intCocCreater > 10)
				  {
					  e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
					  return;
				  }
				  if (intCocCreater == 0)
				  {
					  e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量为0]);
					  return;
				  }
			  }
			  string strReply = "那么当调查员" + strNickName + "醒来，发现自己可能是这个样子";
			  for (int i = 1; i <= intCocCreater; i++)
			  {
				  string strOneCreat;
				  int intVal, intSum = 0, intSumLuck = 0;
				  intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "力量:" + to_string(intVal) + " ";
				  intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "体质:" + to_string(intVal) + " ";
				  intVal = SimpleDice(2, 6).result * 5 + 30; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "体型:" + to_string(intVal) + " ";
				  intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "敏捷:" + to_string(intVal) + " ";
				  intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "外貌:" + to_string(intVal) + " ";
				  intVal = SimpleDice(2, 6).result * 5 + 30; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "智力:" + to_string(intVal) + " ";
				  intVal = SimpleDice(3, 6).result * 5; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "意志:" + to_string(intVal) + " ";
				  intVal = SimpleDice(2, 6).result * 5 + 30; intSum += intVal; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "教育:" + to_string(intVal) + " ";
				  intVal = SimpleDice(3, 6).result * 5; intSumLuck += intVal;
				  strOneCreat = strOneCreat + "幸运:" + to_string(intVal) + " 共计:" + to_string(intSumLuck) + "(" + to_string(intSum) + ")";
				  strReply += "\n" + strOneCreat;
			  }
			  e.group.sendMsg(strReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 3) == "dnd")
		   {
				IdleTimer(e.group.id());
				intMsgCnt += 3;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				int intCocCreater;
				string strCocCreater;
				while (intMsgCnt != strLowerMessage.length() && isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strCocCreater += strLowerMessage[intMsgCnt];
					if (strCocCreater.length() >= 3)
					{
						e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
						return;
					}
					intMsgCnt++;
				}
				if (strCocCreater.empty())
				{
					intCocCreater = 1;
				}
				else
				{
					intCocCreater = stoi(strCocCreater);
					if (intCocCreater > 10)
					{
						e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量过多]);
						return;
					}
					if (intCocCreater == 0)
					{
						e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量为0]);
						return;
					}
				}
				string strReply = "那么当冒险者" + strNickName + "醒来，发现自己可能是这个样子";
				for (int i = 1; i <= intCocCreater; i++)
				{
					string strOneCreat;
					int intSum = 0;
					int intVal[4] = { RandomGenerator::Randint(1, 6), RandomGenerator::Randint(1, 6), RandomGenerator::Randint(1, 6), RandomGenerator::Randint(1, 6) };
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "力量:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "敏捷:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "体质:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "智力:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "感知:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " "; intSum = intSum + intVal[1] + intVal[2] + intVal[3];
					intVal[0] = RandomGenerator::Randint(1, 6); intVal[1] = RandomGenerator::Randint(1, 6); intVal[2] = RandomGenerator::Randint(1, 6); intVal[3] = RandomGenerator::Randint(1, 6);
					sort(intVal, intVal + 4);
					strOneCreat = strOneCreat + "魅力:" + to_string(intVal[1] + intVal[2] + intVal[3]) + " 总计" + to_string(intSum + intVal[1] + intVal[2] + intVal[3]);
					strReply += "\n" + strOneCreat;
				}
				e.group.sendMsg(strReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "ti")
		   {
		     IdleTimer(e.group.id());
			 intMsgCnt += 2;
			 string strReply = "好像有人疯了，" + strNickName + "的疯狂发作临时症状:D10=";
			 int intLi = RandomGenerator::Randint(1, 10);
			 strReply = strReply + to_string(intLi) + "\n";
			 string strLiTime = "D10=" + to_string(RandomGenerator::Randint(1, 10));
			 if (intLi == 9)
			 {
				 int intLi9 = RandomGenerator::Randint(1, 100);
				 strReply = strReply + format(TempInsanity[intLi], strLiTime, "D100=" + to_string(intLi9), strFear[intLi9]);
			 }
			 else if (intLi == 10)
			 {
				 int intLi10 = RandomGenerator::Randint(1, 100);
				 strReply = strReply + format(TempInsanity[intLi], strLiTime, "D100=" + to_string(intLi10), strPanic[intLi10]);
			 }
			 else
			 {
				 strReply = strReply + format(TempInsanity[intLi], strLiTime);
			 }
			 e.group.sendMsg(strReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "li")
		   {
			  IdleTimer(e.group.id());
			  intMsgCnt += 2;
			   string strReply = "与一位调查员失去联系，" + strNickName + "疯狂发作总结症状:D10=";
			   int intLi = RandomGenerator::Randint(1, 10);
			   strReply = strReply + to_string(intLi) + "\n";
			   string strLiTime = "D10=" + to_string(RandomGenerator::Randint(1, 10));
			   if (intLi == 9)
			   {
				   int intLi9 = RandomGenerator::Randint(1, 100);
				   strReply = strReply + format(LongInsanity[intLi], strLiTime, "D100=" + to_string(intLi9), strFear[intLi9]);
			   }
			   else if (intLi == 10)
			   {
				   int intLi10 = RandomGenerator::Randint(1, 100);
				   strReply = strReply + format(LongInsanity[intLi], strLiTime, "D100=" + to_string(intLi10), strPanic[intLi10]);
			   }
			   else
			   {
				   strReply = strReply + format(LongInsanity[intLi], strLiTime);
			   }
			   e.group.sendMsg(strReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "ri")
		   {
			   IdleTimer(e.group.id());
				 intMsgCnt += 2;
				 while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					 intMsgCnt++;
				 string strReply = "的先攻投点：D20";
				 bool boolcorrection = false;
				 int isPostive = 1;
				 if (strLowerMessage[intMsgCnt] == '-' || strLowerMessage[intMsgCnt] == '+')
				 {
					 boolcorrection = true;
					 if (strLowerMessage[intMsgCnt] == '-')
					 {
						 isPostive = -1;
						 strReply += '-';
					 }
					 else
					 {
						 strReply += '+';
					 }
				 }
				 int intCorrection = 0;
				 if (boolcorrection)
				 {
					 string strCorrection;
					 intMsgCnt++;
					 while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						 intMsgCnt++;
					 while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					 {
						 strCorrection += strLowerMessage[intMsgCnt];
						 intMsgCnt++;
					 }
					 if (strCorrection.empty())
					 {
						 e.group.sendMsg("先攻修正值错误");
						 return;
					 }
					 else if(strCorrection.length()>=3)
					 {
						 e.group.sendMsg("先攻修正值是不是有点太大了");
						 return;
					 }
					 else
					 {
						 strReply += strCorrection;
					 }
					 intCorrection = stoi(strCorrection);
				 }
				 while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					 intMsgCnt++;
				 string strInitNickName = msg.substr(intMsgCnt);
				 if (strInitNickName.empty())
				 {
					 strInitNickName = strNickName;
				 }
				 int intResult = RandomGenerator::Randint(1, 20) + (isPostive * intCorrection);
				 strReply = strReply + "=" + to_string(intResult);
				 InitList[e.group.id()][strInitNickName] = intResult;
				 e.group.sendMsg("唯快不破，" + strInitNickName + strReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 4) == "init")
		   {
			   IdleTimer(e.group.id());
			   intMsgCnt += 4;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
			   {
				   if (!InitList.count(e.group.id()))
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.先攻表为空]);
					   return;
				   }
				   InitList.erase(e.group.id());
				   e.group.sendMsg(GlobalMsg[EnumInfoMsg.清除先攻]);
				   return;
			   }
			   if (!InitList.count(e.group.id()))
			   {
				   e.group.sendMsg(GlobalMsg[EnumErrorMsg.先攻表为空]);
				   return;
			   }
			   multimap<int, string>temInitList;
			   string strReply = "先攻记录查询如下：";
			   for (map<string, int>::iterator InitListCut = InitList[e.group.id()].begin(); InitListCut != InitList[e.group.id()].end(); InitListCut++)
			   {
				   temInitList.insert({ InitListCut->second, InitListCut->first });
			   }
			   for (map<int, string>::iterator InitListCut = temInitList.begin(); InitListCut != temInitList.end(); InitListCut++)
			   {
				   strReply = strReply + "\n" + InitListCut->second + ":" + to_string(InitListCut->first);
			   }
			   e.group.sendMsg(strReply);
			   return;
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 3) == "ast")/*mark*/
		   {
			   IdleTimer(e.group.id());
			   intMsgCnt += 3;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   string Command;
			   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(
				   static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			   {
				   Command += strLowerMessage[intMsgCnt];
				   intMsgCnt++;
			   }
			   if (Command == "c")
			   {
				   RoomRule[e.group.id()] = 101;
				   e.group.sendMsg("房规设置成功，当前房规为\n当技能值大于50时，大成功=1，大失败=100\n当技能值小于50时，大成功=1，大失败>95");
			   }
			   else if (Command == "show")
			   {
				   string strReply;
				   if (!RoomRule.count(e.group.id()))
				   {
					   strReply = "当前未设置房规，默认房规为：大成功：1-5 大失败：96-100";
				   }
				   else if (RoomRule[e.group.id()] == 1)
				   {
					   strReply = "当前房规为：大成功：1 大失败：100";
				   }
				   else if (RoomRule[e.group.id()] == 0)
				   {
					   strReply = "当前房规为：大成功：已禁用 大失败：已禁用";
				   }
				   else
				   {
					   strReply = "当前房规为：大成功：1-" + to_string(RoomRule[e.group.id()]) + "大失败：" + to_string(101 - RoomRule[e.group.id()]) + "-100";
				   }
				   e.group.sendMsg(strReply);
				   return;
			   }
			   else
			   {
				   string strRoomRule;
				   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   {
					   strRoomRule += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (!strRoomRule.length())
				   {
					   if (RoomRule.count(e.group.id()))
					   {
						   RoomRule.erase(e.group.id());
						   e.group.sendMsg("已将房规删除");
					   }
					   else
						   e.group.sendMsg("房规删除失败");
					   return;
				   }
				   int intRoomRule = stoi(strRoomRule);
				   if (intRoomRule > 50 || intRoomRule < 0)
				   {
					   e.group.sendMsg("房规设置失败，请输入0-50的整数，若使用公式书规则请.astc");
				   }
				   else
				   {
					   string strReply;
					   if (intRoomRule == 1)
					   {
						   strReply = "当前房规为：大成功：1  大失败：100";
						   RoomRule[e.group.id()] = intRoomRule;
					   }
					   else if (!intRoomRule)
					   {
						   strReply = "当前房规为：大成功：已禁用  大失败：已禁用";
						   RoomRule[e.group.id()] = intRoomRule;
					   }
					   else if (intRoomRule == 5)
					   {
						   strReply = "当前房规为：大成功：1-" + to_string(intRoomRule) + "大失败：" + to_string(101 - intRoomRule) + "-100";
						   if (RoomRule.count(e.group.id()))
						   {
							   RoomRule.erase(e.group.id());
						   }
					   }
					   else
					   {
						   RoomRule[e.group.id()] = intRoomRule;
						   strReply = "当前房规为：大成功：1-" + to_string(intRoomRule) + "大失败：" + to_string(101 - intRoomRule) + "-100";
					   }
					   e.group.sendMsg(strReply);
				   }
				   return;
			   }
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "st")
		   {
			   IdleTimer(e.group.id());
			   intMsgCnt += 2;
			   string strDetail;//若涉及属性修改，则使用这一项
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   if (intMsgCnt == strLowerMessage.length())/*.st后面什么都没有*/
			   {
				   e.group.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
				   return;
			   }
			   if (strLowerMessage.substr(intMsgCnt, 3) == "clr")
			   {
				   if (CharacterProp.count(e.sender.id()))
				   {
					   CharacterProp.erase(e.sender.id());
				   }
				   e.group.sendMsg(GlobalMsg[EnumInfoMsg.属性删除完成]);
				   return;
			   }
			   if (strLowerMessage.substr(intMsgCnt, 3) == "del")
			   {
				   intMsgCnt += 3;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   intMsgCnt++;
				   string strSkillName;
				   while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
					   intMsgCnt] == '|'))
				   {
					   strSkillName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
					   if (SkillNameReplace.count(strSkillName))
					   {
						   strSkillName = SkillNameReplace[strSkillName];
					   }
					   if (CharacterProp.count(e.sender.id()) && CharacterProp[e.sender.id()].count(strSkillName))
					   {
						   CharacterProp[e.sender.id()].erase(strSkillName);
						   e.group.sendMsg(GlobalMsg[EnumInfoMsg.技能删除完成]);
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
					   }
					   return;
			   }
			   if (strLowerMessage.substr(intMsgCnt, 4) == "show")
			   {
				   intMsgCnt += 4;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   intMsgCnt++;
				   string strSkillName;
				   while (intMsgCnt != strLowerMessage.length() && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !(strLowerMessage[
					   intMsgCnt] == '|'))
				   {
					   strSkillName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
					   if (SkillNameReplace.count(strSkillName))
						   strSkillName = SkillNameReplace[strSkillName];
					   if (PlayerGroupList.count(e.sender.id()) && PlayerGroupList[e.sender.id()].count(e.group.id())
						   && MultCharProp[e.sender.id()][PlayerGroupList[e.sender.id()][e.group.id()]].count(strSkillName))
					   {
						   e.group.sendMsg(format(GlobalMsg[EnumInfoMsg.属性查找成功], { "[角色卡]" + PlayerGroupList[e.sender.id()][e.group.id()],strSkillName,
							   to_string(MultCharProp[e.sender.id()][PlayerGroupList[e.sender.id()][e.group.id()]][strSkillName]) }));
					   }
					   else if (CharacterProp.count(e.sender.id())
						   && CharacterProp[e.sender.id()].count(strSkillName))
					   {
						   e.group.sendMsg(format(GlobalMsg[EnumInfoMsg.属性查找成功], { strNickName + "的[默认卡]", strSkillName,
							   to_string(CharacterProp[e.sender.id()][strSkillName]) }));
					   }
					   else if (SkillDefaultVal.count(strSkillName))
					   {
						   e.group.sendMsg(format(GlobalMsg[EnumInfoMsg.属性查找成功], { strNickName, strSkillName, to_string(SkillDefaultVal[strSkillName]) }));
					   }
					   else if (!strSkillName.length())/*mark*/
					   {
						   string strReply = "根据客户档案上的记录，" + strNickName + "的[默认卡]上的属性如下：";
						   map<string, int> AllSkill = CharacterProp[e.sender.id()];
						   if (AllSkill.empty())
						   {
							   e.group.sendMsg(strNickName + "的[默认卡]好像没有录入过和默认值不一样的信息诶");
							   return;
						   }
						   map<string, int>::iterator SkillCount = AllSkill.begin();
						   while (!(SkillCount == AllSkill.end()))
						   {
							   strReply = strReply + " " + SkillCount->first + to_string(SkillCount->second);
							   SkillCount++;
						   }
						   e.group.sendMsg(strReply);
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
					   }
					   return;
			   }
			   bool boolError = false;
			   /*多卡存储（下方*/
			   if (strLowerMessage.substr(intMsgCnt, 3) == "add")
			   {
				   intMsgCnt += 3;
				   if (MultCharProp.count(e.sender.id()))
				   {
					   map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
					   if ((Allcharacter.size() >= 10) && (e.sender.id() != MasterQQId))
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.存储卡数量过多]);
						   return;
					   }
				   }

				   string strCharName;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
					   intMsgCnt++;
				   if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
					   return;
				   }
				   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
					   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
					   != ':')
				   {
					   strCharName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
					   intMsgCnt++;


				   while (intMsgCnt != strLowerMessage.length())
				   {
					   string strSkillName;
					   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
						   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
						   != ':'
						   && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '+'
						   && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '-' && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '*'
						   )
					   {
						   strSkillName += strLowerMessage[intMsgCnt];
						   intMsgCnt++;
					   }
					   if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];/*技能名标准化*/
					   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
					   ] == ':')intMsgCnt++;
					   string strSkillVal;
					   /*加入-+*d的判定 向下*/
					   while (intMsgCnt != strLowerMessage.length() && (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
						   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
						   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'))//修改了技能名获取的逻辑
					   {
						   strSkillVal += strLowerMessage[intMsgCnt];
						   intMsgCnt++;
					   }
					   if (strSkillName.empty() || strSkillVal.empty())//移动大小判断到下方
					   {
						   boolError = true;
						   break;
					   }
					   if (strSkillVal.find("+") != string::npos || strSkillVal.find("-") != string::npos
						   || strSkillVal.find("*") != string::npos || strSkillVal.find("d") != string::npos)
					   {
						   if (MultCharProp.count(e.sender.id()) && MultCharProp[e.sender.id()].count(strCharName) && MultCharProp[e.sender.id()][strCharName].count(strSkillName))
						   {
							   strSkillVal = to_string(MultCharProp[e.sender.id()][strCharName][strSkillName]) + strSkillVal;
						   }
						   else if (SkillDefaultVal.count(strSkillName))
						   {
							   strSkillVal = to_string(SkillDefaultVal[strSkillName]) + strSkillVal;
						   }
						   else
						   {
							   strSkillVal = strSkillVal;
						   }
						   DiceCalculatorOutput Result = DiceCalculator(strSkillVal, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						   if (!Result.complate)
						   {
							   boolError = true;
							   break;
						   }
						   if (Result.result < 0)
						   {
							   Result.result = 0;
						   }
						   strDetail = strDetail + strSkillName + ":" + strSkillVal + "=" + to_string(Result.result) + "\n";//记录修改过程
						   strSkillVal = to_string(Result.result);
					   }
					   if (strSkillVal.length() > 3)
					   {
						   boolError = true;
						   break;
					   }
					   /*加入-+*d的判定 向上*/
					   if (!(stoi(strSkillVal) == SkillDefaultVal[strSkillName]))
						   MultCharProp[e.sender.id()][strCharName][strSkillName] = stoi(strSkillVal);
					   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
				   }
				   if (boolError)
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
					   return;
				   }
				   if (strDetail.empty())
					   e.group.sendMsg("已储存[角色卡]：" + strCharName);
				   else
					   e.group.sendMsg("已储存[角色卡]：" + strCharName + "\n以下属性被修改\n" + strDetail.substr(0, strDetail.length() - 1));
				   return;
			   }
			   if (strLowerMessage.substr(intMsgCnt, 3) == "rmv")
			   {
				   if (!MultCharProp.count(e.sender.id()))
				   {
					   e.group.sendMsg("您还没有在这里储存过[角色卡]！");
					   return;
				   }
				   map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
				   if (!Allcharacter.size())
				   {
					   e.group.sendMsg("您还没有在这里储存过[角色卡]！");
					   MultCharProp.erase(e.sender.id());
					   return;
				   }
				   intMsgCnt += 3;
				   string strCharName;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
					   intMsgCnt++;
				   if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
					   return;
				   }
				   if (strLowerMessage.substr(intMsgCnt, 3) == "all")
				   {
					   MultCharProp.erase(e.sender.id());
					   if (PlayerGroupList.count(e.sender.id()))
					   {
						   PlayerGroupList.erase(e.sender.id());
					   }
					   e.group.sendMsg("已销毁所有[角色卡]！");
					   return;
				   }
				   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
					   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
					   != ':')
				   {
					   strCharName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (!Allcharacter.count(strCharName))
				   {
					   e.group.sendMsg("该[角色卡]不存在！");
					   return;
				   }
				   MultCharProp[e.sender.id()].erase(strCharName);
				   if (PlayerGroupList.count(e.sender.id()))
				   {
					   if (!PlayerGroupList[e.sender.id()].empty())
					   {
						   map<unsigned long long, string>::iterator GroupListCount = PlayerGroupList[e.sender.id()].begin();
						   map<unsigned long long, string> TemGroupList;
						   while (GroupListCount != PlayerGroupList[e.sender.id()].end())
						   {
							   if (GroupListCount->second != strCharName)
							   {
								   TemGroupList[GroupListCount->first] = GroupListCount->second;
							   }
							   GroupListCount++;
						   }
						   PlayerGroupList[e.sender.id()] = TemGroupList;
					   }
					   else
						   PlayerGroupList.erase(e.sender.id());
				   }
				   e.group.sendMsg("已销毁[角色卡]：" + strCharName);
				   return;
			   }
			   if (strLowerMessage.substr(intMsgCnt, 4) == "card")
			   {
				   if (!MultCharProp.count(e.sender.id()))
				   {
					   e.group.sendMsg("您还没有在这里储存过[角色卡]！");
					   return;
				   }
				   map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
				   if (!Allcharacter.size())
				   {
					   e.group.sendMsg("您还没有在这里储存过[角色卡]！");
					   MultCharProp.erase(e.sender.id());
					   return;
				   }
				   intMsgCnt += 4;
				   string strCharName;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
					   intMsgCnt++;
				   if (strLowerMessage.substr(intMsgCnt, 3) == "all")
				   {
					   string strReply;
					   map<string, PropType>::iterator CharCount = Allcharacter.begin();
					   while (!(CharCount == Allcharacter.end()))
					   {
						   strReply = strReply + " " + CharCount->first;
						   CharCount++;
					   }
					   e.group.sendMsg(strNickName + "在这里存储了的[角色卡]有：" + strReply);
					   return;
				   }
				   if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
					   return;
				   }
				   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
					   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
					   != ':')
				   {
					   strCharName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (!Allcharacter.count(strCharName))
				   {
					   e.group.sendMsg("该[角色卡]不存在！");
					   return;
				   }
				   string strReply = "根据客户档案上的记录，[角色卡]" + strCharName + "的属性如下：";
				   map<string, int> CharacterProp = Allcharacter[strCharName];
				   map<string, int>::iterator SkillCount = CharacterProp.begin();
				   while (!(SkillCount == CharacterProp.end()))
				   {
					   strReply = strReply + " " + SkillCount->first + to_string(SkillCount->second);
					   SkillCount++;
				   }
				   e.group.sendMsg(strReply);
				   return;
			   }
			   if (strLowerMessage.substr(intMsgCnt, 3) == "set")
			   {
				   if (!MultCharProp.count(e.sender.id()))
				   {
					   e.group.sendMsg("您还没有在这里储存过[角色卡]！");
					   return;
				   }
				   map<string, PropType> Allcharacter = MultCharProp[e.sender.id()];
				   if (!Allcharacter.size())
				   {
					   e.group.sendMsg("您还没有在这里储存过[角色卡]！");
					   MultCharProp.erase(e.sender.id());
					   return;
				   }
				   intMsgCnt += 3;
				   if (strLowerMessage.substr(intMsgCnt, 2) == "gr")
				   {
					   intMsgCnt += 2;
					   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
						   intMsgCnt++;
					   if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
					   {
						   if (PlayerGroupList.count(e.sender.id()))
						   {
							   if (PlayerGroupList[e.sender.id()].count(e.group.id()))
								   e.group.sendMsg("您在本群中使用的[角色卡]是：" + PlayerGroupList[e.sender.id()][e.group.id()]);
							   else
								   e.group.sendMsg("您还未在本群中设置过[角色卡]！");
						   }
						   else
							   e.group.sendMsg("您还未在任何群设置过[角色卡]！");
						   return;
					   }
					   if (strLowerMessage.substr(intMsgCnt, 3) == "all")
					   {
						   if (PlayerGroupList.count(e.sender.id()))
						   {
							   map<unsigned long long, string> GroupList = PlayerGroupList[e.sender.id()];
							   if (GroupList.empty())
							   {
								   e.group.sendMsg("您还未在任何群设置过[角色卡]！");
								   PlayerGroupList.erase(e.sender.id());
								   return;
							   }
							   map<unsigned long long, string>::iterator GroupListCount = GroupList.begin();
							   string strReply = "您在这些群里设置过这些[角色卡]:";
							   while (!(GroupListCount == GroupList.end()))
							   {
								   strReply = strReply + "\n" + GroupListCount->second + to_string(GroupListCount->first);
								   GroupListCount++;
							   }
							   e.group.sendMsg(strReply);
							   return;
						   }
						   else
						   {
							   e.group.sendMsg("您还未在任何群设置过[角色卡]！");
							   return;
						   }
						   return;
					   }
					   string strCharName;
					   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
						   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
						   != ':')
					   {
						   strCharName += strLowerMessage[intMsgCnt];
						   intMsgCnt++;
					   }
					   if (!Allcharacter.count(strCharName))
					   {
						   e.group.sendMsg("该[角色卡]不存在！");
						   return;
					   }
					   PlayerGroupList[e.sender.id()][e.group.id()] = strCharName;
					   e.group.sendMsg(strNickName + "已在本群中启用[角色卡]" + strCharName);
					   return;
				   }
				   if (strLowerMessage.substr(intMsgCnt, 2) == "de")
				   {
					   intMsgCnt += 2;
					   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
						   intMsgCnt++;
					   if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
					   {
						   if (PlayerGroupList.count(e.sender.id()))
						   {
							   if (PlayerGroupList[e.sender.id()].count(e.group.id()))
							   {
								   PlayerGroupList[e.sender.id()].erase(e.group.id());
								   e.group.sendMsg(strNickName + "已关闭在本群中使用的[角色卡]");
								   if (PlayerGroupList[e.sender.id()].empty())
									   PlayerGroupList.erase(e.sender.id());
							   }
							   else
							   {
								   e.group.sendMsg("您还未在本群中设置过[角色卡]！");
							   }
						   }
						   else
						   {
							   e.group.sendMsg("您还未在任何群设置过[角色卡]！");
						   }
						   return;
					   }
					   if (strLowerMessage.substr(intMsgCnt, 3) == "all")
					   {
						   if (PlayerGroupList.count(e.sender.id()))
						   {
							   PlayerGroupList.erase(e.sender.id());
							   e.group.sendMsg("已清除您在所有群中设置的[角色卡]");
							   return;
						   }
						   e.group.sendMsg("您还未在任何群设置过[角色卡]！");
						   return;
					   }
					   return;
				   }
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')
					   intMsgCnt++;
				   if (intMsgCnt == strLowerMessage.length())/*后面什么都没有*/
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
					   return;
				   }
				   string strCharName;
				   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
					   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
					   != ':')
				   {
					   strCharName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (!Allcharacter.count(strCharName))
				   {
					   e.group.sendMsg("该[角色卡]不存在！");
					   return;
				   }
				   if (CharacterProp.count(e.sender.id()))
				   {
					   CharacterProp.erase(e.sender.id());
				   }
				   CharacterProp[e.sender.id()] = Allcharacter[strCharName];
				   e.group.sendMsg(GlobalMsg[EnumInfoMsg.属性存储完成]);
				   return;
			   }
			   /*多卡存储（上方*/
			   while (intMsgCnt != strLowerMessage.length())
			   {
				   string strSkillName;
				   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
					   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt]
					   != ':'
					   && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '+'
					   && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '-' && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '*'
					   )
				   {
					   strSkillName += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (SkillNameReplace.count(strSkillName))
					   strSkillName = SkillNameReplace[strSkillName];
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt
				   ] == ':')intMsgCnt++;
				   string strSkillVal;
				   /*加入-+*d的判定 向下*/
				   while (intMsgCnt != strLowerMessage.length() && (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
					   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
					   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'))
				   {
					   strSkillVal += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (strSkillName.empty() || strSkillVal.empty())//移动大小判断到下方
				   {
					   boolError = true;
					   break;
				   }
				   if (strSkillVal.find("+") != string::npos|| strSkillVal.find("-") != string::npos
					   || strSkillVal.find("*") != string::npos|| strSkillVal.find("d") != string::npos)
				   {
					   if (CharacterProp[e.sender.id()].count(strSkillName))
					   {
						   strSkillVal = to_string(CharacterProp[e.sender.id()][strSkillName]) + strSkillVal;
					   }
					   else if (SkillDefaultVal.count(strSkillName))
					   {
						   strSkillVal = to_string(SkillDefaultVal[strSkillName]) + strSkillVal;
					   }
					   else
					   {
						   strSkillVal = strSkillVal;
					   }
					   DiceCalculatorOutput Result = DiceCalculator(strSkillVal, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
					   if (!Result.complate)
					   {
						   boolError = true;
						   break;
					   }
					   if (Result.result < 0)
					   {
						   Result.result = 0;
					   }
					   strDetail = strDetail + strSkillName + ":" + strSkillVal + "=" + to_string(Result.result) + "\n";//记录修改过程
					   strSkillVal = to_string(Result.result);
				   }
				   if (strSkillVal.length() > 3)
				   {
					   boolError = true;
					   break;
				   }
				   /*加入-+*d的判定 向上*/
				   if (stoi(strSkillVal) != SkillDefaultVal[strSkillName])
					   CharacterProp[e.sender.id()][strSkillName] = stoi(strSkillVal);
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '|')intMsgCnt++;
			   }
			   if (boolError)
			   {
				   e.group.sendMsg(GlobalMsg[EnumErrorMsg.ST指令格式错误]);
			   }
			   else
			   {
				   if (strDetail.empty())
					   e.group.sendMsg(GlobalMsg[EnumInfoMsg.属性存储完成]);
				   else
					   e.group.sendMsg(GlobalMsg[EnumInfoMsg.属性存储完成] + "\n以下属性被修改\n" + strDetail.substr(0, strDetail.length() - 1));
			   }
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "tz")/*随机特性的部分*/
			{
				IdleTimer(e.group.id());
				const int intcharacteristic = RandomGenerator::Randint(1, 120);
				string CharacterReply = "经查证，在某个梦境中的" + strNickName + "的同位体拥有的特性为：\n" + Characteristic[intcharacteristic];
				e.group.sendMsg(CharacterReply);
			}
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "ct")/*mark.ct的部分*/
			{
				intMsgCnt += 2;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				int intcocktail;
				if (intMsgCnt == strLowerMessage.length())/*.ct后面什么都没有*/
				{
					intcocktail = RandomGenerator::Randint(1, (*CocktailList).size());
					string RandomCockReply = "品味生活！已为" + strNickName + "调好最合适的饮品：\n" + CocktailList->at(intcocktail).CocktailENName + "（" + CocktailList->at(intcocktail).CockTailCNName + "）" + CocktailList->at(intcocktail).CocktailData + "\n描述仅供参考";
					e.group.sendMsg(RandomCockReply);
					return;
				}
				string strCocktailName;
				while (intMsgCnt != strLowerMessage.length())
				{
					strCocktailName += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				while (strCocktailName.find(" ") != string::npos)
					strCocktailName.erase(strCocktailName.find(" "), 1);
				string TemCocktailName;
				if (CNCockList.count(strCocktailName))
				{
					intcocktail = CNCockList[strCocktailName];
					string RandomCockReply = "品味生活！已为" + strNickName + "调好最合适的饮品：\n" + CocktailList->at(intcocktail).CocktailENName + "（" + CocktailList->at(intcocktail).CockTailCNName + "）" + CocktailList->at(intcocktail).CocktailData + "\n描述仅供参考";
					e.group.sendMsg(RandomCockReply);
					return;
				}
				if (ENCockList.count(strCocktailName))
				{
					intcocktail = ENCockList[strCocktailName];
					string RandomCockReply = "品味生活！已为" + strNickName + "调好最合适的饮品：\n" + CocktailList->at(intcocktail).CocktailENName + "（" + CocktailList->at(intcocktail).CockTailCNName + "）" + CocktailList->at(intcocktail).CocktailData + "\n描述仅供参考";
					e.group.sendMsg(RandomCockReply);
					return;
				}
				map<const int, CocktailType>::iterator CocktailCount = CocktailList->begin();
				while (CocktailCount != CocktailList->end())
				{
					TemCocktailName = CDs_tolower(CocktailCount->second.CocktailENName) + CocktailCount->second.CockTailCNName;
					if (TemCocktailName.find(strCocktailName) != string::npos)
					{
						string RandomCockReply = "品味生活！为" + strNickName + "调好已点单的饮品：\n" + CocktailCount->second.CocktailENName + "（" + CocktailCount->second.CockTailCNName + "）" + CocktailCount->second.CocktailData + "\n描述仅供参考";
						e.group.sendMsg(RandomCockReply);
						return;
					}
					CocktailCount++;
				}
				e.group.sendMsg("该酒品不存在！");
			}
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "so")/*浅草寺100签*/
		   {
			   const int intsortilege = RandomGenerator::Randint(1, 100);
			   string Sortilege = "（晃动签桶）......那么神明给" + strNickName + "的指引是.....\n" + "第" + std::to_string(intsortilege) + "签  " + SensojiTempleDivineSign[intsortilege];
			   e.group.sendMsg(Sortilege);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "ra")
		   {
			   IdleTimer(e.group.id());
			   intMsgCnt += 2;
			   bool setporb = 0, isPunish = 1;/*下面是修改的部分mark(包括本行*/
			   string strpbNum;
			   int intpbNum = 1;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   if (strLowerMessage[intMsgCnt] == 'p' || strLowerMessage[intMsgCnt] == 'b')
			   {
				   setporb = 1;
				   if (strLowerMessage[intMsgCnt] == 'b')
					   isPunish = 0;
				   intMsgCnt++;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   intMsgCnt++;
				   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   {
					   strpbNum += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (strpbNum.length())
					   intpbNum = stoi(strpbNum);
				   if (intpbNum > 10)
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.掷骰次数过多]);
					   return;
				   }
				   if (!intpbNum)
					   setporb = 0;
			   }/*上面是修改的部分mark(包括本行*/
			   string strSkillName;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   while (intMsgCnt != strLowerMessage.length() && !isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !
				   isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && strLowerMessage[intMsgCnt] != '=' && strLowerMessage[intMsgCnt] !=
				   ':' && strLowerMessage[intMsgCnt] != '+' && strLowerMessage[intMsgCnt] != '-')/*添加了+-判定*/
			   {
				   strSkillName += strLowerMessage[intMsgCnt];
				   intMsgCnt++;
			   }
			   if (SkillNameReplace.count(strSkillName))strSkillName = SkillNameReplace[strSkillName];/*取得标准技能名称*/
			   signed int intcorrection = 0;/*补正数据读取*/
			   if (strLowerMessage[intMsgCnt] == '+' || strLowerMessage[intMsgCnt] == '-')
			   {
				   if (strLowerMessage[intMsgCnt] == '+')
				   {
					   intcorrection = 1;
				   }
				   else
				   {
					   intcorrection = -1;
				   }
				   intMsgCnt++;
				   string strCorrection;
				   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   {
					   strCorrection += strLowerMessage[intMsgCnt];
					   intMsgCnt++;
				   }
				   if (strCorrection.empty())
					   intcorrection = 0;
				   else if (strCorrection.length() > 2)
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
					   return;
				   }
				   else
				   {
					   intcorrection *= stoi(strCorrection);
				   }
			   }/*补正数据读取结束*/
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || strLowerMessage[intMsgCnt] == '=' || strLowerMessage[intMsgCnt] == ':')
				   intMsgCnt++;
			   string strSkillVal;
			   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			   {
				   strSkillVal += strLowerMessage[intMsgCnt];
				   intMsgCnt++;
			   }
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
			   {
				   intMsgCnt++;
			   }
			   string strReason = msg.substr(intMsgCnt);
			   int intSkillVal;
			   if (strSkillVal.empty())
			   {
				   if (PlayerGroupList.count(e.sender.id()) 
					   && (PlayerGroupList[e.sender.id()].count(e.group.id())))/*是否在群组里绑定过卡*/
				   {
					   string strCharName = PlayerGroupList[e.sender.id()][e.group.id()];
					   if (MultCharProp[e.sender.id()][strCharName].count(strSkillName))
					   {
						   intSkillVal = MultCharProp[e.sender.id()][strCharName][strSkillName];
					   }
					   else if (SkillDefaultVal.count(strSkillName))
					   {
						   intSkillVal = SkillDefaultVal[strSkillName];
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败] + "\n当前绑定卡：" + strCharName);
						   return;
					   }
				   }
				   else
				   {
					   if (CharacterProp.count(e.sender.id()) && CharacterProp[e.sender.id()].count(strSkillName))
					   {
						   intSkillVal = CharacterProp[e.sender.id()][strSkillName];
					   }
					   else if (SkillDefaultVal.count(strSkillName))
					   {
						   intSkillVal = SkillDefaultVal[strSkillName];
					   }
					   else
					   {
						   e.group.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
						   return;
					   }
				   }
			   }
			   else if (strSkillVal.length() > 3)
			   {
				   e.group.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
				   return;
			   }
			   else
			   {
				   intSkillVal = stoi(strSkillVal);
			   }
			   if (intcorrection)/*数据补正*/
			   {
				   intSkillVal += intcorrection;
			   }
			   int intD100Res = RandomGenerator::Randint(1, 100);
			   string strReply = strNickName + "进行" + strSkillName + "检定: D100=" + to_string(intD100Res);/*下面是修改的部分mark(包括本行*/
			   if (setporb)
			   {
				   int pbRandom, single_figures;
				   string pbShow = "";
				   if (intD100Res == 100)
					   single_figures = 0;
				   else
					   single_figures = intD100Res % 10;
				   if (isPunish)
				   {
					   pbShow = "（惩罚骰：";
					   for (int pbCunt = 0; pbCunt < intpbNum; pbCunt++)
					   {
						   pbRandom = RandomGenerator::Randint(0, 9);
						   pbShow = pbShow + " " + to_string(pbRandom);
						   if ((pbRandom == 0) && (single_figures == 0))
							   pbRandom = 10;
						   pbRandom = pbRandom * 10;
						   if (pbRandom > intD100Res)
							   intD100Res = pbRandom + single_figures;
					   }
				   }
				   else
				   {
					   pbShow = "（奖励骰：";
					   for (int pbCunt = 0; pbCunt < intpbNum; pbCunt++)
					   {
						   pbRandom = RandomGenerator::Randint(0, 9);
						   pbShow = pbShow + " " + to_string(pbRandom);
						   if ((pbRandom == 0) && (single_figures == 0))
							   pbRandom = 10;
						   pbRandom = pbRandom * 10;
						   if (pbRandom < intD100Res)
							   intD100Res = pbRandom + single_figures;
					   }
				   }
				   pbShow = pbShow + "），最终结果是：" + to_string(intD100Res);
				   strReply += pbShow + "/" + to_string(intSkillVal) + " ";
			   }
			   else
				   strReply += "/" + to_string(intSkillVal) + " ";
			   int RoomRuleNum = 5;/*自定义房规部分（包括此行向下*/
			   if (RoomRule.count(e.group.id()))
				   RoomRuleNum = RoomRule[e.group.id()];
			   /*上面是修改的部分mark(包括本行*/

			   if (RoomRuleNum == 101)//公式书规则检定
			   {
				   if (intD100Res == 1 && intD100Res <= intSkillVal)
				   {
					   strReply += GlobalMsg[EnumInfoMsg.大成功];
				   }
				   else if (intD100Res <= (intSkillVal / 5))
				   {
					   strReply += GlobalMsg[EnumInfoMsg.极难成功];
				   }
				   else if (intD100Res <= (intSkillVal / 2))
				   {
					   strReply += GlobalMsg[EnumInfoMsg.困难成功];
				   }
				   else if (intD100Res <= intSkillVal)
				   {
					   strReply += GlobalMsg[EnumInfoMsg.成功];
				   }
				   else
				   {
					   if (50 <= intSkillVal)/*技能大于50*/
					   {
						   if (intD100Res <= 99)
						   {
							   strReply += GlobalMsg[EnumInfoMsg.失败];
						   }
						   else
						   {
							   strReply += GlobalMsg[EnumInfoMsg.大失败];
						   }
					   }
					   else
					   {
						   if (intD100Res <= 95)
						   {
							   strReply += GlobalMsg[EnumInfoMsg.失败];
						   }
						   else
						   {
							   strReply += GlobalMsg[EnumInfoMsg.大失败];
						   }
					   }
				   }
			   }
			   else
			   {
				   if (intD100Res <= intSkillVal)/*成功*/
				   {
					   if (intD100Res <= RoomRuleNum)
					   {
						   strReply += GlobalMsg[EnumInfoMsg.大成功];
					   }
					   else if (intD100Res <= intSkillVal / 5)
					   {
						   strReply += GlobalMsg[EnumInfoMsg.极难成功];
					   }
					   else if (intD100Res <= intSkillVal / 2)
					   {
						   strReply += GlobalMsg[EnumInfoMsg.困难成功];
					   }
					   else
					   {
						   strReply += GlobalMsg[EnumInfoMsg.成功];
					   }
				   }
				   else/*失败*/
				   {
					   if (intD100Res >= (101 - RoomRuleNum))
					   {
						   strReply += GlobalMsg[EnumInfoMsg.大失败];
					   }
					   else
					   {
						   strReply += GlobalMsg[EnumInfoMsg.失败];
					   }
				   }
			   }

			   if (!strReason.empty())
			   {
				   strReply = "由于" + strReason + " " + strReply;
			   }
			   e.group.sendMsg(strReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 3) == "set")
		   {
			   IdleTimer(e.group.id());
			   intMsgCnt += 3;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   string strDefaultDice = strLowerMessage.substr(intMsgCnt, strLowerMessage.find(" ", intMsgCnt) - intMsgCnt);
			   while (strDefaultDice[0] == '0')
				   strDefaultDice.erase(strDefaultDice.begin());
			   if (strDefaultDice.empty())
				   strDefaultDice = "100";
			   for (auto charNumElement : strDefaultDice)
				   if (!isdigit(static_cast<unsigned char>(charNumElement)))
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.默认骰面数为0]);
					   return;
				   }
			   if (strDefaultDice.length() > 5)
			   {
				   e.group.sendMsg(GlobalMsg[EnumErrorMsg.默认骰面数过多]);
				   return;
			   }
			   const int intDefaultDice = stoi(strDefaultDice);
			   if (intDefaultDice == 100)
				   DefaultDice.erase(e.sender.id());
			   else
				   DefaultDice[e.sender.id()] = intDefaultDice;
			   const string strSetSuccessReply = "已将" + strNickName + "的默认骰类型更改为D" + strDefaultDice;
			   e.group.sendMsg(strSetSuccessReply);
		   }
		   else if (strLowerMessage.substr(intMsgCnt, 4) == "name")
			{
				intMsgCnt += 4;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;

				string type;
				while (isalpha(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					type += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}

				auto nameType = NameGenerator::Type::UNKNOWN;
				if (type == "cn")
					nameType = NameGenerator::Type::CN;
				else if (type == "en")
					nameType = NameGenerator::Type::EN;
				else if (type == "jp")
					nameType = NameGenerator::Type::JP;

				while (isspace(static_cast<unsigned char>(msg[intMsgCnt])))
					intMsgCnt++;

				string strNum;
				while (isdigit(static_cast<unsigned char>(msg[intMsgCnt])))
				{
					strNum += msg[intMsgCnt];
					intMsgCnt++;
				}
				if (strNum.size() > 2)
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.命名人数过多]);
					return;
				}
				int intNum = stoi(strNum.empty() ? "1" : strNum);
				if (intNum > 10)
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.命名人数过多]);
					return;
				}
				if (intNum == 0)
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.人物作成数量为0]);
					return;
				}
				vector<string> TempNameStorage;
				while (TempNameStorage.size() != intNum)
				{
					string name = NameGenerator::getRandomName(nameType);
					if (find(TempNameStorage.begin(), TempNameStorage.end(), name) == TempNameStorage.end())
					{
						TempNameStorage.push_back(name);
					}
				}
				string strReply = "既然你让我起名了，那么 " + strNickName + " 的随机名称就这些吧:\n";
				for (auto i = 0; i != TempNameStorage.size(); i++)
				{
					strReply.append(TempNameStorage[i]);
					if (i != TempNameStorage.size() - 1)strReply.append(", ");
				}
				e.group.sendMsg(strReply);
			}
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "sc")
			{
				IdleTimer(e.group.id());
				intMsgCnt += 2;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				string strSuccessSc, strFailSc, strSan;
				int intSan;
				while (intMsgCnt != strLowerMessage.length() && 
					(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'))
				{
					strSuccessSc += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strSuccessSc.length() == 0)
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
					return;
				}
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				if (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '/')
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
					return;
				}
				else
				{
					intMsgCnt++;
				}
				while (intMsgCnt != strLowerMessage.length()&&
					(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'))
				{
					strFailSc += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strFailSc.length() == 0)
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
					return;
				}
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strSan += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				//用于判断从哪里取得的san值，0为直接取得，1为在角色卡中取得，2为在默认卡中取得
				int intWhereComesSan = 0;
				if (strSan.length() != 0)
				{
					if (strSan.length() > 3)
					{
						e.group.sendMsg(GlobalMsg[EnumErrorMsg.SCsan值错误]);
						return;
					}
					intSan = stoi(strSan);
				}
				else if (PlayerGroupList.count(e.sender.id())
					&& PlayerGroupList[e.sender.id()].count(e.group.id()))//检查一下是否在组里绑定过卡
				{
					string strCharName = PlayerGroupList[e.sender.id()][e.group.id()];
					if (MultCharProp[e.sender.id()][strCharName].count("理智"))
					{
						intSan = MultCharProp[e.sender.id()][strCharName]["理智"];
						intWhereComesSan = 1;
					}
					else
					{
						e.group.sendMsg(GlobalMsg[EnumErrorMsg.SCsan值错误] + "\n当前绑定卡：" + strCharName);
						return;
					}
				}
				else if (CharacterProp.count(e.sender.id())
					&& CharacterProp[e.sender.id()].count("理智"))
				{
					intSan = CharacterProp[e.sender.id()]["理智"];
					intWhereComesSan = 2;
				}
				else
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.SCsan值错误]);
					return;
				}
				string strReply = strNickName + "的Sancheck:\n1D100=";
				int intD100Res = RandomGenerator::Randint(1, 100);
				strReply = strReply + to_string(intD100Res) + "/" + to_string(intSan);

				int RoomRuleNum = 5;/*自定义房规部分（包括此行向下*/
				if (RoomRule.count(e.group.id()))
					RoomRuleNum = RoomRule[e.group.id()];
				/*上面是修改的部分mark(包括本行*/

				if (RoomRuleNum == 101)//公式书规则检定
				{
					if (intD100Res <= intSan)
					{
						DiceCalculatorOutput OutputReply = DiceCalculator(strSuccessSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (OutputReply.complate == false)
						{
							e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
							return;
						}
						intSan -= OutputReply.result;
						if (intSan < 0)
						{
							intSan = 0;
						}
						strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc成功] +
							"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
					}
					else
					{
						if (50 <= intSan)/*技能大于50*/
						{
							if (intD100Res <= 99)
							{
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
							else
							{
								while (strFailSc.find("d") != string::npos)
								{
									int intdPos = strFailSc.find("d");
									if (intdPos != 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, "*");
										}
										else if (!isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && !isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos == 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos != 0 && intdPos == strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else
									{
										strFailSc = DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100";
									}
								}
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc大失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
						}
						else
						{
							if (intD100Res <= 95)
							{
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
							else
							{
								while (strFailSc.find("d") != string::npos)
								{
									int intdPos = strFailSc.find("d");
									if (intdPos != 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, "*");
										}
										else if (!isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && !isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos == 0 && intdPos != strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
										{
											strFailSc.erase(intdPos, 1);
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else if (intdPos != 0 && intdPos == strFailSc.length() - 1)
									{
										if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])))
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
										}
										else
										{
											strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
										}
									}
									else
									{
										strFailSc = DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100";
									}
								}
								DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
								if (OutputReply.complate == false)
								{
									e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
									return;
								}
								intSan -= OutputReply.result;
								if (intSan < 0)
								{
									intSan = 0;
								}
								strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc大失败] +
									"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
							}
						}
					}
				}
				else
				{
					if (intD100Res <= intSan)/*成功*/
					{
						DiceCalculatorOutput OutputReply = DiceCalculator(strSuccessSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (OutputReply.complate == false)
						{
							e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
							return;
						}
						intSan -= OutputReply.result;
						if (intSan < 0)
						{
							intSan = 0;
						}
						strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc成功] +
							"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
					}
					else/*失败*/
					{
						if (intD100Res >= (101 - RoomRuleNum))
						{
							while (strFailSc.find("d") != string::npos)
							{
								int intdPos = strFailSc.find("d");
								if (intdPos != 0 && intdPos != strFailSc.length() - 1)
								{
									if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.replace(intdPos, 1, "*");
									}
									else if (!isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.erase(intdPos, 1);
									}
									else if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])) && !isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
									}
									else
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
									}
								}
								else if (intdPos == 0 && intdPos != strFailSc.length() - 1)
								{
									if (isdigit(static_cast<unsigned char>(strFailSc[intdPos + 1])))
									{
										strFailSc.erase(intdPos, 1);
									}
									else
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
									}
								}
								else if (intdPos != 0 && intdPos == strFailSc.length() - 1)
								{
									if (isdigit(static_cast<unsigned char>(strFailSc[intdPos - 1])))
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? "*" + to_string(DefaultDice[e.sender.id()]) : "*100");
									}
									else
									{
										strFailSc.replace(intdPos, 1, DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100");
									}
								}
								else
								{
									strFailSc = DefaultDice.count(e.sender.id()) ? to_string(DefaultDice[e.sender.id()]) : "100";
								}
							}
							DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
							if (OutputReply.complate == false)
							{
								e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
								return;
							}
							intSan -= OutputReply.result;
							if (intSan < 0)
							{
								intSan = 0;
							}
							strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc大失败] +
								"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
						}
						else
						{
							DiceCalculatorOutput OutputReply = DiceCalculator(strFailSc, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
							if (OutputReply.complate == false)
							{
								e.group.sendMsg(GlobalMsg[EnumErrorMsg.SC格式错误]);
								return;
							}
							intSan -= OutputReply.result;
							if (intSan < 0)
							{
								intSan = 0;
							}
							strReply = strReply + " " + GlobalMsg[EnumInfoMsg.sc失败] +
								"\n你的san值减少" + to_string(OutputReply.result) + "点，当前剩余" + to_string(intSan) + "点";
						}
					}
				}
				if (intWhereComesSan == 1)
				{
					MultCharProp[e.sender.id()][PlayerGroupList[e.sender.id()][e.group.id()]]["理智"] = intSan;
				}
				else if (intWhereComesSan == 2)
				{
					CharacterProp[e.sender.id()]["理智"] = intSan;
				}
				else
				{
					//啥都不干
				}
				e.group.sendMsg(strReply);
			}
		   else if (strLowerMessage.substr(intMsgCnt, 2) == "en")
			{
				IdleTimer(e.group.id());
				intMsgCnt += 2;
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				string strSkillName;
				while (intMsgCnt != strLowerMessage.length() &&
					!isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) && !isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt]))
					&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '+'
					&& static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '-' && static_cast<unsigned char>(strLowerMessage[intMsgCnt]) != '*')
				{
					strSkillName += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				if (strSkillName.length() == 0)
				{
					e.group.sendMsg(GlobalMsg[EnumErrorMsg.EN指令格式错误]);
					return;
				}
				if (SkillNameReplace.count(strSkillName))
					strSkillName = SkillNameReplace[strSkillName];
				while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					intMsgCnt++;
				string strSkillVal;
				while (intMsgCnt != strLowerMessage.length() &&
					isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				{
					strSkillVal += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				//用于判断从哪里取得的技能值，0为直接取得，1为在角色卡中取得，2为在默认卡中取得
				int intWhereComesSkill = 0;
				int intSkillVal;
				if (strSkillVal.empty())
				{
					if (PlayerGroupList.count(e.sender.id())
						&& (PlayerGroupList[e.sender.id()].count(e.group.id())))/*是否在群组里绑定过卡*/
					{
						intWhereComesSkill = 1;
						string strCharName = PlayerGroupList[e.sender.id()][e.group.id()];
						if (MultCharProp[e.sender.id()][strCharName].count(strSkillName))
						{
							intSkillVal = MultCharProp[e.sender.id()][strCharName][strSkillName];
						}
						else if (SkillDefaultVal.count(strSkillName))
						{
							intSkillVal = SkillDefaultVal[strSkillName];
						}
						else
						{
							e.group.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败] + "\n当前绑定卡：" + strCharName);
							return;
						}
					}
					else
					{
						intWhereComesSkill = 2;
						if (CharacterProp.count(e.sender.id()) && CharacterProp[e.sender.id()].count(strSkillName))
						{
							intSkillVal = CharacterProp[e.sender.id()][strSkillName];
						}
						else if (SkillDefaultVal.count(strSkillName))
						{
							intSkillVal = SkillDefaultVal[strSkillName];
						}
						else
						{
							e.group.sendMsg(GlobalMsg[EnumErrorMsg.属性查找失败]);
							return;
						}
					}
				}
				else
				{
					if (strSkillVal.length() > 3)
					{
						e.group.sendMsg(GlobalMsg[EnumErrorMsg.技能值错误]);
						return;
					}
					intSkillVal = stoi(strSkillVal);
				}
				string strEnEquation;
				while (intMsgCnt != strLowerMessage.length()&&
					(isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt])=='+'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
					|| static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd'))
				{
					strEnEquation += strLowerMessage[intMsgCnt];
					intMsgCnt++;
				}
				int intdD100Res = RandomGenerator::Randint(1, 100);
				string strAns = strNickName + "的" + strSkillName + "增强或成长检定:1D100=" + to_string(intdD100Res) + "/" + to_string(intSkillVal) + "\n";
				if (intdD100Res > intSkillVal || intdD100Res >= 95)
				{
					int intENresult = 0;
					if (strEnEquation.empty())
					{
						intENresult = RandomGenerator::Randint(1, 10);
						intSkillVal += intENresult;
					}
					else
					{
						DiceCalculatorOutput ENRecult = DiceCalculator(strEnEquation, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
						if (!ENRecult.complate)
						{
							e.group.sendMsg(GlobalMsg[EnumErrorMsg.EN指令格式错误]);
							return;
						}
						intENresult = ENRecult.result;
						intSkillVal += intENresult;
					}
					strAns = strAns + GlobalMsg[EnumInfoMsg.en成功] + ",你的" + strSkillName + "成长了" + to_string(intENresult) + "点，当前值为" + to_string(intSkillVal) + "点";
					if (intWhereComesSkill == 1)
					{
						string strCharName = PlayerGroupList[e.sender.id()][e.group.id()];
						if (SkillDefaultVal.count(strSkillName) && SkillDefaultVal[strSkillName] == intSkillVal)
						{
							MultCharProp[e.sender.id()][strCharName].erase(strSkillName);
						}
						else
						{
							MultCharProp[e.sender.id()][strCharName][strSkillName] = intSkillVal;
						}
					}
					else if (intWhereComesSkill == 2)
					{
						if (SkillDefaultVal.count(strSkillName) && SkillDefaultVal[strSkillName] == intSkillVal)
						{
							CharacterProp[e.sender.id()].erase(strSkillName);
						}
						else
						{
							CharacterProp[e.sender.id()][strSkillName] = intSkillVal;
						}
					}
					e.group.sendMsg(strAns);
				}
				else
				{
					strAns = strAns + GlobalMsg[EnumInfoMsg.en失败];
					e.group.sendMsg(strAns);
				}
			}
		   else if (strLowerMessage.substr(intMsgCnt, 6) == "master")
		   {
			   if (e.sender.id() != MasterQQId)
			   {
				   e.group.sendMsg("前面是工作区域，请顾客止步哦");
				   return;
			   }
			   intMsgCnt += 6;
			   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				   intMsgCnt++;
			   if (strLowerMessage.substr(intMsgCnt, 3) == "cir")
			   {
				   intMsgCnt += 3;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   intMsgCnt++;
				   if (strLowerMessage.substr(intMsgCnt, 2) == "on")
				   {
					   if (boolRunCirculate)
					   {
						   e.group.sendMsg("时钟已经在运行了");
						   return;
					   }
					   else
					   {
						   boolRunCirculate = true;
						   e.group.sendMsg("时钟已启动");
						   thread thrMainCirculate(MainCirculate, MasterQQId, e.bot.id);
						   thrMainCirculate.join();
						   return;
					   }
				   }
				   else if (strLowerMessage.substr(intMsgCnt, 3) == "off")
				   {
					   if (!boolRunCirculate)
					   {
						   e.group.sendMsg("时钟已经关闭了");
						   return;
					   }
					   else
					   {
						   boolRunCirculate = false;
						   e.group.sendMsg("时钟已关闭");
						   return;
					   }
				   }
				   else
				   {
					   return;
				   }
			   }
			   else if (strLowerMessage.substr(intMsgCnt, 4) == "list")
			   {
				   logger->info("——————————群组列表——————————");
				   vector<unsigned long long>ReplyGroupList = e.bot.getGroupList();
				   for (vector<unsigned long long>::iterator i = ReplyGroupList.begin(); i != ReplyGroupList.end(); i++)
				   {
					   logger->info(to_string(*i) + "\t" + Group(*i, e.bot.id).nickOrNameCard());
				   }
				   logger->info("——————————好友列表——————————");
				   vector<unsigned long long>ReplyFriendList = e.bot.getFriendList();
				   for (vector<unsigned long long>::iterator i = ReplyFriendList.begin(); i != ReplyFriendList.end(); i++)
				   {
					   logger->info(to_string(*i) + "\t" + Friend(*i, e.bot.id).nickOrNameCard());
				   }
				   logger->info("——————————列表结束——————————");
			   }
			   else if (strLowerMessage.substr(intMsgCnt, 3) == "lmt")
			   {
				   intMsgCnt += 3;
				   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   intMsgCnt++;
				   if (strLowerMessage.substr(intMsgCnt, 2) == "on")
				   {
					   if (boolRunLmt)
					   {
						   e.group.sendMsg("LMT已经在运行了");
						   return;
					   }
					   else
					   {
						   boolRunLmt = true;
						   e.group.sendMsg("LMT已启动");
						   return;
					   }
				   }
				   else if (strLowerMessage.substr(intMsgCnt, 3) == "off")
				   {
					   if (!boolRunLmt)
					   {
						   e.group.sendMsg("LMT已经关闭了");
						   return;
					   }
					   else
					   {
						   boolRunLmt = true;
						   e.group.sendMsg("LMT已停用");
						   return;
					   }
				   }
				   else if (strLowerMessage.substr(intMsgCnt, 3) == "add")
				   {
					   intMsgCnt += 3;
					   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						   intMsgCnt++;
					   string strGroupNum;
					   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   {
						   strGroupNum += strLowerMessage[intMsgCnt];
						   intMsgCnt++;
					   }
					   if (strGroupNum.empty())
					   {
						   e.group.sendMsg("试图加入LMT白名单的群号为空");
						   return;
					   }
					   unsigned long long intGroupNum = stoi(strGroupNum);
					   if (LMTWhiteList.count(intGroupNum))
					   {
						   e.group.sendMsg("该群已存在在LMT白名单中");
						   return;
					   }
					   LMTWhiteList.insert(intGroupNum);
					   if (LmtGroupList.count(intGroupNum))
					   {
						   LmtGroupList.erase(intGroupNum);
					   }
					   e.group.sendMsg("已将" + strGroupNum + "加入白名单");
					   return;
				   }
				   else if (strLowerMessage.substr(intMsgCnt, 3) == "rmv")
				   {
					   intMsgCnt += 3;
					   while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						   intMsgCnt++;
					   string strGroupNum;
					   while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					   {
						   strGroupNum += strLowerMessage[intMsgCnt];
						   intMsgCnt++;
					   }
					   if (strGroupNum.empty())
					   {
						   e.group.sendMsg("试图移除LMT白名单的群号为空");
						   return;
					   }
					   unsigned long long intGroupNum = stoi(strGroupNum);
					   if (!LMTWhiteList.count(intGroupNum))
					   {
						   e.group.sendMsg("该群不在LMT白名单中");
						   return;
					   }
					   LMTWhiteList.erase(intGroupNum);
					   IdleTimer(intGroupNum);
					   e.group.sendMsg("已将" + strGroupNum + "从白名单移除");
					   return;
				   }
				   else
				   {
					   return;
				   }
			   }
			   else if (strLowerMessage.substr(intMsgCnt, 2) == "to")
			   {
				  intMsgCnt += 2;
				  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					  intMsgCnt++;
				  if (strLowerMessage.substr(intMsgCnt, 5) == "group")
				  {
					  intMsgCnt += 5;
					  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						  intMsgCnt++;
					  string strGroupId;
					  while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					  {
						  strGroupId += strLowerMessage[intMsgCnt];
						  intMsgCnt++;
					  }
					  if (strGroupId.empty())
					  {
						  e.group.sendMsg("未找到群号");
						  return;
					  }
					  unsigned long long GroupId = stoi(strGroupId);
					  vector<unsigned long long>GroupList = e.bot.getGroupList();
					  vector<unsigned long long>::iterator result = find(GroupList.begin(), GroupList.end(), GroupId);
					  if (result == GroupList.end())//检测群列表里是否存在该群
					  {
						  e.group.sendMsg("您拨打的群号是空号，请查证后再拨");
						  return;
					  }
					  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						  intMsgCnt++;
					  if (intMsgCnt != msg.length())
					  {
						  Group(GroupId, e.bot.id).sendMsg("来自Master的消息：" + msg.substr(intMsgCnt));
						  e.group.sendMsg("消息已发送往" + Group(GroupId, e.bot.id).nickOrNameCard() + strGroupId);
					  }
					  else
					  {
						  e.group.sendMsg("留言为空");
					  }
				  }
				  else if (strLowerMessage.substr(intMsgCnt, 6) == "friend")
				  {
					  intMsgCnt += 6;
					  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						  intMsgCnt++;
					  string strQQId;
					  while (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					  {
						  strQQId += strLowerMessage[intMsgCnt];
						  intMsgCnt++;
					  }
					  if (strQQId.empty())
					  {
						  e.group.sendMsg("未找到好友ID");
						  return;
					  }
					  unsigned long long QQId = stoi(strQQId);
					  vector<unsigned long long>FriendList = e.bot.getFriendList();
					  vector<unsigned long long>::iterator result = find(FriendList.begin(), FriendList.end(), QQId);
					  if (result == FriendList.end())//检测群列表里是否存在该群
					  {
						  e.group.sendMsg("您拨打的好友是空号，请查证后再拨");
						  return;
					  }
					  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
						  intMsgCnt++;
					  if (intMsgCnt != msg.length())
					  {
						  Friend(QQId, e.bot.id).sendMsg("来自Master的消息：" + msg.substr(intMsgCnt));
						  e.group.sendMsg("消息已发送往" + Friend(QQId, e.bot.id).nickOrNameCard() + strQQId);
					  }
					  else
					  {
						  e.group.sendMsg("留言为空");
					  }
				  }
				  else
				  {

				  }
	
				}
			   else
			   {
				   return;
			   }

			}
		   else if (strLowerMessage.substr(intMsgCnt, 8) == "tomaster")
		   {
				 intMsgCnt += 8;
				 while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					 intMsgCnt++;
				 if (intMsgCnt != msg.length())
				 {
					 Friend(MasterQQId, e.bot.id).sendMsg("来自群" + e.group.nickOrNameCard() + to_string(e.group.id()) +"的" + e.sender.nickOrNameCard() + to_string(e.sender.id()) + "的消息\n" + msg.substr(intMsgCnt));
					 e.group.sendMsg("好的，话我已经传达给柴刀了");
				 }
				 else
				 {
					 e.group.sendMsg("我不知道你想告诉柴刀什么");
				 }
			}
		   else if (strLowerMessage[intMsgCnt] == 'n')
		   {
			  intMsgCnt++;
			  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
				  intMsgCnt++;
			  if (strLowerMessage[intMsgCnt] == 'n')
			  {
				  intMsgCnt++;
				  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					  intMsgCnt++;
				  if (intMsgCnt != strLowerMessage.length())
				  {
					  string strNickName;
					  strNickName = msg.substr(intMsgCnt);
					  if (strNickName.length() >= 50)
					  {
						  e.group.sendMsg(GlobalMsg[EnumErrorMsg.昵称太长]);
						  return;
					  }
					  GroupNickNameList[e.sender.id()][e.group.id()] = strNickName;
					  e.group.sendMsg(GlobalMsg[EnumInfoMsg.群组昵称设置成功] + strNickName);
				  }
				  else
				  {
					  if (GroupNickNameList.count(e.sender.id()) && GroupNickNameList[e.sender.id()].count(e.group.id()))
					  {
						 GroupNickNameList[e.sender.id()].erase(e.group.id());
						 if (GroupNickNameList[e.sender.id()].empty())
						 {
							 GroupNickNameList.erase(e.sender.id());
						  }
						  e.group.sendMsg(GlobalMsg[EnumInfoMsg.全局昵称删除成功]);
					  }
					  else
					  {
						  e.group.sendMsg(GlobalMsg[EnumErrorMsg.删除昵称不存在]);
					  }
				  }
			  }
			  else
			  {
				  while (isspace(static_cast<unsigned char>(strLowerMessage[intMsgCnt])))
					  intMsgCnt++;
				  if (intMsgCnt != strLowerMessage.length())
				  {
					  string strNickName;
					  strNickName = msg.substr(intMsgCnt);
					  if (strNickName.length() >= 50)
					  {
						  e.group.sendMsg(GlobalMsg[EnumErrorMsg.昵称太长]);
						  return;
					  }
					  GlobalNickName[e.sender.id()] = strNickName;
					  e.group.sendMsg(GlobalMsg[EnumInfoMsg.全局昵称设置成功] + strNickName);
				  }
				  else
				  {
					  if (GlobalNickName.count(e.sender.id()))
					  {
						  GlobalNickName.erase(e.sender.id());
						  e.group.sendMsg(GlobalMsg[EnumInfoMsg.全局昵称删除成功]);
					  }
					  else
					  {
						  e.group.sendMsg(GlobalMsg[EnumErrorMsg.删除昵称不存在]);
					  }
				  }
			  }
			}
		   else if (strLowerMessage[intMsgCnt] == 'r')
		   {
		       IdleTimer(e.group.id());
			   intMsgCnt++;
			   bool isHiden = false, showDetail = false;
			   while (intMsgCnt != strLowerMessage.length()
				   && (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'h' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 's'))
			   {
				   if (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'h')
				   {
					   isHiden = true;
				   }
				   if (static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 's')
				   {
					   showDetail = true;
				   }
				   intMsgCnt++;
			   }
			   int intMultDice = 1, intFindMultDiceCut = intMsgCnt;
			   string strMultDice;
			   while (intFindMultDiceCut != strLowerMessage.length()
				   && (static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut]) != '#' || isdigit(static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut])))
				   && intFindMultDiceCut <= intMsgCnt + 3)
			   {
				   if (isdigit(static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut])))
				   {
					   strMultDice += strLowerMessage[intFindMultDiceCut];
				   }
				   intFindMultDiceCut++;
			   }
			   if (intFindMultDiceCut != strLowerMessage.length() && static_cast<unsigned char>(strLowerMessage[intFindMultDiceCut]) == '#')
			   {
				   if (strMultDice.length() == 0)
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.掷骰轮数无效]);
					   return;
				   }
				   intMultDice = stoi(strMultDice);
				   if (intMultDice > 10)
				   {
					   e.group.sendMsg(GlobalMsg[EnumErrorMsg.掷骰轮数过多]);
					   return;
				   }
				   intMsgCnt = intFindMultDiceCut;
				   intMsgCnt++;
			   }

			   //替换中文括号——————————————————————————
			   string strEquation, strReason;
			   string leftBrackens = "（", rightBrackens = "）";
			   while (strLowerMessage.find("（") != string::npos)
				   strLowerMessage.replace(strLowerMessage.find(leftBrackens), leftBrackens.length(), "(");
			   while (strLowerMessage.find("）") != string::npos)
				   strLowerMessage.replace(strLowerMessage.find(rightBrackens), rightBrackens.length(), ")");
			   while (isspace(static_cast<unsigned char>(msg[intMsgCnt])))
				   intMsgCnt++;
			   //取得算式与原因,并判断算式是否存在——————————————————

			   while (intMsgCnt != strLowerMessage.length() && (isdigit(static_cast<unsigned char>(strLowerMessage[intMsgCnt])) || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '+'
				   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '-' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '*'
				   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == 'd' || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == '('
				   || static_cast<unsigned char>(strLowerMessage[intMsgCnt]) == ')'))
			   {
				   strEquation += strLowerMessage[intMsgCnt];
				   intMsgCnt++;
			   }
			   if (strEquation.length() == 0)
			   {
				   if (DefaultDice.count(e.sender.id()))
				   {
					   strEquation = "d" + to_string(DefaultDice[e.sender.id()]);
				   }
				   else
					   strEquation = "d100";
			   }
			   if (intMsgCnt != strLowerMessage.length())
				   strReason = msg.substr(intMsgCnt);



			   string strReply;
			   if (strReason.length() != 0)
			   {
				   strReply = "由于" + strReason + ",";
			   }
			   strReply = strReply + strNickName + "骰出了：";

			   if (showDetail)
			   {
				   for (int MultDiceCut = 1; MultDiceCut <= intMultDice; MultDiceCut++)
				   {
					   DiceCalculatorOutput OutputReply = DiceCalculator(strEquation, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
					   if (!OutputReply.complate)
					   {
						   e.group.sendMsg(OutputReply.detail);
						   return;
					   }
					   strReply = strReply + "\n" + OutputReply.detail;
				   }
			   }
			   else
			   {
				   for (int MultDiceCut = 1; MultDiceCut <= intMultDice; MultDiceCut++)
				   {
					   DiceCalculatorOutput OutputReply = DiceCalculator(strEquation, DefaultDice.count(e.sender.id()) ? DefaultDice[e.sender.id()] : 100);
					   if (!OutputReply.complate)
					   {
						   e.group.sendMsg(OutputReply.detail);
						   return;
					   }
					   strReply = strReply + "\n" + strEquation + "=" + to_string(OutputReply.result);
				   }
			   }
			   if (isHiden)
			   {
				   e.group.sendMsg(strNickName + GlobalMsg[EnumInfoMsg.暗骰]);
				   e.sender.sendMsg("在群" + e.group.nickOrNameCard() + "中，" + strReply);
			   }
			   else
			   {
				   e.group.sendMsg(strReply);
			   }
		   }
		   else
		   {
			   return;
		   }
		   logger->info("指令：" + msg + "\t处理完成");
		   return;
        });
        // 监听群临时会话
        procession->registerEvent<GroupTempMessageEvent>([](GroupTempMessageEvent e) {
        });
        // 群事件
        procession->registerEvent<MemberJoinEvent>([](MemberJoinEvent e) {
        });
        procession->registerEvent<MemberLeaveEvent>([](MemberLeaveEvent e) {
        });
        procession->registerEvent<TimeOutEvent>([](TimeOutEvent e){
            logger->info(e.msg);
        });
    }

    void onDisable() override {

		boolRunCirculate = false;
		Sleep(11 * 1000);
		ofstream ofstreamLMTWhite(strFileLoc + "LMTWhite.RDconf", ios::out | ios::trunc);
		for (auto it = LMTWhiteList.begin(); it != LMTWhiteList.end(); ++it)
		{
			ofstreamLMTWhite << *it << std::endl;
		}
		ofstreamLMTWhite.close();
		logger->info("LMT白名单存储完成");
		ofstream ofstreamLMTList(strFileLoc + "LMTList.RDconf", ios::out | ios::trunc);
		for (auto it = LmtGroupList.begin(); it != LmtGroupList.end(); ++it)
		{
			ofstreamLMTList << it->first << " " << it->second << std::endl;
		}
		ofstreamLMTList.close();
		logger->info("LMT列表存储完成");
		ofstream ofstreamJrrpMap(strFileLoc + "JrrpMap.RDconf", ios::out | ios::trunc);
		for (auto it = JrrpMap.begin(); it != JrrpMap.end(); ++it)
		{
			ofstreamJrrpMap << it->first << " " << it->second << std::endl;
		}
		ofstreamJrrpMap.close();
		logger->info("Jrrp存储完成");
		ofstream ofstreamGlobalNickName(strFileLoc + "GlobalNickName.RDconf", ios::out | ios::trunc);
		for (auto it = GlobalNickName.begin(); it != GlobalNickName.end(); ++it)
		{
			ofstreamGlobalNickName << it->first << " " << it->second << std::endl;
		}
		ofstreamGlobalNickName.close();
		logger->info("全局昵称存储完成");
		ofstream ofstreamGroupNickName(strFileLoc + "GroupNickName.RDconf", ios::out | ios::trunc);
		for (auto it = GroupNickNameList.begin(); it != GroupNickNameList.end(); ++it)
		{
			for (auto it1 = it->second.begin(); it1 != it->second.end(); ++it1)
			{
				ofstreamGroupNickName << it->first << " " << it1->first << " " << it1->second << std::endl;
			}
		}
		ofstreamGroupNickName.close();
		logger->info("群组昵称存储完成");
		ofstream ofstreamDisabledGroup(strFileLoc + "DisabledGroup.RDconf", ios::out | ios::trunc);
		for (auto it = DisabledGroup.begin(); it != DisabledGroup.end(); ++it)
		{
			ofstreamDisabledGroup << *it << std::endl;
		}
		ofstreamDisabledGroup.close();
		logger->info("关闭群组存储完成");
		ofstream ofstreamWelcomeMsg(strFileLoc + "WelcomeMsg.RDconf", ios::out | ios::trunc);
		for (auto it = WelcomeMsg.begin(); it != WelcomeMsg.end(); ++it)
		{
			while (it->second.find(' ') != string::npos)it->second.replace(it->second.find(' '), 1, "{space}");
			while (it->second.find('\t') != string::npos)it->second.replace(it->second.find('\t'), 1, "{tab}");
			while (it->second.find('\n') != string::npos)it->second.replace(it->second.find('\n'), 1, "{endl}");
			while (it->second.find('\r') != string::npos)it->second.replace(it->second.find('\r'), 1, "{enter}");
			ofstreamWelcomeMsg << it->first << " " << it->second << std::endl;
		}
		ofstreamWelcomeMsg.close();
		logger->info("群组欢迎信息存储完成");
		ofstream ofstreamCharacterProp(strFileLoc + "CharacterProp.RDconf", ios::out | ios::trunc);
		for (auto it = CharacterProp.begin(); it != CharacterProp.end(); ++it)
		{
			for (auto it1 = it->second.cbegin(); it1 != it->second.cend(); ++it1)
			{
				ofstreamCharacterProp << it->first << " " << it1->first << " " << it1->second << std::endl;
			}
		}
		ofstreamCharacterProp.close();
		logger->info("默认卡存储完成");
		ofstream ofstreamMultCard(strFileLoc + "MultCard.RDconf", ios::out | ios::trunc);
		for (auto QQID = MultCharProp.begin(); QQID != MultCharProp.end(); ++QQID)
		{
			map<string, PropType> CharList = QQID->second;
			for (auto CharCount = CharList.begin(); CharCount != CharList.end(); ++CharCount)
			{
				map<string, int> PropList = CharCount->second;
				for (auto PropCount = PropList.begin(); PropCount != PropList.end(); ++PropCount)
				{
					ofstreamMultCard << QQID->first << " " << CharCount->first << " " << PropCount->first << " " << PropCount->second << std::endl;
				}
			}
		}
		ofstreamMultCard.close();
		logger->info("角色卡存储完成");
		ofstream ofstreamGroupCard(strFileLoc + "GroupCard.RDconf", ios::out | ios::trunc);
		for (auto PlayerGroup = PlayerGroupList.begin(); PlayerGroup != PlayerGroupList.end(); ++PlayerGroup)
		{
			map<unsigned long long, string> GroupChar = PlayerGroup->second;
			for (auto GroupCount = GroupChar.begin(); GroupCount != GroupChar.end(); ++GroupCount)
			{
				ofstreamGroupCard << PlayerGroup->first << " " << GroupCount->first << " " << GroupCount->second << std::endl;
			}
		}
		ofstreamGroupCard.close();
		logger->info("角色卡绑定存储完成");
		ofstream ofstreamRoomRule(strFileLoc + "RoomRule.RDconf", ios::out | ios::trunc);
		for (auto it = RoomRule.begin(); it != RoomRule.end(); ++it)
		{
			ofstreamRoomRule << it->first << " " << it->second << std::endl;
		}
		ofstreamRoomRule.close();
		logger->info("村规存储完成");
        /*插件结束*/
    }
};

// 绑定当前插件实例
void MiraiCP::enrollPlugin(){
    MiraiCP::enrollPlugin0(new Main());
}