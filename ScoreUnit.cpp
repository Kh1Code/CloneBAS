#include "ScoreUnit.h"
#define P(x) m_config->GetPValue(x)

std::unordered_map<std::string, ScoreUnitConfig> g_scoreUnitConfigs;
void InitScoreUnitConfigs() {
  auto json_obj_list = CreateObjList<ScoreUnitConfig>("score_config.json");
  //通过JsonObjectList对象的obj_list成员即可获取vector<B>
  for (ScoreUnitConfig sconfig : json_obj_list.obj_list) {
    g_scoreUnitConfigs.emplace(sconfig.m_name, sconfig);
  }
}

void ScoreUnitVector::UnregisterAllEvents()
{
	for (ScoreUnit* sunit : m_scoreUnits) {
		sunit->OnDetach();
	}
}

void ScoreUnitVector::RegisterAllEvents()
{
	for (ScoreUnit* sunit : m_scoreUnits) {
		sunit->OnAttach();
	}
}

void ScoreUnitVector::FreeMemory()
{
	for (ScoreUnit* unit : m_scoreUnits) {
		delete unit;
	}
	m_scoreUnits.clear();
	m_scoreUnits.shrink_to_fit();
	m_childScoreUnits.clear();
	m_childScoreUnits.shrink_to_fit();
	delete this;
}

void ScoreUnitVector::Display() { 
	Info("------------------------------------------------------"); 
	switch (m_type) {
        case SUVType::Global: 
			Info("Type: Global"); 
			break;
        case SUVType::Function:
			Info("Type: Function");
			break;
        case SUVType::Record: 
			Info("Type: Record"); 
			break;
        case SUVType::Stmt: 
			Info("Type: Stmt"); 
			break;
    }
    if (m_beginLoc.m_file) {
      Info("File: " + std::string(m_beginLoc.m_file));     
	}
    Info("Name: " + m_name);
    Info("[l:c]BeginLoc:(" + std::to_string(m_beginLoc.m_line) + ":" +
             std::to_string(m_beginLoc.m_colum) + "),EndLoc(" +
             std::to_string(m_endLoc.m_line) + ":" +
             std::to_string(m_endLoc.m_colum) + ")");
    Info("ScoreUnits: -----------------> ");
    for (auto su : m_scoreUnits) {
      su->Display();
	}
	Info("------------------------------------------------------");
}

void ScoreUnit::Display() {
    Info("代码度量元名称: " + m_config->m_display +
       "    代码度量元权重:" + std::to_string(m_config->m_weight));
	InfoNoChangeLine("该代码度量元属于: ");
    if (m_config->m_unittype & 0b1000) {
		Info("规范性 ");
	}
    if (m_config->m_unittype & 0b0100) {
		Info("执行效率 ");
	}
    if (m_config->m_unittype & 0b0010) {
		Info("可维护性 ");
	}
    if (m_config->m_unittype & 0b0001) {
		Info("安全性(代码风险) ");
	}
	Info("该代码度量元数据段: ");
	OnDisplay();
	Info("");
}

int ScoreUnit::GetScore()
{
	int score = OnCalculate();
	if (score > 100) {
		return 100;
	}
	if (score < 0) {
		return 0;
	}
	return score;
}

/*
*编译器错误
*/

int CompileErrorUnit::OnCalculate()
{
	double errornums = m_errors;
	double toknums = m_myvector->m_charNumWeight;
	if (m_errors > 0) {
		return 60.0f - (errornums/toknums) * P(0);
	}
	else {
		return 100;
	}
}
void CompileErrorUnit::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(
      EventName::CaughtError, &CompileErrorUnit::AddErrors, this);
}
void CompileErrorUnit::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::CaughtError, &CompileErrorUnit::AddErrors, this);
}
void CompileErrorUnit::AddErrors(IErrorInfo errorInfo)
{
	m_errors++;
}
void CompileErrorUnit::OnDisplay()
{
	Info("错误数量: " + std::to_string(m_errors));
}

/*
*编译器/分析器警告
*/

int CompileWarningUnit::OnCalculate()
{
	float totalwarning = 0;
	for (int i = 0; i < 3; i++) {
		totalwarning += (float)m_warnings[i] * P(i);
	}
	float toknums = m_myvector->m_charNumWeight;
	return 100 - (totalwarning / toknums);
}

