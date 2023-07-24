#include"NodeTravesal.h"
#include"FixedNode.h"
bool g_opNodeDiff = false; 
bool g_opNodeType = false; 
bool g_noLiteral = false; 
bool g_noDecl = false;
bool g_noDeclStmt = false;
bool g_noArraySExpr = false;
bool g_arrType = false;

std::string GetNodeName(const Stmt *root) {
  if (root->getStmtClass() == Stmt::ImplicitCastExprClass ||
      root->getStmtClass() == Stmt::RecoveryExprClass ||
      root->getStmtClass() == Stmt::UnresolvedLookupExprClass ||
      root->getStmtClass() == Stmt::DeclRefExprClass) {
    return "";
  }
  if (g_noArraySExpr) {
    if (root->getStmtClass() == Stmt::ArraySubscriptExprClass) {
      return "";
    }
  }
  if (g_noLiteral) {
    //忽略常见的Literal
    if (root->getStmtClass() == Stmt::IntegerLiteralClass || 
        root->getStmtClass() == Stmt::CharacterLiteralClass ||
        root->getStmtClass() == Stmt::StringLiteralClass ||
        root->getStmtClass() == Stmt::FloatingLiteralClass) {
      return "";
    }
  }
  if (root->getStmtClass() == Stmt::DeclStmtClass) {
    if (g_noDeclStmt) {
      //忽略DeclStmt
      return "";
    } else {
      const DeclStmt *declstmt = llvm::dyn_cast<DeclStmt>(root);
      if (declstmt->getDeclGroup().isSingleDecl()) {
        return "DeclStmt_Single";
      } else {
        return "DeclStmt_Group";
      }
    }
  }
  if (root->getStmtClass() == Stmt::FixedStmtClass) {
    FixedParentStmt *fstmt = (FixedParentStmt *)root;
    //忽略只有有1个或0个子块的fixedcompstmt
    if (fstmt->m_fixedType == FixedParentStmt::FixedCompoundStmtClass) {
      FixedCompoundStatement *fcompstmt = (FixedCompoundStatement *)fstmt;
      if (fcompstmt->m_childnodeslist.size() <= 1) {
        return "";
      }
    }
    return fstmt->GetFixedTypeName();
  }
  std::string sname = root->getStmtClassName();
  if (g_opNodeDiff) {
    // BO和UO输出的时候要带上OPCode
    if (root->getStmtClass() == Stmt::BinaryOperatorClass) {
      const BinaryOperator *BOp = llvm::dyn_cast<BinaryOperator>(root);
      sname += "_" + BOp->getOpcodeStr().str();
    } else if (root->getStmtClass() == Stmt::UnaryOperatorClass) {
      const UnaryOperator *UOp = llvm::dyn_cast<UnaryOperator>(root);
      sname += "_" + UOp->getOpcodeStr(UOp->getOpcode()).str();
    }
  }
  if (g_arrType) {
    //ArraySubscriptExpr要加上类型
    if (root->getStmtClass() == Stmt::ArraySubscriptExprClass) {
      const ArraySubscriptExpr *arrexpr = llvm::dyn_cast<ArraySubscriptExpr>(root);
      auto type = arrexpr->getType().getTypePtr();
      if (type) {
        std::string t = type->getTypeClassName();
        if (type->getTypeClass() == Type::Builtin) {
          const BuiltinType *builtinType = dyn_cast<BuiltinType>(type);
          if (builtinType->isBooleanType()) {
            sname += "_Bool";
          }else if (builtinType->isInteger()) {
            sname += "_Integer";
          } else if (builtinType->isFloatingPoint()) {
            sname += "_FloatingPoint";
          } else {
            sname += "_" + t;
          }
        } else {
          sname += "_" + t;
        }
      }
    }
  }
  if (g_opNodeType) {
    // BO和UO输出的时候要带上大致的类型
    if (root->getStmtClass() == Stmt::BinaryOperatorClass) {
      const BinaryOperator *BOp = llvm::dyn_cast<BinaryOperator>(root);
      auto type = BOp->getType().getTypePtr();
      if (type) {
        std::string t = type->getTypeClassName();
        if (type->getTypeClass() == Type::Builtin) {
          const BuiltinType *builtinType = dyn_cast<BuiltinType>(type);
          if (builtinType->isBooleanType()) {
            sname += "_Bool";
          }else if (builtinType->isInteger()) {
            sname += "_Integer";
          } else if (builtinType->isFloatingPoint()) {
            sname += "_FloatingPoint";
          } else {
            sname += "_" + t;
          }
        } else {
          sname += "_" + t;
        }
      }
    } else if (root->getStmtClass() == Stmt::UnaryOperatorClass) {
      const UnaryOperator *UOp = llvm::dyn_cast<UnaryOperator>(root);
      auto type = UOp->getType().getTypePtr();
      if (type) {
        std::string t = type->getTypeClassName();
        if (type->getTypeClass() == Type::Builtin) {
          const BuiltinType *builtinType = dyn_cast<BuiltinType>(type);
          if (builtinType->isBooleanType()) {
            sname += "_Bool";
          } else if (builtinType->isInteger()) {
            sname += "_Integer";
          } else if (builtinType->isFloatingPoint()) {
            sname += "_FloatingPoint";
          } else {
            sname += "_" + t;
          }
        }
        else {
          sname += "_" + t;
        }

      }
    }
  }
  //输出默认名称
  return sname;
}

