#pragma once
#include "FixedNode.h"
#include "ScoreController.h"

extern DiagnosticsEngine *g_currentDiagEngine;

//分析文件遮罩(防止同一.h文件的AST部件被重复分析)
extern std::set<int> g_fileMask;
extern std::set<int> g_fileMaskCache; //分析文件缓存(缓存在一个.c/.cpp文件完成分析后刷新到m_fileMask)

class FixedFileManager {
public:
  void AddFile(std::string &file);
  int GetFiexdFileID(std::string &file);
  int GetOrAddFiexdFileID(std::string &file);

  std::unordered_map<std::string, int> m_fileMap;

private:
  int m_fileAmount = 0;
};
extern FixedFileManager g_fixedFileManager;

class FixedSimplifier {
public:
  /*
   * 化简函数
   */
  //递归化简,深层的先化简(只处理Stmt)
  int Simplify(const Stmt *&node);
  //化简组合语句结点
  int SimplifyCompoundStatement(const Stmt *&node);
  //化简普通语句
  int SimplifyStatement(const Stmt *&temp_node);
  //化简If语句
  FixedSelectionStatement *SimplifyIfStatement(const Stmt *&temp_node);
  //化简Switch语句
  FixedSelectionStatement *SimplifySwitchStatement(const Stmt *&temp_node);
  //化简for语句
  FixedLoopStatement *SimplifyForStatement(const Stmt *&temp_node);
  //化简while语句
  FixedLoopStatement *SimplifyWhileStatement(const Stmt *&temp_node);
  //深度化简选择结构
  int DeepSimplifySelection(SelectionUnit *selectunit,
                            FixedSelectionStatement *selectnode,
                            const Stmt *stmtnode);

  bool m_doASTopt = true;

private:
  //switch普通结点处理
  void HandleSwitchNoramlStmt(std::vector<SelectionUnit *> unit_waiting, const Stmt* stmt); 
  //创建运算式
  UnaryOperator *CreateUnaryExpr(UnaryOperatorKind unaryOp, const Expr *stmt);
  BinaryOperator *CreateBinaryExpr(BinaryOperatorKind binaryOp,
                                   const Expr *stmtl, const Expr *stmtr);
  //用来检查是否是loopstmt(非compstmt子结点时),如果是的话要做preinit前移和中间的fixedcompstmt生成
  const Stmt *CheckStmtLoopPreInitForward(const Stmt *temp_node);
};

//用于把clang的错误和分数向量结合
struct ScoreTracerUnit {
  ScoreTracerUnit(unsigned int line, unsigned int colum, int fid,
                  ScoreUnitVector *svec) { 
    m_line = line;
    m_colum = colum;
    m_fileID = fid;
    m_svector = svec;
  }
  unsigned int m_line = 0;
  unsigned int m_colum = 0;
  int m_fileID = -1;
  ScoreUnitVector *m_svector = nullptr; //为空说明为退出，否则为进入
  ScoreTracerUnit *m_next = nullptr;
};

class ScoreTracerList {
public:
  void TraceUnit(SourceLocation loc, ScoreUnitVector *svec) {
    ScoreTracerUnit *st = nullptr; 
    if (loc.isValid()) {
      PresumedLoc locInfo = g_currentsm->getPresumedLoc(loc);
      std::string fname = locInfo.getFilename();
      int id = g_fixedFileManager.GetOrAddFiexdFileID(fname);
      st = new ScoreTracerUnit(locInfo.getLine(), locInfo.getColumn(), id, svec);
    } else {
      if (m_current) {
        st = new ScoreTracerUnit(m_current->m_line, m_current->m_colum, m_current->m_fileID, svec);
      } else {
        st = new ScoreTracerUnit(1, 1, -1, svec);
      }
    }
    if (!m_head) {
      m_head = st;
    } else {
      m_current->m_next = st;
    }
    m_current = st;
  }
  void Clear() { 
    m_current = m_head;
    while (m_current) {
      ScoreTracerUnit *temp = m_current->m_next;
      delete m_current;
      m_current = temp;
    }
    m_head = nullptr;
    m_current = nullptr;
  }
  void ResetCurrent() { m_current = m_head; }
  void MoveCurrent(int line, int colum, int fid, bool toend) {
    if (!m_current) {
      return;
    }
    bool ahead = (fid != m_current->m_fileID || ((m_current->m_line < line) || (m_current->m_line == line && m_current->m_colum <= colum))) || toend;
    while (ahead) {
      if (m_current->m_svector) {
        //gInfoLog << "【Enter:["<< m_current->m_fname << " , " <<  m_current->m_line << " , " << m_current->m_colum << "]" << "】\n";
        ScoreController::g_scoreController.EnterScoreUnitVectorWithoutInit(m_current->m_svector);
      } else {
        //gInfoLog << "【Exit:["<< m_current->m_fname << " , " <<  m_current->m_line << " , " << m_current->m_colum << "]" << "】\n";
        ScoreController::g_scoreController.ExitScoreUnitVector();
      }
      m_current = m_current->m_next;
      if (m_current == nullptr) {
        break;      
      }
      ahead = (fid != m_current->m_fileID || ((m_current->m_line < line) || (m_current->m_line == line && m_current->m_colum <= colum))) || toend;
    }
  }
  ScoreTracerUnit *m_head = nullptr;
  ScoreTracerUnit *m_current = nullptr;
};

class DiagnosticWithLevel{
public:
  DiagnosticWithLevel(const Diagnostic &diag, DiagnosticsEngine::Level dlevel);
  DiagnosticsEngine::Level m_dlevel;
  int m_fileID;
  unsigned m_line;
  unsigned m_column;
  unsigned m_id;
  std::string m_display;
  std::string m_diagText;
  std::vector<std::string> m_placeHolders;
};

class FixedChecker {
public:
  //递归化简,深层的先化简
  int Check(const Decl *&node);
  int Check(const Stmt *&node);

  //检查圈复杂度
  int CheckConditionStmt(const Stmt *stmt);
  //检查函数大小和函数变量
  int CheckFunctionDecl(const FunctionDecl* funcDecl);

  unsigned AddOffsetsToTotal(const Decl *decl);

  //报错信息的重新填充
  void CatchDiagnostic(const Diagnostic &Info,
                       DiagnosticsEngine::Level &dlevel);
  void MatchDiagnosticCollection(ScoreUnitVector *globalST,
                                 std::string filename);

  ScoreTracerList m_socreTracerList;
  bool m_displayDiag = false;
  unsigned m_totalchar = 0;
  unsigned m_stmtdepth = 0; //此时的代码深度
  unsigned m_calldepth = 0; //此时的链式调用深度
  unsigned m_depthest_calldepth = 0; //此时的语句的链式最深调用深度(只有最深深度要报错)

private:
  bool TryEnterScoreUnit(const Decl *node);
  bool TryEnterScoreUnit(const Stmt *node);
  bool TryLeaveScoreUnit(const Decl *node, bool toleave);
  bool TryLeaveScoreUnit(const Stmt *node, bool toleave);
  void SendInDefineWarning(std::string str, SourceLocation beginLoc,
                           SourceLocation endloc, std::vector<std::string> placeHolders, int id);

  std::vector<DiagnosticWithLevel> m_diagnosticCollection;
  const Stmt *m_currentRootComp = nullptr;
};