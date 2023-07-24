#include"EigenWordGenerate.h"
#include"FixedNode.h"

bool g_RecordExprType = false;  //记录expr的type
bool g_RecordDeclType = false;  //记录decl的type
bool g_RecordDeclName = false; //记录decl的名称
bool g_RecordDecls = false; //记录DeclStmt
bool g_PrintTestAST = false;

//存放需要生成签名的父节点栈（用于让指纹知道自己的父节点们）
std::vector<const Stmt*> g_fingerprintnodestack;

void OldTestNodeOutput(std::string str, int depth, std::string mark) {
  if (!g_PrintTestAST)
    return;
  for (int i = 0; i < depth; i++) {
    gInfoLog << mark;
  }
  gInfoLog << str << "\n";
  int *c;
}

void GenStmtEigenWord(const Stmt *S, std::vector<EigenWord *> &eigen_wordvec,
                      int depth) {
    //不同类型Stmt的判断
    //判断是不是组合结点语句
  FixedParentStmt *fixed_node = nullptr;
  const FixedStmt *fixed_com_node = llvm::dyn_cast<FixedStmt>(S);
    switch (S->getStmtClass()) { 
    case (Stmt::FixedStmtClass):
      fixed_node = (FixedParentStmt *)const_cast<FixedStmt *>(fixed_com_node);
      fixed_node->GetEigenWord(eigen_wordvec, depth + 1);
      break;
    case Stmt::CompoundStmtClass:
      //现在只允许CompoundStmt加入指纹
      GenCompoundStmtEigenWord(S, eigen_wordvec, depth);
      break;
    case Stmt::DeclStmtClass:
      if (const DeclStmt *DS = llvm::dyn_cast<DeclStmt>(S)) {
        GenDeclStmtEigenWord(DS, eigen_wordvec, depth);
      }
      break;
    case Stmt::BinaryOperatorClass:
      if (const BinaryOperator *expr = llvm::dyn_cast<BinaryOperator>(S)) {
        GenBinaryOperatorEigenWord(expr, eigen_wordvec, depth);
      }   
      break;
    case Stmt::UnaryOperatorClass:
      if (const UnaryOperator *expr = llvm::dyn_cast<UnaryOperator>(S)) {
        GenUnaryOperatorEigenWord(expr, eigen_wordvec, depth);
      }
      break;
    case Stmt::DeclRefExprClass:
      if (const DeclRefExpr *expr = llvm::dyn_cast<DeclRefExpr>(S)) {
        GenDeclRefExprEigenWord(expr, eigen_wordvec, depth);
      }
      break;
    default:
      const Expr *expr = llvm::dyn_cast<Expr>(S);
      if (expr) {
        GenOtherExprEigenWord(expr, eigen_wordvec, depth);
      }
      else {
        GenOtherStmtEigenWord(S, eigen_wordvec, depth);
      }   
      break;
  }
}

void GenCompoundStmtEigenWord(const Stmt *S,
                              std::vector<EigenWord *> &eigen_wordvec,
                              int depth) {
  g_fingerprintnodestack.push_back(S);
  //将自己的结点类型加入特征词
  std::vector<EigenWord *> new_words;
  new_words.reserve(16);
  std::string str(S->getStmtClassName());
  AddEigenWord(str, str, new_words);
  //临时测试输出
  OldTestNodeOutput(str + "<fingerprint>", depth, "--");
  //获得所有子结点
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtEigenWord(*I, new_words, depth + 1);
    }
  }
  g_fingerprintnodestack.pop_back();
  //判断是否要生成指纹
  SourceLocation beginLoc = S->getBeginLoc();
  SourceLocation endLoc = S->getEndLoc();
  unsigned beginOffset = g_currentsm->getFileOffset(beginLoc);
  unsigned endOffset = g_currentsm->getFileOffset(endLoc);
  bool genfg = (endOffset - beginOffset) > COMPSTMT_TOKEN_NUM_BOUNDARY;
  if (genfg) {
    //组合语句块定义指纹，初始化数据
    FingerPrint *fingerprint =
        GetFingerPrint(new_words, FingerPrint::FP_Type::FP_FixedCompStmt,
                       (endOffset - beginOffset));
    fingerprint->m_nodeInfo.Init(S, g_currentsm);
    for (const Stmt *node : g_fingerprintnodestack) {
      fingerprint->m_fathernodes.push_back(node);
    }
    AddFingerPrint(fingerprint);
  }
  //置入父节点特征词
  eigen_wordvec.insert(eigen_wordvec.end(), new_words.begin(),
                       new_words.end());
}