std::string GetNodeName(const Decl *decl) {
  return std::string(decl->getDeclKindName()) + "Decl";
}


void TravesalChildren(const Stmt *root, std::string &output, int type) {
  if (root->getStmtClass() == Stmt::FixedStmtClass) {
    FixedParentStmt *fstmt = (FixedParentStmt *)root;
    if (fstmt->m_fixedType == FixedParentStmt::FixedCompoundStmtClass) {
      FixedCompoundStatement *cstmt = (FixedCompoundStatement *)fstmt;
      for (auto cs : cstmt->m_childnodeslist) {
        TravesalAST(cs, output, type);
      }
    }
    else if (fstmt->m_fixedType == FixedParentStmt::FixedSelectionStmtClass) {
      FixedSelectionStatement *cstmt = (FixedSelectionStatement *)fstmt;
      for (auto cs : cstmt->m_selectionunits) {
        TravesalAST(cs->m_expr, output, type);
        TravesalAST(cs->m_stmt, output, type);
      }
    }
    else if (fstmt->m_fixedType == FixedParentStmt::FixedLoopStmtClass) {
      FixedLoopStatement *cstmt = (FixedLoopStatement *)fstmt;
      TravesalAST(cstmt->m_expr, output, type);
      TravesalAST(cstmt->m_stmt, output, type);
    }
  } else {
    if (!g_noDecl) {
      if (const DeclStmt *dstmt = llvm::dyn_cast<DeclStmt>(root)) {
        for (const Decl *decl : dstmt->getDeclGroup()) {
          TravesalAST(decl, output, type);
        }
      }
    }
    for (Stmt::const_child_iterator I = root->child_begin();
         I != root->child_end(); ++I) {
      if (*I != nullptr) {
        TravesalAST(*I, output, type);
      }
    }
  }
}

bool is_only_spaces(const std::string &str) {
  return std::all_of(str.begin(), str.end(), [](char c) {
    return std::isspace(static_cast<unsigned char>(c));
  });
}

bool is_empty_or_space(const std::string &str) {
  return str.empty() || is_only_spaces(str);
}

// 0前序 1中序(多叉树咋中序) 2后序
void TravesalAST(const Stmt* root, std::string& output, int type) {
  if (!root) {
    return;
  }
  if (root->getStmtClass() == Stmt::RecoveryExprClass) {
    return;
  }
  if (type == 0) {
    //前序
    output += GetNodeName(root) + " ";
    TravesalChildren(root, output, type);
  }
  else if (type == 1) {
    //中序
  } 
  else if (type == 2) {
    //后序
    TravesalChildren(root, output, type);
    output += GetNodeName(root) + " ";
  } else if (type == 3) {
    //树结构
    std::string noden = GetNodeName(root);
    if (noden.empty()) {
      std::string str;
      TravesalChildren(root, str, type);
      if (!is_empty_or_space(str)) {
        output += str + " ";
      }
    } else {
      output += noden + " ";
      std::string str;
      TravesalChildren(root, str, type);
      if (!is_empty_or_space(str)) {
        output += " [ " + str + " ] ";
      }
    }
  }
}

