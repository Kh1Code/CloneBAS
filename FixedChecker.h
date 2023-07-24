#pragma once
#include "FixedNode.h"
#include "ScoreController.h"

extern DiagnosticsEngine *g_currentDiagEngine;

//�����ļ�����(��ֹͬһ.h�ļ���AST�������ظ�����)
extern std::set<int> g_fileMask;
extern std::set<int> g_fileMaskCache; //�����ļ�����(������һ��.c/.cpp�ļ���ɷ�����ˢ�µ�m_fileMask)

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
   * ������
   */
  //�ݹ黯��,�����Ȼ���(ֻ����Stmt)
  int Simplify(const Stmt *&node);
  //������������
  int SimplifyCompoundStatement(const Stmt *&node);
  //������ͨ���
  int SimplifyStatement(const Stmt *&temp_node);
  //����If���
  FixedSelectionStatement *SimplifyIfStatement(const Stmt *&temp_node);
  //����Switch���
  FixedSelectionStatement *SimplifySwitchStatement(const Stmt *&temp_node);
  //����for���
  FixedLoopStatement *SimplifyForStatement(const Stmt *&temp_node);
  //����while���
  FixedLoopStatement *SimplifyWhileStatement(const Stmt *&temp_node);
  //��Ȼ���ѡ��ṹ
  int DeepSimplifySelection(SelectionUnit *selectunit,
                            FixedSelectionStatement *selectnode,
                            const Stmt *stmtnode);

  bool m_doASTopt = true;

private:
  //switch��ͨ��㴦��
  void HandleSwitchNoramlStmt(std::vector<SelectionUnit *> unit_waiting, const Stmt* stmt); 
  //��������ʽ
  UnaryOperator *CreateUnaryExpr(UnaryOperatorKind unaryOp, const Expr *stmt);
  BinaryOperator *CreateBinaryExpr(BinaryOperatorKind binaryOp,
                                   const Expr *stmtl, const Expr *stmtr);
  //��������Ƿ���loopstmt(��compstmt�ӽ��ʱ),����ǵĻ�Ҫ��preinitǰ�ƺ��м��fixedcompstmt����
  const Stmt *CheckStmtLoopPreInitForward(const Stmt *temp_node);
};

//���ڰ�clang�Ĵ���ͷ����������
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
  ScoreUnitVector *m_svector = nullptr; //Ϊ��˵��Ϊ�˳�������Ϊ����
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
        //gInfoLog << "��Enter:["<< m_current->m_fname << " , " <<  m_current->m_line << " , " << m_current->m_colum << "]" << "��\n";
        ScoreController::g_scoreController.EnterScoreUnitVectorWithoutInit(m_current->m_svector);
      } else {
        //gInfoLog << "��Exit:["<< m_current->m_fname << " , " <<  m_current->m_line << " , " << m_current->m_colum << "]" << "��\n";
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
  //�ݹ黯��,�����Ȼ���
  int Check(const Decl *&node);
  int Check(const Stmt *&node);

  //���Ȧ���Ӷ�
  int CheckConditionStmt(const Stmt *stmt);
  //��麯����С�ͺ�������
  int CheckFunctionDecl(const FunctionDecl* funcDecl);

  unsigned AddOffsetsToTotal(const Decl *decl);

  //������Ϣ���������
  void CatchDiagnostic(const Diagnostic &Info,
                       DiagnosticsEngine::Level &dlevel);
  void MatchDiagnosticCollection(ScoreUnitVector *globalST,
                                 std::string filename);

  ScoreTracerList m_socreTracerList;
  bool m_displayDiag = false;
  unsigned m_totalchar = 0;
  unsigned m_stmtdepth = 0; //��ʱ�Ĵ������
  unsigned m_calldepth = 0; //��ʱ����ʽ�������
  unsigned m_depthest_calldepth = 0; //��ʱ��������ʽ����������(ֻ���������Ҫ����)

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