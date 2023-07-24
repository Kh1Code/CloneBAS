#include"EventCenter.h"
#include"Log.h"

inline void Info(std::string str) { gInfoLog << str << "\n"; }
inline void InfoNoChangeLine(std::string str) { gInfoLog << str; }

#pragma init_seg(lib)
EventCenter EventCenter::g_eventCenter;//一定要先初始化这个垃圾东西

TempData* g_tempdata = nullptr;//使用的临时数据段指针

EventCenter::EventCenter()
{
	m_listenerMap = new std::unordered_map<EventName, EventInfo*>();
}

EventCenter::~EventCenter()
{
	delete m_listenerMap;
}

void EventCenter::Reset() {
	for (EventInfo* eventi : m_allListener) {
		if (eventi) {
			delete eventi;
		}
	}
	m_allListener.clear();
	m_allListener.shrink_to_fit();
	m_listenerMap->clear();
}

//分析进度管理
AnalyzeStep g_currentAnalyze;
void EnterAnalyzeStep(std::string name, int total, int current) {
	g_currentAnalyze.m_stepName = name;
	g_currentAnalyze.m_totalStep = total;
    g_currentAnalyze.m_currentStep = current;
	EventCenter::g_eventCenter.EventTrigger(EventName::AnaStep, g_currentAnalyze);
}
void AddAnalyzeStep() {
	g_currentAnalyze.m_currentStep++;
	EventCenter::g_eventCenter.EventTrigger(EventName::AnaStep, g_currentAnalyze);
}