void TravesalAST(const Decl *root, std::string &output, int type) {
  if (!root) {
    return;
  }
  output += GetNodeName(root) + " ";
}


void CountChildren(const Stmt *root, std::map<std::string, int> &result) {
  if (root->getStmtClass() == Stmt::FixedStmtClass) {
    FixedParentStmt *fstmt = (FixedParentStmt *)root;
    if (fstmt->m_fixedType == FixedParentStmt::FixedCompoundStmtClass) {
      FixedCompoundStatement *cstmt = (FixedCompoundStatement *)fstmt;
      for (auto cs : cstmt->m_childnodeslist) {
        CountASTNode(cs, result);
      }
    } else if (fstmt->m_fixedType == FixedParentStmt::FixedSelectionStmtClass) {
      FixedSelectionStatement *cstmt = (FixedSelectionStatement *)fstmt;
      for (auto cs : cstmt->m_selectionunits) {
        CountASTNode(cs->m_expr, result);
        CountASTNode(cs->m_stmt, result);
      }
    } else if (fstmt->m_fixedType == FixedParentStmt::FixedLoopStmtClass) {
      FixedLoopStatement *cstmt = (FixedLoopStatement *)fstmt;
      CountASTNode(cstmt->m_expr, result);
      CountASTNode(cstmt->m_stmt, result);
    }
  } else {
    if (!g_noDecl) {
      if (const DeclStmt *dstmt = llvm::dyn_cast<DeclStmt>(root)) {
        for (const Decl *decl : dstmt->getDeclGroup()) {
          CountASTNode(decl, result);
        }
      }
    }
    for (Stmt::const_child_iterator I = root->child_begin();
         I != root->child_end(); ++I) {
      if (*I != nullptr) {
        CountASTNode(*I, result);
      }
    }
  }
}

void CountASTNode(const Stmt *root, std::map<std::string, int> &result) {
  if (!root)
    return;
  if (root->getStmtClass() == Stmt::RecoveryExprClass) {
    return;
  }
  std::string str = GetNodeName(root);
  if (str.empty())
    return;
  auto iter = result.find(str);
  if (iter == result.end()) {
    result.emplace(str, 1);
  }
  else {
    iter->second++;
  }
  //子节点递归
  CountChildren(root, result);
}

void CountASTNode(const Decl *root, std::map<std::string, int> &result) {
  if (!root)
    return;
  std::string str = GetNodeName(root);
  if (str.empty())
    return;
  auto iter = result.find(str);
  if (iter == result.end()) {
    result.emplace(str, 1);
  } else {
    iter->second++;
  }
}

void CountASTOutput(std::map<std::string, int> &result) {
  std::ofstream file("count.txt");
  unsigned long long total = 0;
  for (auto iter : result) {
    file << iter.first << " " << iter.second << std::endl;
    total += iter.second;
  }
  file.close();
  std::ofstream filet("count_total.txt");
  filet << total << std::endl;
  filet.close();
}

void TravesalASTOutput(std::map<std::string, std::string> &result, int type) {
  std::string typestr;
  if (type == 0) {
    typestr = "preorder";
  }
  if (type == 1) {
    typestr = "inorder";
  }
  if (type == 2) {
    typestr = "postorder";
  }
  if (type == 3) {
    typestr = "tree_struct";
  }
  for (auto iter : result) {
    std::string fname = iter.first;
    fname = fname.replace(fname.end() - 4, fname.end(), "_" + typestr + ".txt");
    gInfoLog << typestr << " Output File: " << fname << "\n";
    std::ofstream file(fname);
    if (type == 3) {
      file << "Root" << " [ " << iter.second << " ] " << std::endl;
    } 
    else {
      file << iter.second << std::endl;
    }
    file.close();
  }

}