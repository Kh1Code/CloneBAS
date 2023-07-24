#include"FixedNode.h"

void TestNodeOutput(std::string str, int depth, std::string mark) {
  for (int i = 0; i < depth; i++) {
    gInfoLog << mark;
  }
  gInfoLog << str << "\n";
  int *c;
}

void DisplayStmt(const Stmt *S, int depth) {
  if (S == nullptr)
    return;
  if (const FixedStmt *stmt = llvm::dyn_cast<FixedStmt>(S)) {
    FixedParentStmt * p =((FixedParentStmt *)stmt);
    p->Display(depth);
    return;
  }
  TestNodeOutput(S->getStmtClassName(), depth, "--");
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      DisplayStmt(*I, depth + 1);
    }
  }
}

void FixedCompoundStatement::Display(int depth) {
  TestNodeOutput("FixedCompoundStatement", depth, "==");
  for (const Stmt *node : m_childnodeslist) {
    if (!node) {
      continue;
    }
    DisplayStmt(node, depth + 1);
  }
}

void FixedCompoundStatement::GetEigenWord (
  std::vector<EigenWord *> &eigen_wordvec, int depth) {
  g_fingerprintnodestack.push_back(this);
  //�������_��Ҫ����ָ��
  std::vector<EigenWord *> new_words;
  std::string name = GetFixedTypeName();
  AddEigenWord(name, name, new_words);
  new_words.reserve(16);
  for (const Stmt *node : m_childnodeslist) {
    if (!node) {
      break;
    }
    GenStmtEigenWord(node, new_words, depth + 1);
  }
  g_fingerprintnodestack.pop_back();

  //token������ָ����Ŀ�����ָ��
  unsigned beginOffset = g_currentsm->getFileOffset(BeginLoc);
  unsigned endOffset = g_currentsm->getFileOffset(EndLoc);
  this->m_tokenNums = endOffset - beginOffset;
  if (this->m_tokenNums > FIXEDCOMPSTMT_TOKEN_NUM_BOUNDARY && m_genFP) {
    //������鶨��ָ�ƣ���ʼ������
    FingerPrint *fingerprint =
        GetFingerPrint(new_words, FingerPrint::FP_Type::FP_FixedCompStmt, this->m_tokenNums);
    fingerprint->m_nodeInfo.Init(this, g_currentsm);
    for (const Stmt *node : g_fingerprintnodestack) {
      fingerprint->m_fathernodes.push_back(node);
    }
    AddFingerPrint(fingerprint);
  }
  eigen_wordvec.insert(eigen_wordvec.end(), new_words.begin(),
                       new_words.end()); //���븸�ڵ�������
}

void FixedSelectionStatement::Display(int depth) {
  TestNodeOutput("FixedSelectionStatement", depth, "==");
  for (SelectionUnit *suint : m_selectionunits) {
    TestNodeOutput("SelectionUnit", depth + 1, "++");
    DisplayStmt(suint->m_expr, depth+2);
    DisplayStmt(suint->m_stmt, depth+2);
  }
}

void FixedSelectionStatement::GetEigenWord(
    std::vector<EigenWord *> &eigen_wordvec, int depth) {
  g_fingerprintnodestack.push_back(this);
  //ѡ������,��Ҫ����ָ��
  std::vector<EigenWord *> new_words;
  std::string name = GetFixedTypeName();
  AddEigenWord(name, name, new_words);

  for (SelectionUnit *suint : m_selectionunits) {
    GenStmtEigenWord(suint->m_expr, new_words, depth + 1);
    GenStmtEigenWord(suint->m_stmt, new_words, depth + 1);
  }
  g_fingerprintnodestack.pop_back();
  
  unsigned beginOffset = g_currentsm->getFileOffset(BeginLoc);
  unsigned endOffset = g_currentsm->getFileOffset(EndLoc);
  this->m_tokenNums = endOffset - beginOffset;
  //������Ϊһ�����,�����ǲ𿪿��ܸ�����һЩ��
  if (this->m_tokenNums > FIXEDSELECTSTMT_TOKEN_NUM_BOUNDARY && m_genFP) {
    //������鶨��ָ�ƣ���ʼ������
    FingerPrint *fingerprint = GetFingerPrint(
        new_words, FingerPrint::FP_Type::FP_FixedCompStmt, this->m_tokenNums);
    fingerprint->m_nodeInfo.Init(this, g_currentsm);
    for (const Stmt *node : g_fingerprintnodestack) {
      fingerprint->m_fathernodes.push_back(node);
    }
    AddFingerPrint(fingerprint);
  }
  
  eigen_wordvec.insert(eigen_wordvec.end(), new_words.begin(),
                       new_words.end()); //���븸�ڵ�������
}

void FixedLoopStatement::Display(int depth) {
  TestNodeOutput("FixedLoopStatement", depth, "==");
  if (m_expr) {
    DisplayStmt(m_expr, depth + 1);
  }
  if (m_stmt) {
    DisplayStmt(m_stmt, depth + 1);
  }
}

void FixedLoopStatement::GetEigenWord(std::vector<EigenWord *> &eigen_wordvec,
                                      int depth) {
  g_fingerprintnodestack.push_back(this);

  std::vector<EigenWord *> new_words;
  std::string name = GetFixedTypeName();
  AddEigenWord(name, name, new_words);

  if (m_expr) {
    GenStmtEigenWord(m_expr, new_words, depth + 1);
  }
  if (m_stmt) {
    GenStmtEigenWord(m_stmt, new_words, depth + 1);
  }
  g_fingerprintnodestack.pop_back();

  //������Ϊһ�����,�����ǲ𿪿��ܸ�����һЩ��
  unsigned beginOffset = g_currentsm->getFileOffset(BeginLoc);
  unsigned endOffset = g_currentsm->getFileOffset(EndLoc);
  this->m_tokenNums = endOffset - beginOffset;
  if (this->m_tokenNums > FIXEDLOOPSTMT_TOKEN_NUM_BOUNDARY && m_genFP) {
    //������鶨��ָ�ƣ���ʼ������
    FingerPrint *fingerprint =
        GetFingerPrint(new_words, FingerPrint::FP_Type::FP_FixedCompStmt, this->m_tokenNums);
    fingerprint->m_nodeInfo.Init(this, g_currentsm);
    for (const Stmt *node : g_fingerprintnodestack) {
      fingerprint->m_fathernodes.push_back(node);
    }
    AddFingerPrint(fingerprint);
  }

  eigen_wordvec.insert(eigen_wordvec.end(), new_words.begin(),
                       new_words.end()); //���븸�ڵ�������
}