void CompileWarningUnit::OnDisplay()
{
	for (int i = 0; i < 2; i++) {
		Info("安全性警告[等级:"+std::to_string(i+1)+"]数量: " + std::to_string(m_warnings[i]));
	}
}

void CompileWarningUnit::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(
      EventName::CaughtWarning, &CompileWarningUnit::AddWarnings, this);
}

void CompileWarningUnit::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::CaughtWarning, &CompileWarningUnit::AddWarnings, this);
}

void CompileWarningUnit::AddWarnings(IWarningInfo warningInfo)
{
	//只记录安全性警告 0->Info 1->Warning 2->Fetal
	if (warningInfo.m_calScore) {
          int wlevel = warningInfo.m_warningLevel;
          m_warnings[wlevel]++;
	}
}

/*
*函数圈复杂度
*/

int CycleComplexityUnit::OnCalculate()
{
	//5,10,20,30分界点
	int score = 0;
	//0~50分
    float offset = m_myvector->m_charNumWeight;
	if (m_cyclecomplexity > 30) {
      score = 50 - (float)(m_cyclecomplexity - 30) * P(3) / offset;
	}
	//0~70分
	else if (m_cyclecomplexity > 20) {
      score = 70 - (float)(m_cyclecomplexity - 20) * P(2) / offset;
	}
	//0~90分
	else if (m_cyclecomplexity > 10) {
      score = 90 - (float)(m_cyclecomplexity - 10) * P(1) / offset;
	}
	//0~100分
	else if (m_cyclecomplexity > 5) {
      score = 100 - (float)(m_cyclecomplexity - 5) * P(0) / offset;
	}
	//100分
	else {
		score = 100;
	}
	return score;
}

void CycleComplexityUnit::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(
      EventName::CycleComplexity, &CycleComplexityUnit::SetCycleComplexity,
      this);
}

void CycleComplexityUnit::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::CycleComplexity, &CycleComplexityUnit::SetCycleComplexity,
      this);
}

void CycleComplexityUnit::OnDisplay()
{
	Info("圈复杂度: " + std::to_string(m_cyclecomplexity));
}

void CycleComplexityUnit::SetCycleComplexity(int ccomplexity)
{
	m_cyclecomplexity = ccomplexity;
}

/*
*过长函数/类
*/

int TooLongUnit::OnCalculate()
{
	//函数实现大小判断
	if (m_isfunction) {
		if (m_myvector->m_charNumWeight < 400) {
			return 100;
		}
		else if (m_myvector->m_charNumWeight < 1000) {
			return 80;
		}
		else {
           return 60 - (float)(m_myvector->m_charNumWeight - 1000) / 50.0f;
		}
	}
	//类大小判断
	else {
		if (m_myvector->m_charNumWeight < 200) {
			return 100;
		}
		else if (m_myvector->m_charNumWeight < 500) {
			return 80;
		}
		else {
            return 60 - (float)(m_myvector->m_charNumWeight - 500) / 40.0f;
		}
	}

}

void TooLongUnit::OnAttach() {

}

void TooLongUnit::OnDetach() {

}

void TooLongUnit::OnDisplay()
{
	Info("函数/类字符数量: " + std::to_string(m_myvector->m_charNumWeight));
}

/*
过多参数函数度量元
*/

int TooManyParam::OnCalculate()
{
	if (m_paramnums <= P(0)) {
		return 100;
	} else if (m_paramnums <= P(1)) {
        return 100 - (m_paramnums - P(0)) * P(2);
	}
	else {
        return 60 - (m_paramnums - P(1)) * P(3);
	}
}

void TooManyParam::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(EventName::ParamNum,
                                              &TooManyParam::SetParamNum, this);
}

void TooManyParam::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::ParamNum, &TooManyParam::SetParamNum, this);
}

void TooManyParam::OnDisplay()
{
	Info("函数参数: " + std::to_string(m_paramnums));
}

void TooManyParam::SetParamNum(int pnum)
{
	m_paramnums = pnum;
}

/*
重复度评分单元(暂时只给公共域)
*/

void CloneCodeUnit::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(EventName::CloneCode,
                                              &CloneCodeUnit::CatchClone, this);
}

void CloneCodeUnit::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::CloneCode, &CloneCodeUnit::CatchClone, this);
}


