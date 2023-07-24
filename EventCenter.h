#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<stack>
#include<queue>
#include<sstream>
#include<unordered_map>//哈希表
#include<functional>
#include"InfoStruct.h"

//默认信息函数
extern void Info(std::string str);
extern void InfoNoChangeLine(std::string str);


class EventInfo //纯虚接口
{
public:
	virtual ~EventInfo() {};
};

class MemberFunction //纯虚接口
{
public:
	virtual void Execute() {}
};

//临时数据段
struct TempData { TempData() {} };

template<typename T1, typename T2>
struct DoubleParamTempData :public TempData { 
	DoubleParamTempData() {} 
	T1 m_param1; T2 m_param2; 
};

template<typename T1>
struct SingleParamTempData :public TempData { 
	SingleParamTempData() {} 
	T1 m_param1; 
};

extern TempData* g_tempdata;//使用的临时数据段指针（）

//成员函数指针与执行单元
template<typename CLASS, typename T1, typename T2>
class DoubleParamMemberFunction : public MemberFunction {
public:
	CLASS* m_obj;
	void(CLASS::*m_action)(T1, T2);
	void Execute() { 
		DoubleParamTempData<T1, T2>* dtempdata = (DoubleParamTempData<T1, T2>*)g_tempdata;
		std::invoke(m_action, m_obj, dtempdata->m_param1, dtempdata->m_param2);
	}
};
template<typename CLASS, typename T1>
class SingleParamMemberFunction : public MemberFunction {
public:
	CLASS* m_obj;
	void(CLASS::* m_action)(T1);
	void Execute() { 
		SingleParamTempData<T1>* dtempdata = (SingleParamTempData<T1>*)g_tempdata;
		std::invoke(m_action, m_obj, dtempdata->m_param1);
	}
};
template<typename CLASS>
class NoneParamMemberFunction : public MemberFunction {
public:
	CLASS* m_obj;
	void(CLASS::* m_action)();
	void Execute() { 
		std::invoke(m_action,m_obj);
	}
};

//双参数事件
template<typename T1, typename T2>
class DoubleParamEventInfo : public EventInfo
{
public:
	DoubleParamEventInfo() { }
	~DoubleParamEventInfo() {
		for (MemberFunction* mfunc : m_doubleParamFunc) {
			if (mfunc) {
				delete mfunc;
			}
		}
		m_doubleParamFunc.clear();
		m_doubleParamFunc.shrink_to_fit();
	}
	std::vector<MemberFunction*> m_doubleParamFunc;//事件函数数组
};

//单参数事件
template<typename T1>
class SingleParamEventInfo : public EventInfo
{
public:
	SingleParamEventInfo() { }
	~SingleParamEventInfo() {
		for (MemberFunction* mfunc : m_singleParamFunc) {
			if (mfunc) {
				delete mfunc;
			}
		}
		m_singleParamFunc.clear();
		m_singleParamFunc.shrink_to_fit();
	}
	std::vector<MemberFunction*> m_singleParamFunc;//事件函数数组
};

//无参数事件
class NoneParamEventInfo : public EventInfo
{
public:
	NoneParamEventInfo() {	}
	~NoneParamEventInfo() {
		for (MemberFunction* mfunc : m_noneParamFunc) {
			if (mfunc) {
				delete mfunc;
			}
		}
		m_noneParamFunc.clear();
		m_noneParamFunc.shrink_to_fit();
	}
	std::vector<MemberFunction*> m_noneParamFunc;//事件函数数组
};

//评分使用的事件
static enum class EventName
{
	//评分使用事件写这下面
	CaughtError/*ErrorInfo错误信息*/,
	CaughtWarning/*WarningInfo警告消息*/,
	CycleComplexity/*int圈复杂度*/,
	ParamNum/*int函数参数数量*/,
	CallDepth/*链式调用复杂度*/,
	BlockDepth/*代码块深度*/,
	CloneCode/*CloneInfo代码重复信息*/,