void GenOtherStmtEigenWord(const Stmt *S,
                           std::vector<EigenWord *> &eigen_wordvec,
                      int depth) {
  //将自己的结点类型加入特征词
  std::string str(S->getStmtClassName());
  AddEigenWord(str, str, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "--");
  //获得所有子结点
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtEigenWord(*I, eigen_wordvec, depth + 1);
    }
  }
}

void GenOtherExprEigenWord(const Expr *S,
                           std::vector<EigenWord *> &eigen_wordvec, int depth) {
  //将自己的结点类型加入特征词
  std::string str(S->getStmtClassName());
  std::string stmtname = str;
  if (g_RecordExprType) {
    std::string add = S->getType().getAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add; 
  }
  AddEigenWord(str, stmtname, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "==");
  //获得所有子结点
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtEigenWord(*I, eigen_wordvec, depth + 1);
    }
  }
}

void GenBinaryOperatorEigenWord(const BinaryOperator *S,
                                std::vector<EigenWord *> &eigen_wordvec,
                                int depth) {
  //将自己的结点类型加入特征词
  std::string str(S->getStmtClassName());
  std::string stmtname = str;
  std::string add = S->getOpcodeStr().str();
  add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
  str += " BO_" + add; 
  if (g_RecordExprType) {
    std::string add = S->getType().getAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add; 
  }
  AddEigenWord(str, stmtname, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "==");
  //获得所有子结点
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtEigenWord(*I, eigen_wordvec, depth + 1);
    }
  }
}

void GenUnaryOperatorEigenWord(const UnaryOperator *S,
                               std::vector<EigenWord *> &eigen_wordvec,
                               int depth) {
  //将自己的结点类型加入特征词
  std::string str(S->getStmtClassName());
  std::string stmtname = str;
  std::string add = S->getOpcodeStr(S->getOpcode()).str();
  add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
  str += " UO_" + add;
  if (g_RecordExprType) {
    std::string add = S->getType().getAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add;
  }
  AddEigenWord(str, stmtname, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "==");
  //获得所有子结点
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtEigenWord(*I, eigen_wordvec, depth + 1);
    }
  }
}

void GenDeclRefExprEigenWord(const DeclRefExpr *S,
                             std::vector<EigenWord *> &eigen_wordvec,
                             int depth) {
  //将自己的结点类型加入特征词
  std::string str(S->getStmtClassName());
  std::string stmtname = str;
  if (g_RecordDeclName) {
    std::string add = S->getFoundDecl()->getNameAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add;
  }
  if (g_RecordExprType) {
    std::string add = S->getType().getAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add;
  }
  AddEigenWord(str, stmtname, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "==");
  //获得所有子结点
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtEigenWord(*I, eigen_wordvec, depth + 1);
    }
  }
}

void GenDeclStmtEigenWord(const DeclStmt *S,
                          std::vector<EigenWord *> &eigen_wordvec, int depth) {
  if (!g_RecordDecls)
    return;
  //将自己的结点类型加入特征词
  std::string str(S->getStmtClassName());
  std::string stmtname = str;
  AddEigenWord(str, stmtname, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "--");
  //获得所有子结点
  for (Decl *decl : S->getDeclGroup()) {
    VarDecl *vd = dyn_cast<VarDecl>(decl);
    if (vd) {
      GenVarDeclEigenWord(vd, eigen_wordvec, depth + 1);
    }
  }
}

