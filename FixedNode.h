#pragma once
#include"EigenWord.h"
#include"EigenWordGenerate.h"
#include"InfoStruct.h"

extern void TestNodeOutput(std::string str, int depth, std::string mark);
extern void DisplayStmt(const Stmt *S, int depth);

namespace clang {
class FixedParentStmt : public FixedStmt {
public:
  enum FixedType {
    FixedCompoundStmtClass,
    FixedSelectionStmtClass,
    FixedLoopStmtClass
  };
  std::string FixedTypeName[3]{
      "FixedCompoundStmtClass",
      "FixedSelectionStmtClass",
      "FixedLoopStmtClass",
  };
  SourceLocation getBeginLoc() const { return BeginLoc; }
  SourceLocation getEndLoc() const { return EndLoc; }
  virtual void Display (int depth) { 
      llvm::errs() << "���ﲻӦ�����е�\n"; 
  };
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec,
                            int depth) {
    llvm::errs() << "���ﲻӦ�����е�\n";
  };
  virtual void Display(int depth) const {
    llvm::errs() << "���ﲻӦ�ñ�����\n";
  }
  std::string GetFixedTypeName() { return FixedTypeName[m_fixedType]; }
  void InitBeiginLoc(SourceManager *SM, const Stmt *stmt) {
    if (const FixedStmt *fstmt = llvm::dyn_cast<FixedStmt>(stmt)) {
      BeginLoc = ((FixedParentStmt*)fstmt)->getBeginLoc();
    } else if (const DoStmt *fstmt = llvm::dyn_cast<DoStmt>(stmt)) {
      InitBeiginLoc(SM, fstmt->getBody());
    } else {
      BeginLoc = stmt->getBeginLoc();
    }
    m_startLoc.Init(SM, BeginLoc);
  }

  void InitEndLoc(SourceManager *SM, const Stmt *stmt) {
    if (const FixedStmt *fstmt = llvm::dyn_cast<FixedStmt>(stmt)) {
      EndLoc = ((FixedParentStmt*)fstmt)->getEndLoc();
    } else if (const IfStmt *fstmt = llvm::dyn_cast<IfStmt>(stmt)) {
      if (fstmt->hasElseStorage()) {
        InitEndLoc(SM, fstmt->getElse());
      } else {
        InitEndLoc(SM, fstmt->getThen());
      }
    } else if (const SwitchStmt *fstmt = llvm::dyn_cast<SwitchStmt>(stmt)) {
      InitEndLoc(SM, fstmt->getBody());
    } else if (const ForStmt *fstmt = llvm::dyn_cast<ForStmt>(stmt)) {
      InitEndLoc(SM, fstmt->getBody());
    } else if (const WhileStmt *fstmt = llvm::dyn_cast<WhileStmt>(stmt)) {
      InitEndLoc(SM, fstmt->getBody());
    } else {
      EndLoc = stmt->getEndLoc();
    }
    m_endLoc.Init(SM, EndLoc);
  }
  int GetCharNum() {
    unsigned beginOffset = g_currentsm->getFileOffset(BeginLoc);
    unsigned endOffset = g_currentsm->getFileOffset(EndLoc);
    return endOffset - beginOffset;
  }
  void SetGenFP(bool gen) { m_genFP = gen; }
  FixedLoc m_startLoc;
  FixedLoc m_endLoc;
  int m_tokenNums;
  FixedType m_fixedType;
protected:
  bool m_genFP = true; //�Ƿ�����ָ��,switch�е��ӽ����Բ�����ָ��
  SourceLocation BeginLoc;
  SourceLocation EndLoc;
};
//ͳһ�ṹ���CompoundStmt
class FixedCompoundStatement : public FixedParentStmt {
public:
  FixedCompoundStatement() : FixedParentStmt() {
    m_fixedType = FixedParentStmt::FixedCompoundStmtClass;
  };
  std::vector<const Stmt *> m_childnodeslist;

  virtual void Display(int depth);
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec, int depth);
};

//ѡ��Ԫ
struct SelectionUnit {
  ~SelectionUnit() {}
  const Expr *m_expr = NULL;
  const Stmt *m_stmt = NULL;
  bool m_independent = false;
  int m_boolresult = 0; // 0���� 1���� 2����
  int origin_index = 0; //��ԭ��stmt���е����
};

//�򻯺��ѡ��if/switch�����
class FixedSelectionStatement : public FixedParentStmt {
public:
  FixedSelectionStatement() : FixedParentStmt() {
    m_fixedType = FixedType::FixedSelectionStmtClass;
  }
  ~FixedSelectionStatement() {
    for (auto unit : m_selectionunits) {
      delete unit;
    }
    m_selectionunits.clear();
    m_selectionunits.shrink_to_fit();
  }
  std::vector<SelectionUnit *> m_selectionunits;

  virtual void Display(int depth);
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec, int depth);
};

//�򻯺��ѭ��(for/while)���
class FixedLoopStatement : public FixedParentStmt {
public:
  FixedLoopStatement() : FixedParentStmt() {
    m_fixedType = FixedType::FixedLoopStmtClass;
  }
  const Stmt *m_expr = NULL;
  const Stmt *m_stmt = NULL; //(��������FixedCompStmt)

  const Stmt *m_preInit = NULL;//��ʱ����(�������������ʵ�����,�ᱻ���͵��ϲ�)

  bool m_isdo = false; //�ǲ���do���(do���һ����ִ��һ��stmt)

  virtual void Display(int depth);
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec, int depth);
};
} // namespace clang