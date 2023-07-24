#ifndef INFOSTRUCT
#define INFOSTRUCT
#define _CPPUNWIND
#include<iostream>
#include<string>
#include<set>
#include<map>
#include<unordered_map>//哈希表
#include<boost/json.hpp>

#define VERSION "A2.14"

class StmtNodeInfo;//声明一下

//报错的分数都在这里
struct IErrorInfo {
  IErrorInfo() {}
  IErrorInfo(std::string mes, int line, int colum, int file, int id, std::vector<std::string> phs) {
    m_message = mes;
    m_line = line;
    m_colum = colum;
    m_file = file;
    m_id = id;
    m_placeholders = phs;
  }
  //报错文字信息
  std::string m_message;
  //报错文件
  int m_file = -1;
  //报错行数,-1表示无明确行数
  int m_line = 0;
  //报错列数,-1表示无明确列数
  int m_colum = 0;
  //报错id
  int m_id = 0;
  //占位符
  std::vector<std::string> m_placeholders;
};

//警告的分数都在这里
struct IWarningInfo
{
  IWarningInfo() {}
  IWarningInfo(std::string mes, int line, int colum, int file,
               int warning_type, int wlevel, int id, bool cs,
               std::vector<std::string> phs) {
		m_message = mes;
		m_line = line;
		m_colum = colum;
		m_file = file;
		m_warningtype = warning_type;
        m_warningLevel = wlevel;
                m_id = id;
        m_calScore = cs;
        m_placeholders = phs;
	}
	//报错文字信息
	std::string m_message;
	//报错文件
	int m_file = -1;
	//报错行数,-1表示无明确行数
	int m_line = 0;
	//报错列数,-1表示无明确列数
	int m_colum = 0;
    //警告等级
    int m_warningLevel = -1;
	//警告类型
	int m_warningtype;
	//是否是需要计分的
    bool m_calScore = false;
    //报错id
    int m_id = 0;
    //占位符
    std::vector<std::string> m_placeholders;
};

//局部克隆信息
struct CloneInfo {
	CloneInfo() {}
	CloneInfo(int wlevel) {
		m_clonelevel = wlevel;
	}
	//海明距离
	int m_clonelevel = 0;
};

//克隆位置
struct ClonePosition {
  ClonePosition(){};
  ClonePosition(int fid, int bl, int bc, int el, int ec){
    m_file = fid;
    m_beginLine = bl;
    m_beginColumn = bc;
    m_endLine = el;
    m_endColumn = ec;
  };
  int m_file = -1;
  int m_beginLine = 0;
  int m_beginColumn = 0;
  int m_endLine = 0;
  int m_endColumn = 0;
};

//克隆集合信息
struct CloneSetInfo {
  int m_degree;//0表示普通，1表示普通
  std::vector<ClonePosition> m_cloneSets;//所有重复的位置
};

//全局信息
struct GlobalInfo{
	GlobalInfo() {}

	//总行数
	int m_totalline = 0;
	//有效行数(非空行)
	int m_usefulline = 0;
	//总词数
	int m_totalchar = 0;
	//普通变量数量
	int m_noramlsymbol = 0;
	//函数数量
	int m_functionsymbol = 0;
	//类型(不包括typedef)数量
	int m_classsymbol = 0;
	//四个维度的分数(NORMATIVE/*规范性N*/  EFFICIENCY/*执行效率E*/, MAINTAINABILITY/*可维护性M*/, SECURITY/*安全性(代码风险)S*/)+总分()
	int m_scores[5] = { 0,0,0,0,0 };
};

//分析进度
struct AnalyzeStep {
	AnalyzeStep() {}

	//当前分析所在的步骤
	std::string m_stepName;
	//当前分析的进度
	int m_currentStep = 0;
	//当前分析的总进度
	int m_totalStep = 0;
};

//代码命名规范(目前只会发一个全局的信息)
struct CodeNamingInfo {
  CodeNamingInfo() {}
  CodeNamingInfo(int vt, std::map<int,int> namings) {
    for (auto ntamount : namings) {
      int nt = ntamount.first;
      int amount = ntamount.second;
      m_namingType.push_back(nt);
      m_namingAmount.push_back(amount);
      m_type = vt;
    }
  }
  
  //类型 0-var 1-function 2-record
  int m_type = 0;
  //命名类型
  std::vector<int> m_namingType;
  //该类型的数量
  std::vector<int> m_namingAmount;
};

//文件表
struct FileTable {
  FileTable(){};
  //所有文件名
  std::vector<std::string> m_files;
  //所有文件对应的ID
  std::vector<int> m_filesID;
};

//重构信息
#define CG_Modify 0
#define CG_Delete 1
#define CG_Add 2
struct CodeGenerateInfo {
	CodeGenerateInfo() {}
	CodeGenerateInfo(int gcType, int beginLine, int beginColum, int endLine, int endColum, std::string modifiedCode) {
		m_gcType = gcType;
		m_beginLine = beginLine;
		m_beginColum = beginColum;
		m_endLine = endLine;
		m_endColum = endColum;
		m_modifiedCode = modifiedCode;
	}
	//重构类型
	int m_gcType = -1;

	//如果是
	//开始行数/列数
	int m_beginLine = -1;
	int m_beginColum = -1;
	//结束行数/列数
	int m_endLine = -1;
	int m_endColum = -1;

	//更改后的代码(Modify时使用)
	std::string m_modifiedCode = "";

};

extern std::string BoostStringToStdString(const boost::json::string &s);

extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       IErrorInfo const &obj);
extern IErrorInfo tag_invoke(boost::json::value_to_tag<IErrorInfo>,
                             boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       IWarningInfo const &obj);
extern IWarningInfo tag_invoke(boost::json::value_to_tag<IWarningInfo>,
                               boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       GlobalInfo const &obj);
extern GlobalInfo tag_invoke(boost::json::value_to_tag<GlobalInfo>,
                             boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       AnalyzeStep const &obj);
extern AnalyzeStep tag_invoke(boost::json::value_to_tag<AnalyzeStep>,
                              boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       ClonePosition const &obj);
extern ClonePosition tag_invoke(boost::json::value_to_tag<ClonePosition>,
                              boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       CloneSetInfo const &obj);
extern CloneSetInfo tag_invoke(boost::json::value_to_tag<CloneSetInfo>,
                                boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       CodeNamingInfo const &obj);
extern CodeNamingInfo tag_invoke(boost::json::value_to_tag<CodeNamingInfo>,
                               boost::json::value const &jv);
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       FileTable const &obj);
extern FileTable tag_invoke(boost::json::value_to_tag<FileTable>,
                                 boost::json::value const &jv);
#endif // INFOSTUCT

/*
指令说明如下 -f test -nosimhash() -catchdiag -astopt -detailclone -clonemodel -cloneIP
"42.193.190.147" -cloneport "25566" -csetinfo [-nodecl -nodeclstmt -noliteral
-opnodediff -opnodetype -arrtype](这一坨要加上，对应O4优化) -detailfg(加上这个可以输出指纹信息到modelfg.txt) [-threshold 0.6](这是模型阈值,用0.6就挺好) 
*/