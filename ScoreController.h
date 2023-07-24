#pragma once
#include"ScoreUnit.h"
#include"EigenWord.h"

class ScoreController
{
public:
	static ScoreController g_scoreController;//唯一单例

	ScoreController();
	~ScoreController();

	void EnterGlobalScoreUnitVector(ScoreUnitVector*& scvector);
	void EnterFunctionScoreUnitVector(ScoreUnitVector*& scvector, 
										SourceLocation beginloc, SourceLocation endloc, std::string name);
    void EnterClassScoreUnitVector(ScoreUnitVector *&scvector,
                                       SourceLocation beginloc, SourceLocation endloc, std::string name);
    void EnterStmtScoreUnitVector(ScoreUnitVector *&scvector,
                                      SourceLocation beginloc, SourceLocation endloc, std::string name);
    void EnterScoreUnitVectorWithoutInit(ScoreUnitVector *&scvector);
	void ExitScoreUnitVector();

	//获得所有代码的总分
	int CalculateTotalScore();
	//获得所有代码某一项的总分
	int CalculateTotalScore(ScoreUnit::ScoreUnitType type);
	//获得一个类/函数/所有代码的总分
	int CalculateScore(ScoreUnitVector* suvector);
	//获得一个类/函数/所有代码某一项的总分
	int CalculateScore(ScoreUnitVector* suvector, ScoreUnit::ScoreUnitType type);

	//输出信息展示
	void Display();
	//重置
	void Reset();
	//输出所有度量元信息
    void DisplayAllScoreUnitVectors();

	ScoreUnitVector *m_globalUnits = nullptr; //储存公共域的代码度量元的数组
private:
	ScoreUnitVector* m_currentUnits = nullptr;//储存目前存放的代码度量元的数组
	ScoreUnitVector* m_fatherUnits = nullptr;//储存目前代码度量元数组的上一级代码度量元数组
	std::stack<ScoreUnitVector*> m_allUnits;//存放所有代码度量元栈(用于判断目前在哪个类/函数)
	std::vector<ScoreUnitVector*> m_allUsedUnits;//存放所有使用过的代码度量元栈(用于回收内存)
};

