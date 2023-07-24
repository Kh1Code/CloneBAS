#pragma once
#include<string>
#include<bitset>
#include"MurmurHash3.h"
#include"SimHashConfig.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Execution.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"
#include "Log.h"
#include <clang/Frontend/CompilerInstance.h>
#include<vector>
#include"InfoStruct.h"
#define SEED 0x97c29b3a
#define AFLAG 233 //随便定一个数(不要小于节点类型数就行)，表明这个特征词的作用是一个标志
#define VEC_LEN 128

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

extern bool g_compareDiffFileFG; //是否只匹配不同文件之间的结点
extern bool g_compareSameFileFG; //是否只匹配相同文件之间的结点
extern SourceManager *g_currentsm;
extern ASTContext *g_currentASTContext;

//自定义升级版位置信息
class FixedLoc {
public:
  FixedLoc() {}
  void Init(unsigned l, unsigned c, const char *f) {
    m_line = l;
    m_colum = c;
    m_file = f;
  }
  void Init(clang::SourceManager *SM, clang::SourceLocation loc) {
    if (loc.isInvalid()) {
      return;
    }
    clang::PresumedLoc locInfo = SM->getPresumedLoc(loc);
    m_file = locInfo.getFilename();
    m_line = locInfo.getLine();
    m_colum = locInfo.getColumn();
  }
  int m_line = -1;
  int m_colum = -1;
  const char *m_file = nullptr;
};
//结点信息
class StmtNodeInfo {
public:
  StmtNodeInfo() {}
  void Init(const Stmt *node, SourceManager *SM);
  const Stmt *m_stmt;
  PresumedLoc m_beginlocInfo;
  PresumedLoc m_endlocInfo;
  bool m_useSM = false;
  int m_tokNums = 0;
};

//特征词
class EigenWord {
public:
  EigenWord(std::string &word, std::string &nodename) {
        this->m_word = word;
        this->m_weightVec.resize(VEC_LEN);
        this->SetWeight(nodename);
        this->SetWeightVec();
    }
    ~EigenWord() {
        std::string str;
        m_word.swap(str);
        m_weightVec.clear();
        m_weightVec.shrink_to_fit();
    }
    std::string m_word;
    int m_weight; //权重
    std::vector<int> m_weightVec; //权重向量

    void SetWeightVec(); //获取这个特征词的权重向量
    void SetWeight(std::string &nodename); //给这个特征词加权
  private:
    //获取特征词的哈希值,返回一个128位的bitset
    std::bitset<VEC_LEN> GetEigenWordHashBits();
};

struct FingerPrint {
    ~FingerPrint() {
        int size = m_fathernodes.size();   
        if (size > 0) {
            m_fathernodes.clear();
            m_fathernodes.shrink_to_fit();
        }
        size = m_comp_son_nodes.size();
        if (size > 0) {
            m_comp_son_nodes.clear();
            m_comp_son_nodes.shrink_to_fit();
        }
    }

    static enum class FP_Type
    {FP_CompoundStmt,FP_Function,FP_Class,FP_IfStmt,FP_SwitchStmt,
    FP_DoStmt,FP_WhileStmt,FP_ForStmt,FP_FixedCompStmt,FP_FixedSelectStmt,FP_FixedLoopStmt,
    FP_Optimize};


    int m_token_num; //指纹对应节点的token数"?
    FP_Type m_fptype;//指纹类型
    std::bitset<VEC_LEN> m_fingerprint;//指纹
    StmtNodeInfo m_nodeInfo;       //该指纹对应的结点
    std::vector<const void*> m_fathernodes;//该指纹父节点的指纹

    //该指纹紧挨着的复合语句子节点(因为if语句可能有很多else。所以用vector,把这些else都视为if的子节点)
    //对于其他语句，紧挨着的复合语句子节点就只有一个，即m_comp_son_nodes.size()=1
    std::vector<void*> m_comp_son_nodes; 

    bool to_be_cmp = true; //该指纹是否要被比较

};
extern bool g_displayFingerPrints;

//存放所有代码块各自的指纹
extern std::vector<FingerPrint*> g_allFingerPrints;

//存放所有特征词，用于回收内存
extern std::vector<EigenWord*> g_allEigenword;

//void ClearEigenWords(); //清空g_eigenWords中的特征词

FingerPrint* GetFingerPrint(std::vector<EigenWord*>& eigen_wordvec,FingerPrint::FP_Type fptype, int token_num);

void AddFingerPrint(FingerPrint*& fingerprient); //将算出的指纹加进g_fingerPrints

void AddEigenWord(std::string word, std::string &node_name,
                  std::vector<EigenWord *> &ewordvec); //新增特征词

void DeleteAllEigenWord();

void DeleteAllFingerPrint();






