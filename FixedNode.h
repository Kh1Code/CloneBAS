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
      llvm::errs() << "这里不应该运行到\n"; 
  };
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec,
                            int depth) {
    llvm::errs() << "这里不应该运行到\n";
  };
  virtual void Display(int depth) const {
    llvm::errs() << "这里不应该被调用\n";
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
  bool m_genFP = true; //是否生成指纹,switch中的子结点可以不生成指纹
  SourceLocation BeginLoc;
  SourceLocation EndLoc;
};
//统一结构后的CompoundStmt
class FixedCompoundStatement : public FixedParentStmt {
public:
  FixedCompoundStatement() : FixedParentStmt() {
    m_fixedType = FixedParentStmt::FixedCompoundStmtClass;
  };
  std::vector<const Stmt *> m_childnodeslist;

  virtual void Display(int depth);
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec, int depth);
};

//选择单元
struct SelectionUnit {
  ~SelectionUnit() {}
  const Expr *m_expr = NULL;
  const Stmt *m_stmt = NULL;
  bool m_independent = false;
  int m_boolresult = 0; // 0正常 1永真 2永假
  int origin_index = 0; //在原先stmt块中的序号
};

//简化后的选择（if/switch）结点
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

//简化后的循环(for/while)结点
class FixedLoopStatement : public FixedParentStmt {
public:
  FixedLoopStatement() : FixedParentStmt() {
    m_fixedType = FixedType::FixedLoopStmtClass;
  }
  const Stmt *m_expr = NULL;
  const Stmt *m_stmt = NULL; //(理论上是FixedCompStmt)

  const Stmt *m_preInit = NULL;//临时储存(并不参与特征词的生成,会被移送到上层)

  bool m_isdo = false; //是不是do语句(do语句一定会执行一次stmt)

  virtual void Display(int depth);
  virtual void GetEigenWord(std::vector<EigenWord *> &eigen_wordvec, int depth);
};
} // namespace clang