int CloneCodeUnit::OnCalculate()
{
	int reduce_score = m_normalclone * P(0) + m_worseclone * P(1);
	return 100 - (reduce_score / m_myvector->m_charNumWeight);
}

void CloneCodeUnit::OnDisplay()
{
	Info("普通重复匹配节点数: " + std::to_string(m_normalclone));
	Info("严重重复匹配节点数: " + std::to_string(m_worseclone));
}

void CloneCodeUnit::CatchClone(CloneInfo cloneinfo)
{
	if (cloneinfo.m_clonelevel == 1) {
		m_normalclone++;
	}
	else {
		m_worseclone++;
	}
}

/*
过长的链式调用
*/

int TooLongCall::OnCalculate()
{
	int begin_score = 100;
	if (m_callingdepth[8] > 0) {
		begin_score = 60;
	}
	int reduce_score = 0;
	for (int i = 4; i <= 8; i++) {
		reduce_score += m_callingdepth[i] * (i - 2) * 300;
	}
	reduce_score += m_callingdepth[2] * 80;
	reduce_score += m_callingdepth[3] * 160;
	return begin_score - reduce_score / m_myvector->m_charNumWeight;
}

void TooLongCall::OnDisplay()
{
	Info("链式调用复杂程度为2数量: " + std::to_string(m_callingdepth[2]));
	Info("链式调用复杂程度为3数量:: " + std::to_string(m_callingdepth[3]));
	Info("链式调用复杂程度为4数量: " + std::to_string(m_callingdepth[4]));
	Info("链式调用复杂程度为5数量:: " + std::to_string(m_callingdepth[5]));
	Info("链式调用复杂程度为6数量: " + std::to_string(m_callingdepth[6]));
	Info("链式调用复杂程度为7数量:: " + std::to_string(m_callingdepth[7]));
	Info("链式调用复杂程度大于等于8数量: " + std::to_string(m_callingdepth[8]));
}

void TooLongCall::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(EventName::CallDepth,
                                              &TooLongCall::AddCallDepth, this);
}

void TooLongCall::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::CallDepth, &TooLongCall::AddCallDepth, this);
}

void TooLongCall::AddCallDepth(int cdepth)
{
	if (cdepth >7) {
		m_callingdepth[8]++;
	}
	else {
		m_callingdepth[cdepth]++;
	}
}

/*
过深的调用
*/

int TooDepthCall::OnCalculate()
{
	int begin_score = 100;
	if (m_callingdepth[9] > 0) {
		begin_score = 60;
	}
	else if (m_callingdepth[8] > 0) {
		begin_score = 80;
	}
	else if (m_callingdepth[7] > 0) {
		begin_score = 90;
	}
	else if (m_callingdepth[6] > 0) {
		begin_score = 95;
	}
	int reduce_score = 0;
	for (int i = 4; i <= 9; i++) {
		reduce_score += m_callingdepth[i] * (i - 2) * 300;
	}
	reduce_score += m_callingdepth[3] * 100;
	return begin_score - reduce_score / m_myvector->m_charNumWeight;
}

void TooDepthCall::OnDisplay()
{
	Info("代码块调用深度为2数量: " + std::to_string(m_callingdepth[2]));
	Info("代码块调用深度为3数量:: " + std::to_string(m_callingdepth[3]));
	Info("代码块调用深度为4数量: " + std::to_string(m_callingdepth[4]));
	Info("代码块调用深度为5数量:: " + std::to_string(m_callingdepth[5]));
	Info("代码块调用深度为6数量: " + std::to_string(m_callingdepth[6]));
	Info("代码块调用深度为7数量:: " + std::to_string(m_callingdepth[7]));
	Info("代码块调用深度为8数量:: " + std::to_string(m_callingdepth[8]));
	Info("代码块调用深度大于等于9数量: " + std::to_string(m_callingdepth[9]));
}

void TooDepthCall::OnAttach() {
  EventCenter::g_eventCenter.AddEventListener(
      EventName::BlockDepth, &TooDepthCall::AddBlockDepth, this);
}

void TooDepthCall::OnDetach() {
  EventCenter::g_eventCenter.RemoveEventListener(
      EventName::BlockDepth, &TooDepthCall::AddBlockDepth, this);
}


void TooDepthCall::AddBlockDepth(int cdepth)
{
	if (cdepth > 8) {
		m_callingdepth[9]++;
	}
	else {
		m_callingdepth[cdepth]++;
	}
}