void GenVarDeclEigenWord(const VarDecl *D,
                         std::vector<EigenWord *> &eigen_wordvec, int depth) {
  //将自己的结点类型加入特征词
  std::string str(D->getDeclKindName());
  std::string stmtname = str;
  if (g_RecordDeclName) {
    std::string add = D->getDeclName().getAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add;
  }
  if (g_RecordExprType) {
    std::string add = D->getType().getAsString();
    add.erase(remove_if(add.begin(), add.end(), isspace), add.end());
    str += " " + add;
  }
  AddEigenWord(str, stmtname, eigen_wordvec);
  //临时测试输出
  OldTestNodeOutput(str, depth, "++");
}

#include<unordered_map>
std::unordered_map<std::string, std::vector<EigenWord *>*> g_fileEigenWords;

int GetHammingDis(std::bitset<VEC_LEN> &bs1, std::bitset<VEC_LEN> &bs2) {
  return ((bs1) ^ (bs2)).count();
}


void SavaFileEigenWords(std::vector<EigenWord *> &eigen_wordvec,
                        std::string fname) {
  gInfoLog << "SavaFileEigenWords: " << fname << "\n";
  if (g_fileEigenWords.find(fname) == g_fileEigenWords.end()) {
    std::vector<EigenWord *> *eigenword =
        new std::vector<EigenWord *>(eigen_wordvec);
    g_fileEigenWords.emplace(fname, eigenword);
  }
  else {
    std::vector<EigenWord *> *eigenword = g_fileEigenWords.find(fname)->second;
    eigenword->insert(eigenword->end(), eigen_wordvec.begin(),
                      eigen_wordvec.end());
  }
}

void GenAndCompareFingerPrints() { 
    DeleteAllFingerPrint(); 
    std::vector<std::string> files;
    std::vector<FingerPrint *> fingerprints;
    int index = 0;
    for (auto iter = g_fileEigenWords.begin(); iter != g_fileEigenWords.end();
         ++iter) {
      FingerPrint *fingerprint = GetFingerPrint(
          *iter->second, FingerPrint::FP_Type::FP_FixedCompStmt, 100);
      files.push_back(iter->first);
      fingerprints.push_back(fingerprint);
      for (int i = 0; i < index; i++) {
        FingerPrint *fingerprint2 = fingerprints.at(i);
        int distance = GetHammingDis(fingerprint->m_fingerprint,
                                     fingerprint2->m_fingerprint);
      std::string fname2 = files.at(i);
      std::ofstream file("result.txt", std::ios::app);
        if (file.is_open()) {
          gInfoLog << fname2 << "|" << iter->first << "|" << distance
                       << "\n";
          file << fname2 << "|" << iter->first << "|" << distance << "\n";
          file.close();
        }
      }
      index++;
    }
}

void GenCodeWordFile() {
  DeleteAllFingerPrint();
  std::vector<std::string> files;
  std::vector<FingerPrint *> fingerprints;
  int index = 0;
  for (auto iter = g_fileEigenWords.begin(); iter != g_fileEigenWords.end();
       ++iter){
    std::string outname = iter->first;
    outname.replace(outname.end() - 3, outname.end(), "txt");
    std::ofstream file(outname);
    for (EigenWord *eword : *iter->second) {
      file << eword->m_word << " ";
    }
    file.close();
  }
}


void GenCodeWordFileOnce() {
  DeleteAllFingerPrint();
  std::vector<std::string> files;
  std::vector<FingerPrint *> fingerprints;
  int index = 0;
  std::ofstream file("result.txt");
  for (auto iter = g_fileEigenWords.begin(); iter != g_fileEigenWords.end();
       ++iter) {
    for (EigenWord *eword : *iter->second) {
      file << eword->m_word << " ";
    }
    file << "\n";
  }
}