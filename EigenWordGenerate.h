#include "EigenWord.h"
#include<vector>

extern std::vector<const Stmt *> g_fingerprintnodestack;

//所有配置参数
extern bool g_RecordExprType;  //记录expr的type
extern bool g_RecordDeclType;  //记录decl的type
extern bool g_RecordDeclName; //记录decl的名称
extern bool g_RecordDecls; //记录DeclStmt

//Stmt与Expr处理
extern void GenStmtEigenWord(const Stmt *S,
                             std::vector<EigenWord *> &eigen_wordvec,
                             int depth);
extern void GenCompoundStmtEigenWord(const Stmt *S,
                                  std::vector<EigenWord *> &eigen_wordvec,
                                  int depth);
extern void GenOtherStmtEigenWord(const Stmt *S,
                                  std::vector<EigenWord *> &eigen_wordvec,
                                  int depth);
extern void GenOtherExprEigenWord(const Expr *S,
                                  std::vector<EigenWord *> &eigen_wordvec,
                                  int depth);
extern void GenBinaryOperatorEigenWord(const BinaryOperator *S,
                                  std::vector<EigenWord *> &eigen_wordvec,
                                  int depth);
extern void GenUnaryOperatorEigenWord(const UnaryOperator *S,
                                  std::vector<EigenWord *> &eigen_wordvec,
                                  int depth);
extern void GenDeclRefExprEigenWord(const DeclRefExpr *S,
                                      std::vector<EigenWord *> &eigen_wordvec,
                                      int depth);
extern void GenDeclStmtEigenWord(const DeclStmt *S,
                                    std::vector<EigenWord *> &eigen_wordvec,
                                    int depth);

//Decl处理
extern void GenVarDeclEigenWord(const VarDecl *D,
                                 std::vector<EigenWord *> &eigen_wordvec,
                                 int depth);

//整个文件为基准生成指纹
extern void SavaFileEigenWords(std::vector<EigenWord *> &eigen_wordvec, std::string fname);

extern void GenAndCompareFingerPrints();

extern void GenCodeWordFile();

extern void GenCodeWordFileOnce();

