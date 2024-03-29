#pragma once
#ifndef SpecialFunctionMap
#define SpecialFunctionMap


#include <map>
#include <string>

struct CocktailType
{
	CocktailType(std::string a = "", std::string b = "", std::string c = "") : CocktailENName(a), CockTailCNName(b), CocktailData(c)
	{
	}
	std::string CocktailENName, CockTailCNName, CocktailData;
};

const struct
{
	std::string BOT = "BOT";
	std::string me指令关闭 = "me指令关闭";
	std::string 欢迎信息清除 = "欢迎信息清除";
	std::string 设置欢迎信息 = "设置欢迎信息";
	std::string 属性存储完成 = "属性存储完成";
	std::string 属性删除完成 = "属性删除完成";
	std::string 技能删除完成 = "技能删除完成";
	std::string 属性查找成功 = "属性查找成功";
	std::string 每日占卜 = "每日占卜";
	std::string 大失败 = "大失败";
	std::string 失败 = "失败";
	std::string 成功 = "成功";
	std::string 困难成功 = "困难成功";
	std::string 极难成功 = "极难成功";
	std::string 大成功 = "大成功";
	std::string sc大失败 = "sc大失败";
	std::string sc失败 = "sc失败";
	std::string sc成功 = "sc成功";
	std::string en失败 = "en失败";
	std::string en成功 = "en成功";
	std::string 清除先攻 = "清除先攻";
	std::string 开启骰子 = "开启骰子";
	std::string 关闭骰子 = "关闭骰子";
	std::string 暗骰 = "暗骰";
	std::string HELP = "HELP";
	std::string 全局昵称设置成功 = "全局昵称设置成功";
	std::string 全局昵称删除成功 = "全局昵称删除成功";
	std::string 群组昵称设置成功 = "群组昵称设置成功";
	std::string 群组昵称删除成功 = "群组昵称删除成功";

}EnumInfoMsg;

const struct
{
	std::string 未知错误 = "未知错误";
	std::string 命名人数过多 = "命名人数过多";
	std::string 命名人数为0 = "命名人数为0";
	std::string 无效的默认骰 = "无效的默认骰";
	std::string 默认骰面数过多 = "默认骰面数过多";
	std::string 默认骰面数为0 = "默认骰面数为0";
	std::string 人物作成数量为0 = "人物作成数量为0";
	std::string 人物作成数量过多 = "人物作成数量过多";
	std::string 人物作成次数无效 = "人物作成次数无效";
	std::string SC格式错误 = "SC格式错误";
	std::string SCsan值错误 = "SCsan值错误";
	std::string 技能值错误 = "技能值错误";
	std::string 删除昵称不存在 = "删除昵称不存在";
	std::string 掷骰表达式错误 = "掷骰表达式错误";
	std::string 群不存在 = "群不存在";
	std::string 消息发送失败 = "消息发送失败";
	std::string 命令失败骰子已被关闭 = "命令失败骰子已被关闭";
	std::string 掷骰次数过多 = "掷骰次数过多";
	std::string 骰子面数过多 = "骰子面数过多";
	std::string 骰子面数为0 = "骰子面数为0";
	std::string 掷骰轮数过多 = "掷骰轮数过多";
	std::string 掷骰轮数无效 = "掷骰轮数无效";
	std::string 欢迎信息为空 = "欢迎信息为空";
	std::string 权限不足 = "权限不足";
	std::string 昵称太长 = "昵称太长";
	std::string 属性或卡不存在 = "属性或卡不存在";
	std::string W指令格式错误 = "W指令格式错误";
	std::string ST指令格式错误 = "ST指令格式错误";
	std::string 属性查找失败 = "属性查找失败";
	std::string 规则查找失败 = "规则查找失败";
	std::string 规则查找格式错误 = "规则查找格式错误";
	std::string 服务器未返回任何信息 = "服务器未返回任何信息";
	std::string 无法获取错误信息 = "无法获取错误信息";
	std::string 访问服务器时出现错误 = "访问服务器时出现错误";
	std::string 先攻表为空 = "先攻表为空";
	std::string 非私聊指令 = "非私聊指令";
	std::string 已在开启状态 = "已在开启状态";
	std::string 已在关闭状态 = "已在关闭状态";
	std::string 存储卡数量过多 = "存储卡数量过多";
	std::string EN指令格式错误 = "EN指令格式错误";
}EnumErrorMsg;


extern std::map<const int, CocktailType>* CocktailList;
extern std::map<const int, std::string> SensojiTempleDivineSign;
extern std::map<const int, std::string> Characteristic;
extern std::map<std::string, int> ENCockList;
extern std::map<std::string, int> CNCockList;
std::string BackCocktailList();
extern std::map<const std::string, std::string> GlobalMsg;

#endif // !SpecialFunctionMap

