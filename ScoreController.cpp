#include "ScoreController.h"

ScoreController ScoreController::g_scoreController;//唯一单例

ScoreController::ScoreController()
{
	
}

ScoreController::~ScoreController()
{
}

void ScoreController::EnterGlobalScoreUnitVector(ScoreUnitVector*& scvector)
{
	//加入所有公共域需要判断的代码度量元
	bool needtopush = false;
	if (!scvector) {
		scvector = new ScoreUnitVector();
        scvector->m_type = ScoreUnitVector::Global;
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CompileErrorUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CompileWarningUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CloneCodeUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooLongCall(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooDepthCall(scvector)));
		needtopush = true;
        m_allUsedUnits.push_back(scvector);
	}
	scvector->RegisterAllEvents();

	//结束增加
	m_currentUnits = scvector;
	m_allUnits.push(scvector);
}

void ScoreController::EnterFunctionScoreUnitVector(ScoreUnitVector *&scvector,
                                                   SourceLocation beginloc, SourceLocation endloc, std::string name) {
	//加入所有函数域需要判断的代码度量元
	bool needtopush = false;
	if (!scvector) {
		scvector = new ScoreUnitVector();
        scvector->InitPos(g_currentsm, beginloc, endloc, name);
		scvector->m_type = ScoreUnitVector::Function;
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CompileErrorUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CompileWarningUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CycleComplexityUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooLongUnit(scvector,true)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooManyParam(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooLongCall(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooDepthCall(scvector)));
        //scvector->m_beginLoc.Init(g_currentsm, funcDecl->getLocation());
		needtopush = true;
		m_allUsedUnits.push_back(scvector);
	}
	scvector->RegisterAllEvents();

	//结束增加
	m_fatherUnits = m_currentUnits;
	if (needtopush) {
		m_fatherUnits->m_childScoreUnits.push_back(scvector);
	}
	m_currentUnits = scvector;
	m_allUnits.push(scvector);
}

void ScoreController::EnterClassScoreUnitVector(ScoreUnitVector *&scvector,
                                                SourceLocation beginloc, SourceLocation endloc, std::string name) {
	//加入所有类(向量)域需要判断的代码度量元
	bool needtopush = false;
	if (!scvector) {
		scvector = new ScoreUnitVector();
        scvector->InitPos(g_currentsm, beginloc, endloc, name);
		scvector->m_type = ScoreUnitVector::Record;
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CompileErrorUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new CompileWarningUnit(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooLongUnit(scvector, true)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooLongCall(scvector)));
		scvector->m_scoreUnits.push_back((ScoreUnit*)(new TooDepthCall(scvector)));
		needtopush = true;
        m_allUsedUnits.push_back(scvector);
	}
	scvector->RegisterAllEvents();

	//结束增加
	m_fatherUnits = m_currentUnits;
	if (needtopush) {
		m_fatherUnits->m_childScoreUnits.push_back(scvector);
	}
	m_currentUnits = scvector;
	m_allUnits.push(scvector);
}

void ScoreController::EnterStmtScoreUnitVector(ScoreUnitVector *&scvector,
                                               SourceLocation beginloc, SourceLocation endloc, std::string name) {
  // 加入所有Stmt 域需要判断的代码度量元 
  bool needtopush = false;
  if (!scvector) {
    scvector = new ScoreUnitVector();
    scvector->m_type = ScoreUnitVector::Stmt;
    scvector->InitPos(g_currentsm, beginloc, endloc, name);
    scvector->m_scoreUnits.push_back((ScoreUnit *)(new CompileErrorUnit(scvector)));
    scvector->m_scoreUnits.push_back((ScoreUnit *)(new CompileWarningUnit(scvector)));
    scvector->m_scoreUnits.push_back((ScoreUnit *)(new TooLongUnit(scvector, true)));
    scvector->m_scoreUnits.push_back((ScoreUnit *)(new TooLongCall(scvector)));
    scvector->m_scoreUnits.push_back((ScoreUnit *)(new TooDepthCall(scvector)));
    needtopush = true;
    m_allUsedUnits.push_back(scvector);
  }
  scvector->RegisterAllEvents();

  //结束增加
  m_fatherUnits = m_currentUnits;
  if (needtopush) {
    m_fatherUnits->m_childScoreUnits.push_back(scvector);
  }
  m_currentUnits = scvector;
  m_allUnits.push(scvector);
}

