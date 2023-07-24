#pragma once
#include"EventCenter.h"
#include "CreateObjList.h"
#include "EigenWord.h"
class ScoreUnit;

//代码度量元数组
class ScoreUnitVector {
public:
  enum SUVType {Global, Record, Function, Stmt};
  void InitPos(clang::SourceManager *sm, clang::SourceLocation beginloc,
               clang::SourceLocation endloc, std::string name) {
    m_beginLoc.Init(sm, beginloc);
    m_endLoc.Init(sm, endloc);
    m_name = name;
	}

	//该代码度量元数组中的代码度量元
	std::vector<ScoreUnit*> m_scoreUnits;
	unsigned m_charNumWeight = 1;//目前用该度量元数组对应的域中的char数作为权重

	//该代码度量元数组的子代码度量元数组
	//如一个类中有三个函数,该容器就有指向这三个函数代码度量数组的指针
	std::vector<ScoreUnitVector*> m_childScoreUnits;
    
	FixedLoc m_beginLoc;
    FixedLoc m_endLoc;
    std::string m_name = ""; //标记用的名称
    SUVType m_type = Global;

	void UnregisterAllEvents();
	void RegisterAllEvents();
	void FreeMemory();
    void Display();//测试用输出
};


//可序列化的代码度量元信息结果
struct ScoreUnitConfig {
  ScoreUnitConfig(){}
  std::string m_name = ""; //代码度量元名字
  std::string m_display = ""; //代码度量元显示名字
  int m_weight = 0; //该子代码度量元的权重
  int m_unittype; //是四种类型中的哪几种（NORMATIVE+EFFICIENCY...）

  std::vector<float> m_configValues;//超参数
  float GetPValue(int index) { return m_configValues.at(index);}
};
extern std::unordered_map<std::string, ScoreUnitConfig> g_scoreUnitConfigs;
extern void InitScoreUnitConfigs();

JS_OBJ_EXT(ScoreUnitConfig, m_name, m_display, m_weight, m_unittype, m_configValues);

//NEMS 1 2 4 8

//代码度量元父类
class ScoreUnit {
public:
	//四种代码度量元类型
	static enum ScoreUnitType {
		NORMATIVE/*规范性N*/= 0b0001, EFFICIENCY = 0b0010/*执行效率E*/, MAINTAINABILITY = 0b0100/*可维护性M*/, SECURITY = 0b1000/*安全性(代码风险)S*/
	};

	ScoreUnit(ScoreUnitVector* myvec) { m_myvector = myvec; };

	ScoreUnitConfig *m_config;
	ScoreUnitVector* m_myvector;//记录存储自己的ScoreUnitVecto

	int GetScore();
	void Display();

    virtual void OnAttach() = 0;  //进入该作用域时注册监听器
    virtual void OnDetach() = 0;   //离开该作用域时注销监听器
	virtual int OnCalculate() = 0;//计算度量元分数
	virtual void OnDisplay() {}//返回文字信息
};

/*
* 所有代码度量元继承ScoreUnit
* 函数必须有构造函数,用于初始化各项参数和注册监听器
* OnCalculate()函数必须被定义,用于计算该度量元分数
* OnDisplay()函数必须被定义,用于输出数据信息
*/

//编译错误度量元-S
class CompileErrorUnit :public ScoreUnit {
public:
  CompileErrorUnit(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["CompileErrorUnit"];
  }
	int OnCalculate();
	void OnDisplay();

	//采用错误数:词数来评价
	int m_errors = 0;
	
	void OnAttach();
    void OnDetach();
	void AddErrors(IErrorInfo errorInfo);
};

//编译警告度量元-S
class CompileWarningUnit :public ScoreUnit {
public:
  CompileWarningUnit(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["CompileWarningUnit"];
  }
	int OnCalculate();
	void OnDisplay();

	//采用警告数:词数来评价
	int m_warnings[3] = { 0,0,0 };//三个等级的警告(Info Warning)

	void OnAttach();
    void OnDetach();
	void AddWarnings(IWarningInfo warningInfo);
};

//函数圈复杂度度量元-M-E-N
class CycleComplexityUnit :public ScoreUnit {
public:
  CycleComplexityUnit(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["CycleComplexityUnit"];
  }
	int OnCalculate();
	void OnDisplay();

	int m_cyclecomplexity = 0;

	void OnAttach();
    void OnDetach();
	void SetCycleComplexity(int ccomplexity);
};
//重复度分析度量元-M-E
class CloneCodeUnit :public ScoreUnit {
public:
  CloneCodeUnit(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["CloneCodeUnit"];
  }
	int OnCalculate();
	void OnDisplay();

	int m_normalclone = 0;
	int m_worseclone = 0;

	void OnAttach();
    void OnDetach();
	void CatchClone(CloneInfo cloneinfo);
};
//过长函数/类代码度量元-M-N
class TooLongUnit :public ScoreUnit {
public:
  TooLongUnit(ScoreUnitVector *myvec, bool isfunc) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["TooLongUnit"];
    m_isfunction = isfunc;
  }
	int OnCalculate();
	void OnDisplay();

	void OnAttach();
    void OnDetach();
	bool m_isfunction = false;
};
//过多参数函数度量元-M-N
class TooManyParam :public ScoreUnit {
public:
  TooManyParam(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["TooManyParam"];
  }
	int OnCalculate();
	void OnDisplay();

	int m_paramnums = 0;

	void OnAttach();
    void OnDetach();
	void SetParamNum(int pnum);
};

//过长的链式调用度量元-M-N
class TooLongCall :public ScoreUnit {
public:
  TooLongCall(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["TooLongCall"];
  }
	int OnCalculate();
	void OnDisplay();

	int m_callingdepth[9] = {0,0,0,0,0,0,0,0,0};//1记录深度为1,2记录深度为2...8记录深度>7

	void OnAttach();
    void OnDetach();
	void AddCallDepth(int cdepth);
};
//过深代码块度量元-M
class TooDepthCall :public ScoreUnit {
public:
  TooDepthCall(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["TooDepthCall"];
  }
	int OnCalculate();
	void OnDisplay();

	int m_callingdepth[10] = { 0,0,0,0,0,0,0,0,0,0};//1记录深度为1,2记录深度为2...9记录深度>8

	void OnAttach();
    void OnDetach();
	void AddBlockDepth(int cdepth);
};
//命名规范度量元-M-N
class ItemNaming: public ScoreUnit {
public:
  ItemNaming(ScoreUnitVector *myvec) : ScoreUnit(myvec) {
    m_config = &g_scoreUnitConfigs["ItemNaming"];
  }
  int OnCalculate();
  void OnDisplay();

  void OnAttach();
  void OnDetach();
  void AddItemNaming(int cdepth);
};