	//发送整体分数信息
	StmtInfo /*代码块分数信息*/,
	FunctionInfo /*函数分数信息*/,
	ClassInfo /*类分数信息*/,
	GlobalInfo /*全局分数信息*/,

	//其他事件写这下面
	CodeGen/*代码优化之后的信息*/,
	AnaStep/*此时分析的进度*/
};

//事件管理中心
class EventCenter {
public:
	static EventCenter g_eventCenter;//唯一单例

	//重置并且删除所有监听器
	void Reset();

	EventCenter();
	~EventCenter();

	std::unordered_map<EventName, EventInfo*>* m_listenerMap;
	std::vector<EventInfo*> m_allListener;//用于回收内存

	//注册监听器
	template<typename T1, typename T2, typename CLASS>
	void AddEventListener(EventName name, void(CLASS::*action)(T1, T2), CLASS* obj)
	{
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			DoubleParamEventInfo<T1, T2>* info = (DoubleParamEventInfo<T1, T2>*)einfo;
			DoubleParamMemberFunction<CLASS, T1, T2>* func = new DoubleParamMemberFunction<CLASS,T1,T2>();
			func->m_action = action;
			func->m_obj = obj;
			info->m_doubleParamFunc.push_back(func);
		}
		else {
			DoubleParamEventInfo<T1, T2>* info = new DoubleParamEventInfo<T1, T2>();
			DoubleParamMemberFunction<CLASS, T1, T2>* func = new DoubleParamMemberFunction<CLASS, T1, T2>();
			func->m_action = action;
			func->m_obj = obj;
			info->m_doubleParamFunc.push_back(func);
			m_listenerMap->emplace(name, info);
			m_allListener.push_back(info);
		}
	}

	template<typename T1, typename CLASS>
	void AddEventListener(EventName name, void(CLASS::*action)(T1), CLASS* obj)
	{
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			SingleParamEventInfo<T1>* info = (SingleParamEventInfo<T1>*)einfo;
			SingleParamMemberFunction<CLASS, T1>* func = new SingleParamMemberFunction<CLASS, T1>();
			func->m_action = action;
			func->m_obj = obj;
			info->m_singleParamFunc.push_back(func);
 		}
		else {
			SingleParamEventInfo<T1>* info = new SingleParamEventInfo<T1>();
			SingleParamMemberFunction<CLASS, T1>* func = new SingleParamMemberFunction<CLASS, T1>();
			func->m_action = action;
			func->m_obj = obj;
			info->m_singleParamFunc.push_back(func);
			m_listenerMap->emplace(name, info);
			m_allListener.push_back(info);
		}
	}

	template<typename CLASS>
	void AddEventListener(EventName name, void(CLASS::*action)(), CLASS* obj)
	{
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			NoneParamEventInfo* info = (NoneParamEventInfo*)einfo;
			NoneParamMemberFunction<CLASS>* func = new NoneParamMemberFunction<CLASS>();
			func->m_action = action;
			func->m_obj = obj;
			info->m_noneParamFunc.push_back(func);
		}
		else {
			NoneParamEventInfo* info = new NoneParamEventInfo();
			NoneParamMemberFunction<CLASS>* func = new NoneParamMemberFunction<CLASS>();
			func->m_action = action;
			func->m_obj = obj;
			info->m_noneParamFunc.push_back(func);
			m_listenerMap->emplace(name, info);
			m_allListener.push_back(info);
		}
	}

	//移除监听器
	template<typename T1, typename T2, typename CLASS>
	void RemoveEventListener(EventName name, void(CLASS::*action)(T1, T2),CLASS* obj)
	{
		if (!action) {
			return;
		}
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			DoubleParamEventInfo<T1, T2>* info = (DoubleParamEventInfo<T1, T2>*)einfo;
			std::vector<MemberFunction*>::iterator itor = info->m_doubleParamFunc.begin();
			while (true)
			{
				DoubleParamMemberFunction<CLASS, T1, T2>* func = (DoubleParamMemberFunction<CLASS, T1, T2>*)*itor;
				if (func->m_action == action && func->m_obj == obj) {
					delete func;//回收内存
					info->m_doubleParamFunc.erase(itor);
					break;
				}
				if (itor == info->m_doubleParamFunc.end()) {
					break;
				}
				else {
					itor++;
				}
			}
		}
	}

	template<typename T1, typename CLASS>
	void RemoveEventListener(EventName name, void(CLASS::*action)(T1),CLASS* obj)
	{
		if (!action) {
			return;
		}
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			SingleParamEventInfo<T1>* info = (SingleParamEventInfo<T1>*)einfo;
			std::vector<MemberFunction*>::iterator itor = info->m_singleParamFunc.begin();
			while (true)
			{
				SingleParamMemberFunction<CLASS, T1>* func = (SingleParamMemberFunction<CLASS, T1>*) * itor;
				if (func->m_action == action && func->m_obj == obj) {
					delete func;//回收内存
					info->m_singleParamFunc.erase(itor);
					break;
				}
				if (itor == info->m_singleParamFunc.end()) {
					break;
				}
				else {
					itor++;
				}
			}
		}
	}

	template<typename CLASS>
	void RemoveEventListener(EventName name, void(CLASS::*action)(),CLASS* obj)
	{
		if (!action) {
			return;
		}
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			NoneParamEventInfo* info = (NoneParamEventInfo*)einfo;
			std::vector<MemberFunction*>::iterator itor = info->m_noneParamFunc.begin();
			while (true)
			{
				NoneParamMemberFunction<CLASS>* func = (NoneParamMemberFunction<CLASS>*) * itor;
				if (func->m_action == action && func->m_obj == obj) {
					delete func;//回收内存
					info->m_noneParamFunc.erase(itor);
					break;
				}
				if (itor == info->m_noneParamFunc.end()) {
					break;
				}
				else {
					itor++;
				}
			}
		}
	}

	//触发监听器
	template<typename T1, typename T2>
	void EventTrigger(EventName name, T1 p1, T2 p2)
	{
		g_tempdata = new DoubleParamTempData<T1,T2>();
		DoubleParamTempData<T1, T2>* dtempdata = (DoubleParamTempData<T1, T2>*)g_tempdata;
		dtempdata->m_param1 = p1;
		dtempdata->m_param2 = p2;
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			DoubleParamEventInfo<T1, T2>* deinfo = (DoubleParamEventInfo<T1, T2>*)einfo;
			for (MemberFunction* func : deinfo->m_doubleParamFunc) {
				func->Execute();
			}
		}
		delete g_tempdata;//回收内存
	}

	template<typename T1>
	void EventTrigger(EventName name, T1 p1)
	{
		g_tempdata = new SingleParamTempData<T1>();
		SingleParamTempData<T1>* dtempdata = (SingleParamTempData<T1>*)g_tempdata;
		dtempdata->m_param1 = p1;
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			SingleParamEventInfo<T1>* seinfo = (SingleParamEventInfo<T1>*)einfo;
			for (MemberFunction* func : seinfo->m_singleParamFunc) {
				func->Execute();
			}
		}
		delete g_tempdata;//回收内存
	}

	void EventTrigger(EventName name)
	{
		if (m_listenerMap->count(name)) {
			EventInfo* einfo = m_listenerMap->find(name)->second;
			NoneParamEventInfo* neinfo = (NoneParamEventInfo*)einfo;
			for (MemberFunction* func : neinfo->m_noneParamFunc) {
				func->Execute();
			}
		}
	}

};

//分析进度管理
extern AnalyzeStep g_currentAnalyze;
extern void EnterAnalyzeStep(std::string name, int total, int current);
extern void AddAnalyzeStep();