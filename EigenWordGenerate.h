#include "EigenWord.h"
#include<vector>

extern std::vector<const Stmt *> g_fingerprintnodestack;

//�������ò���
extern bool g_RecordExprType;  //��¼expr��type
extern bool g_RecordDeclType;  //��¼decl��type
extern bool g_RecordDeclName; //��¼decl������
extern bool g_RecordDecls; //��¼DeclStmt

//Stmt��Expr����
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

//Decl����
extern void GenVarDeclEigenWord(const VarDecl *D,
                                 std::vector<EigenWord *> &eigen_wordvec,
                                 int depth);

//�����ļ�Ϊ��׼����ָ��
extern void SavaFileEigenWords(std::vector<EigenWord *> &eigen_wordvec, std::string fname);

extern void GenAndCompareFingerPrints();

extern void GenCodeWordFile();

extern void GenCodeWordFileOnce();