void ScoreController::EnterScoreUnitVectorWithoutInit(ScoreUnitVector *&scvector) {
  scvector->RegisterAllEvents();
  m_fatherUnits = m_currentUnits;
  m_currentUnits = scvector;
  m_allUnits.push(scvector);
}

void ScoreController::ExitScoreUnitVector()
{
	m_currentUnits->UnregisterAllEvents();//销毁监听器
	m_currentUnits = m_fatherUnits;
	m_allUnits.pop();
	ScoreUnitVector* father = m_allUnits.top();
	m_fatherUnits = father;
}

int ScoreController::CalculateTotalScore()
{
	return CalculateScore(m_globalUnits);
}

int ScoreController::CalculateTotalScore(ScoreUnit::ScoreUnitType type)
{
	return CalculateScore(m_globalUnits,type);
}

int ScoreController::CalculateScore(ScoreUnitVector* suvector)
{
	int score_normative = CalculateScore(suvector, ScoreUnit::ScoreUnitType::NORMATIVE);
	int score_efficiency = CalculateScore(suvector, ScoreUnit::ScoreUnitType::EFFICIENCY);
	int score_maintainability = CalculateScore(suvector, ScoreUnit::ScoreUnitType::MAINTAINABILITY);
	int score_security = CalculateScore(suvector, ScoreUnit::ScoreUnitType::SECURITY);
	return (score_normative + score_efficiency + score_maintainability + score_security) / 4;
}

int ScoreController::CalculateScore(ScoreUnitVector* suvector, ScoreUnit::ScoreUnitType type)
{
	//TODO:完善权重算法

	//计算子函数/类分数
	int totalweight = 0;
	int totalscore = 0;
	for (ScoreUnitVector* childsu : suvector->m_childScoreUnits) {
		totalscore += CalculateScore(childsu, type) * childsu->m_charNumWeight;
		totalweight += childsu->m_charNumWeight;
	}
	//计算包含的代码度量元分数
	int unit_totalweight = 0;
	int unit_totalscore = 0;
	for (ScoreUnit* unit : suvector->m_scoreUnits) {
		if (unit->m_config->m_unittype & type) {
            unit_totalscore += unit->GetScore() * unit->m_config->m_weight;
            unit_totalweight += unit->m_config->m_weight;
		}
	}
	//总分
	if (unit_totalweight != 0) {
		totalscore += (unit_totalscore/unit_totalweight) * suvector->m_charNumWeight;
		totalweight += suvector->m_charNumWeight;
	}
	if (totalweight != 0) {
		return (totalscore / totalweight);
	}
	return 0;
}

void ScoreController::Display()
{
	Info("\n【代码度量元评分】");
	Info("总分: " + std::to_string(CalculateTotalScore()));
	Info("规范性: " + std::to_string(CalculateTotalScore(ScoreUnit::ScoreUnitType::NORMATIVE)));
	Info("执行效率: " + std::to_string(CalculateTotalScore(ScoreUnit::ScoreUnitType::EFFICIENCY)));
	Info("可维护性: " + std::to_string(CalculateTotalScore(ScoreUnit::ScoreUnitType::MAINTAINABILITY)));
	Info("安全性: " + std::to_string(CalculateTotalScore(ScoreUnit::ScoreUnitType::SECURITY)));
}

void ScoreController::Reset()
{
	//清空容器
	while (!m_allUnits.empty()) {
		m_allUnits.pop();
	}
	//回收内存
    for (ScoreUnitVector *sv : m_allUsedUnits) {
      sv->FreeMemory();        
	}
	//重置数据
	m_globalUnits = nullptr;//储存公共域的代码度量元的数组
	m_currentUnits = nullptr;//储存目前存放的代码度量元的数组
	m_fatherUnits = nullptr;//储存目前代码度量元数组的上一级代码度量元数组
}

void ScoreController::DisplayAllScoreUnitVectors() {
  for (ScoreUnitVector* svec : m_allUsedUnits) {
    svec->Display();
  }
}

//中文路径的问题

