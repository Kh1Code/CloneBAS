#include "FixedChecker.h"
#include "InfoController.h"
#include "EventClient.h"

DiagnosticsEngine *g_currentDiagEngine = nullptr;
FixedFileManager g_fixedFileManager;

//分析文件遮罩(防止同一.h文件的AST部件被重复分析)
std::set<int> g_fileMask;
std::set<int> g_fileMaskCache; //分析文件缓存(缓存在一个.c/.cpp文件完成分析后刷新到m_fileMask)

int FixedSimplifier::Simplify(const Stmt *&node) { 
    //获得所有子结点
  Stmt::child_iterator I = node->child_begin();
  Stmt::child_iterator Iend = node->child_end();
  while (I != Iend) {
        if (*I != nullptr) {
          const Stmt *stmt = *I;
          Simplify(stmt);
          *I = const_cast<Stmt *>(stmt);
        }
        ++I;
      }
  if (m_doASTopt) {
       SimplifyStatement(node);
  }
  return 0;
}

int FixedSimplifier::SimplifyCompoundStatement(const Stmt *&node) {
  const CompoundStmt *com_node = llvm::dyn_cast<CompoundStmt>(node);
  FixedCompoundStatement *fixedcom_node = new(g_currentASTContext) FixedCompoundStatement();
  fixedcom_node->InitBeiginLoc(g_currentsm, com_node);
  fixedcom_node->InitEndLoc(g_currentsm, com_node);
  for (Stmt::const_child_iterator I = com_node->child_begin();
       I != com_node->child_end();
       ++I) {
    if (*I != nullptr) {
      const Stmt *temp_node = *I;
      //判断compstmt下的loopstmt是否有提前初始化
      if (const FixedStmt* fstmt = llvm::dyn_cast<FixedStmt>(temp_node)) {
        //for语句的preInit前移
        FixedParentStmt *fpstmt = (FixedParentStmt *) fstmt;
        if (fpstmt->m_fixedType == FixedParentStmt::FixedLoopStmtClass) {
          FixedLoopStatement *loop_stmt = (FixedLoopStatement *)fpstmt;
          if (loop_stmt->m_preInit) {
            fixedcom_node->m_childnodeslist.push_back(loop_stmt->m_preInit);
            loop_stmt->m_preInit = nullptr;
          }
        }
      }
      fixedcom_node->m_childnodeslist.push_back(temp_node);
    }
  }
 //赋值
  node = fixedcom_node;
  return 0;
}

int FixedSimplifier::SimplifyStatement(const Stmt *&temp_node) {
  if (temp_node->getStmtClass() == Stmt::IfStmtClass) {
    // TODO:完善判断直接存在的if的永真永假式
    //gInfoLog << "【SimplifyIfStatement】\n";
    temp_node = SimplifyIfStatement(temp_node);
    return 1;
  } else if (temp_node->getStmtClass() == Stmt::SwitchStmtClass) {
    //gInfoLog << "【SimplifySwitchStatement】\n";
    temp_node = SimplifySwitchStatement(temp_node);
    return 2;
  } else if (temp_node->getStmtClass() == Stmt::ForStmtClass) {
    //gInfoLog << "【SimplifyForStatement】\n";
    temp_node = SimplifyForStatement(temp_node);
    return 3;
  } else if (temp_node->getStmtClass() == Stmt::WhileStmtClass ||
             temp_node->getStmtClass() == Stmt::DoStmtClass) {
    //gInfoLog << "【SimplifyWhileStatement】\n";
    temp_node = SimplifyWhileStatement(temp_node);
    return 4;
  } else if (temp_node->getStmtClass() == Stmt::CompoundStmtClass) {
    //gInfoLog << "【SimplifyCompoundStatement】\n";
    SimplifyCompoundStatement(temp_node);
    return 5;
  }
  else {
    return 0;
  }
}

FixedSelectionStatement *FixedSimplifier::SimplifyIfStatement(const Stmt *&temp_node) {
  FixedSelectionStatement *select_node =
      new (g_currentASTContext) FixedSelectionStatement();
  select_node->InitBeiginLoc(g_currentsm, temp_node);
  select_node->InitEndLoc(g_currentsm, temp_node);
  SelectionUnit *select_unit = new SelectionUnit();
  const Expr *if_expr = nullptr;
  //分析if
  const IfStmt *if_node = llvm::dyn_cast<IfStmt>(temp_node);
  select_unit->m_stmt = CheckStmtLoopPreInitForward(if_node->getThen()); 
  select_unit->m_expr = if_node->getCond();
  if_expr = if_node->getCond();
  DeepSimplifySelection(select_unit, select_node,
                        select_unit->m_stmt); //深入分析
  //分析else
  if (if_node->hasElseStorage()) {
    SelectionUnit *elseselect_unit = new SelectionUnit();
    elseselect_unit->m_expr = CreateUnaryExpr(UnaryOperatorKind::UO_Not, if_expr);
    elseselect_unit->m_stmt = CheckStmtLoopPreInitForward(if_node->getElse());
    DeepSimplifySelection(elseselect_unit, select_node,
                          elseselect_unit->m_stmt); //深入分析
  }
  return select_node;
}

FixedSelectionStatement *FixedSimplifier::SimplifySwitchStatement(const Stmt *&temp_node) {
  FixedSelectionStatement *select_node =
      new (g_currentASTContext) FixedSelectionStatement();
  select_node->InitBeiginLoc(g_currentsm, temp_node);
  select_node->InitEndLoc(g_currentsm, temp_node);
  std::vector<SelectionUnit *> unit_waiting;
  std::stack<const Expr *> all_expr;           //记录所有case对应的expr
  //分析switch中的case和default
  const SwitchStmt *switch_node = llvm::dyn_cast<SwitchStmt>(temp_node);
  const Expr *switch_expr = switch_node->getCond();//switch的头部结点
  //这里的这个结点是FixedCompoundStmt
  FixedCompoundStatement *switch_body_stmt =
      (FixedCompoundStatement *)switch_node->getBody();
  for (const Stmt* I : switch_body_stmt->m_childnodeslist) {
    if (I != nullptr) {
      const Stmt *stmt = I;
      if (const CaseStmt *CS = dyn_cast<CaseStmt>(stmt)) {
        //处理case节点(可能嵌套)
        const CaseStmt *case_stmt = CS;
        //递归出所有嵌套的case
        std::stack<BinaryOperator *> eq_opstack;
        //case中的普通结点
        const Stmt *normal_stmt = nullptr;
        while (true) {
          if (isa<CaseStmt>(case_stmt)) {
            //是一个case嵌套case
            const Expr *expr = case_stmt->getLHS();
            if (const ConstantExpr* constantexpr = llvm::dyn_cast<ConstantExpr>(expr)) {
              if (constantexpr->getSubExpr()) {
                expr = constantexpr->getSubExpr();
              }
            }
            BinaryOperator* eq_op = CreateBinaryExpr(BinaryOperatorKind::BO_EQ, switch_expr, expr);
            eq_opstack.push(eq_op);
          } 
          const Stmt *SubStmt = case_stmt->getSubStmt();
          if (!SubStmt)
            break;    
          if (isa<CaseStmt>(SubStmt)) {
            case_stmt = dyn_cast<CaseStmt>(SubStmt);     
          } else  {
            //处理普通结点(暂时存下来)
            normal_stmt = SubStmt;
            break;
          }
        }
        //获得最终的表达式expr
        const Expr *cond_expr;
        while (eq_opstack.size() > 1) {
          const Expr *expr1 = eq_opstack.top();
          eq_opstack.pop();
          const Expr *expr2 = eq_opstack.top();
          eq_opstack.pop();
          BinaryOperator *or_op =
              CreateBinaryExpr(BinaryOperatorKind::BO_Or, expr1, expr2);
          eq_opstack.push(or_op);
        }
        cond_expr = eq_opstack.top();
        //创建
        SelectionUnit *select_unit = new SelectionUnit();
        //switch的子结点暂时不参与比较
        auto fixed = new (g_currentASTContext) FixedCompoundStatement();
        fixed->InitBeiginLoc(g_currentsm, switch_body_stmt);
        fixed->InitEndLoc(g_currentsm, switch_body_stmt);
        fixed->SetGenFP(false);
        select_unit->m_stmt = fixed;
        select_unit->m_expr = cond_expr;
        all_expr.push(cond_expr);
        unit_waiting.push_back(select_unit);

        //保存并处理之前储存的普通结点
        if (normal_stmt) {
          HandleSwitchNoramlStmt(unit_waiting, normal_stmt);
        }        
      } 
      else if (const DefaultStmt *DS = dyn_cast<DefaultStmt>(stmt)) {
        if (!all_expr.empty()) {
          //有多个case的switch
          const Expr *node_1;
          const Expr *node_2;
          node_1 = all_expr.top();
          all_expr.pop();
          while (!all_expr.empty()) {
            node_2 = all_expr.top();
            all_expr.pop();
            BinaryOperator *and_node =
                CreateBinaryExpr(BinaryOperatorKind::BO_And, node_1, node_2);
            node_1 = and_node;
          }
          UnaryOperator *not_node = CreateUnaryExpr(UnaryOperatorKind::UO_Not, node_1);
          SelectionUnit *default_unit = new SelectionUnit();
          default_unit->m_expr = not_node;
          // switch的子结点暂时不参与比较
          auto fixed = new (g_currentASTContext) FixedCompoundStatement();
          fixed->InitBeiginLoc(g_currentsm, switch_body_stmt);
          fixed->InitEndLoc(g_currentsm, switch_body_stmt);
          fixed->SetGenFP(false);
          default_unit->m_stmt = fixed;
          unit_waiting.push_back(default_unit);
          //处理default节点自带的一般语句
          if (DS->getSubStmt()) {
            HandleSwitchNoramlStmt(unit_waiting, DS->getSubStmt());
          }
        }
      } else if (isa<BreakStmt>(stmt)) {
        //处理break节点
        for (SelectionUnit *selunit : unit_waiting) {
          DeepSimplifySelection(selunit, select_node,
                                selunit->m_stmt); //深入分析
        }
        unit_waiting.clear();
      } else {
        //处理普通结点
        HandleSwitchNoramlStmt(unit_waiting, stmt);
      }
    }
  }
  //所有子结点处理完成,视为遇到了最后的Break
  for (SelectionUnit *selunit : unit_waiting) {
    DeepSimplifySelection(selunit, select_node,
                          selunit->m_stmt); //深入分析
  }
  unit_waiting.clear();
  return select_node;
}

FixedLoopStatement *FixedSimplifier::SimplifyForStatement(const Stmt *&temp_node) {
  FixedLoopStatement *loop_node = 
      new(g_currentASTContext) FixedLoopStatement();
  loop_node->InitBeiginLoc(g_currentsm, temp_node);
  loop_node->InitEndLoc(g_currentsm, temp_node);
  const ForStmt *for_node = dyn_cast<ForStmt>(temp_node);
  loop_node->m_expr = for_node->getCond();
  loop_node->m_stmt = CheckStmtLoopPreInitForward(for_node->getBody());
  //for语句后续执行的部分可以看做末尾的stmt
  if (for_node->getInc()) {
    if (const FixedStmt *stmt = llvm::dyn_cast<FixedStmt>(loop_node->m_stmt)) {
      FixedParentStmt *fstmt = (FixedParentStmt *)stmt;
      if (fstmt->m_fixedType == FixedParentStmt::FixedCompoundStmtClass) {
        FixedCompoundStatement *comp_stmt = (FixedCompoundStatement *)fstmt;
        comp_stmt->m_childnodeslist.push_back(for_node->getInc());
      }
    }
  }
  //for语句的preinit应该在上一层的fixedcompstmt中
  if (for_node->getInit()) {
    loop_node->m_preInit = for_node->getInit(); //临时缓存，父节点时计算
  }
  return loop_node;
}

FixedLoopStatement *FixedSimplifier::SimplifyWhileStatement(const Stmt *&temp_node) {
  FixedLoopStatement *loop_node =
      new (g_currentASTContext) FixedLoopStatement();
  if (temp_node->getStmtClass() == Stmt::WhileStmtClass) {
    const WhileStmt *while_node = dyn_cast<WhileStmt>(temp_node);
    loop_node->m_expr = while_node->getCond();
    loop_node->m_stmt = CheckStmtLoopPreInitForward(while_node->getBody());
  } else if (temp_node->getStmtClass() == Stmt::DoStmtClass) {
    const DoStmt *do_node = dyn_cast<DoStmt>(temp_node);
    loop_node->m_expr = do_node->getCond();
    loop_node->m_stmt = CheckStmtLoopPreInitForward(do_node->getBody());
    loop_node->m_isdo = true;
  }
  loop_node->InitBeiginLoc(g_currentsm, temp_node);
  loop_node->InitEndLoc(g_currentsm, temp_node);
  return loop_node;
}

int FixedSimplifier::DeepSimplifySelection(SelectionUnit *selectunit,
                                   FixedSelectionStatement *selectnode,
                                        const Stmt *stmtnode) {
  //预处理,提取出所有const Stmt *,之后顺序分析
  std::vector<const Stmt *> select_nodes;
  const FixedParentStmt* fixed_stmt =
      (FixedParentStmt *)llvm::dyn_cast<FixedStmt>(stmtnode);
  FixedCompoundStatement *com_node = nullptr;
  if (fixed_stmt &&
      fixed_stmt->m_fixedType == FixedParentStmt::FixedCompoundStmtClass) {
    com_node = (FixedCompoundStatement *)fixed_stmt;
    for (const Stmt *node : com_node->m_childnodeslist) {
      select_nodes.push_back(node);
    }
    com_node->m_childnodeslist.clear();
  }
  const FixedSelectionStatement *select_node = nullptr;
  bool isFixedCompStmt =
      fixed_stmt && fixed_stmt->m_fixedType == FixedParentStmt::FixedSelectionStmtClass;
  if (isFixedCompStmt) {
    select_node = (FixedSelectionStatement *)fixed_stmt;
    select_nodes.push_back(stmtnode);
  }
  if (!com_node) {
    com_node = new(g_currentASTContext) FixedCompoundStatement();
  }
  //判断每个const Stmt *的类型并处理
  for (const Stmt *child_node : select_nodes) {
    if (isFixedCompStmt) {
      std::vector<int> remove;
      int index = 0;
      for (SelectionUnit *select_unit : select_node->m_selectionunits) {
        //只有独立结点才能和上层合并(这部分暂时不跑)
        if (/*select_unit->m_independent*/false) {
          remove.push_back(index); //和上层结点合并,移除自身
          SelectionUnit *merge_unit = new SelectionUnit();
          BinaryOperator *and_node =
              CreateBinaryExpr(BinaryOperatorKind::BO_And, selectunit->m_expr,
                               select_unit->m_expr);
          merge_unit->m_expr = and_node; //和并表达式
          merge_unit->m_stmt = select_unit->m_stmt;
          selectnode->m_selectionunits.push_back(merge_unit);
        }
        index++;
      }
      if (select_node->m_selectionunits.size() != remove.size()) {
        //这个选择中有const Stmt *不是上文无关的
        FixedSelectionStatement * select_node_ptr =
            const_cast<FixedSelectionStatement *>(select_node);
        for (int i = remove.size() - 1; i >= 0; i--) {
          select_node_ptr->m_selectionunits.erase(
              select_node_ptr->m_selectionunits.begin() + remove[i]);
        }
        com_node->m_childnodeslist.push_back(select_node_ptr);
      }
    } else {
      com_node->m_childnodeslist.push_back(child_node);
    }
  }
  if (com_node->m_childnodeslist.size() > 0) {
    stmtnode = com_node;
    selectnode->m_selectionunits.push_back(selectunit); //默认操作
  }
  return 0;
}

void FixedSimplifier::HandleSwitchNoramlStmt(
    std::vector<SelectionUnit *> unit_waiting, const Stmt *stmt) {
  const Stmt *for_preInit = nullptr;
  if (const FixedStmt *fstmt = llvm::dyn_cast<FixedStmt>(stmt)) {
    FixedParentStmt *fpstmt = (FixedParentStmt *)fstmt;
    if (fpstmt->m_fixedType == FixedParentStmt::FixedLoopStmtClass) {
      FixedLoopStatement *loop_stmt = (FixedLoopStatement *)fpstmt;
      for_preInit = loop_stmt->m_preInit;
      loop_stmt->m_preInit = nullptr;
    }
  }
  for (SelectionUnit *selunit : unit_waiting) {
    if (for_preInit) {
      ((FixedCompoundStatement *)selunit->m_stmt)
          ->m_childnodeslist.push_back(for_preInit);
    }
    ((FixedCompoundStatement *)selunit->m_stmt)
        ->m_childnodeslist.push_back(stmt);
  }
}

UnaryOperator *FixedSimplifier::CreateUnaryExpr(UnaryOperatorKind unaryOp,
                                             const Expr *stmt) {
  QualType type = g_currentASTContext->BoolTy;
  SourceLocation loc;
  Expr *expr_ptr = const_cast<Expr *>(stmt);
  UnaryOperator *unary_expr = UnaryOperator::Create(
      *g_currentASTContext, expr_ptr, unaryOp, type, ExprValueKind::VK_PRValue,
      OK_Ordinary, loc, false, FPOptionsOverride());
  return unary_expr;
}

BinaryOperator *FixedSimplifier::CreateBinaryExpr(BinaryOperatorKind binaryOp,
                                               const Expr *stmtl,
                                               const Expr *stmtr) {
  QualType type = g_currentASTContext->BoolTy;
  SourceLocation loc;
  Expr *exprl_ptr = const_cast<Expr *>(stmtl);
  Expr *exprr_ptr = const_cast<Expr *>(stmtr);
  BinaryOperator *binary_expr = BinaryOperator::Create(
      *g_currentASTContext, exprl_ptr, exprr_ptr, binaryOp, type,
      ExprValueKind::VK_PRValue, OK_Ordinary, loc, FPOptionsOverride());
  return binary_expr;
}

const Stmt *FixedSimplifier::CheckStmtLoopPreInitForward(const Stmt *temp_node) {
   //正常的有{}的代码块
  if (const FixedStmt *fstmt = llvm::dyn_cast<FixedStmt>(temp_node)) {
    FixedParentStmt *fpstmt = (FixedParentStmt *)fstmt;
    if (fpstmt->m_fixedType == FixedParentStmt::FixedCompoundStmtClass) {
      return temp_node;
    }
  } 
  //下一个是for,preinit前移
  if (const FixedStmt *fstmt = llvm::dyn_cast<FixedStmt>(temp_node)) {
    FixedParentStmt *fpstmt = (FixedParentStmt *)fstmt;
    if (fpstmt->m_fixedType == FixedParentStmt::FixedLoopStmtClass) {
      FixedLoopStatement *loop_stmt = (FixedLoopStatement *)fpstmt;
      if (loop_stmt->m_preInit) {
        //是for语句且存在preInit,进行preInit前移并且创造新的comp_stmt来存放;
        FixedCompoundStatement *fixedcom_node =
            new (g_currentASTContext) FixedCompoundStatement();
        fixedcom_node->InitBeiginLoc(g_currentsm, loop_stmt);
        fixedcom_node->InitEndLoc(g_currentsm, loop_stmt);
        fixedcom_node->m_childnodeslist.push_back(loop_stmt->m_preInit);
        loop_stmt->m_preInit = nullptr;
        fixedcom_node->m_childnodeslist.push_back(loop_stmt);
        return fixedcom_node;
      }
    }
  } 
  //下一个不是{}的代码块,包裹在fixedcompstmt中
  FixedCompoundStatement *fixedcom_node =
      new (g_currentASTContext) FixedCompoundStatement();
  fixedcom_node->InitBeiginLoc(g_currentsm, temp_node);
  fixedcom_node->InitEndLoc(g_currentsm, temp_node);
  fixedcom_node->m_childnodeslist.push_back(temp_node);
  return fixedcom_node;
}

int FixedChecker::Check(const Decl *&node) {
  bool enter = TryEnterScoreUnit(node);
  g_infoController.TryTraceSymbol(node);
  if (const FunctionDecl *funcdecl = llvm::dyn_cast<FunctionDecl>(node)) {
    if (funcdecl->hasBody() && funcdecl->isThisDeclarationADefinition()) {
      CheckFunctionDecl(funcdecl);
    }
  }
  if (node->hasBody()) {
    const Stmt *body = node->getBody();
    Check(body);
  }
  if (llvm::isa<DeclContext>(node)) {
    const DeclContext *declcontext = llvm::dyn_cast<DeclContext>(node);
    for (auto it = declcontext->decls_begin();
         it != declcontext->decls_end(); ++it) { 
      if (*it != node && *it != nullptr) {
        const Decl *decl = *it;
        Check(decl);
      }
    }
  }
  TryLeaveScoreUnit(node, enter);
  return 0;
}

int FixedChecker::Check(const Stmt *&node) {
  bool enter = TryEnterScoreUnit(node);

  bool iscompstmt = node->getStmtClass() == Stmt::CompoundStmtClass;
  bool ismembercall = node->getStmtClass() == Stmt::MemberExprClass;
  if (iscompstmt) {
    m_stmtdepth++;
  }
  int current_calldepth = 0;
  if (ismembercall) {
    m_calldepth++;
    m_depthest_calldepth = m_calldepth;
    current_calldepth = m_calldepth;
  }
  //检查代码深度是否过深
  if ((m_stmtdepth > 5) && iscompstmt) {
    SendInDefineWarning("高代码块深度" + std::to_string(m_stmtdepth), node->getBeginLoc(), node->getEndLoc(),
                        std::vector<std::string>{std::to_string(m_stmtdepth)}, 100002);
    EventCenter::g_eventCenter.EventTrigger(EventName::BlockDepth, m_stmtdepth);
  }
  bool isroot = false;
  if (iscompstmt && m_currentRootComp == nullptr) {
    m_currentRootComp = node;
    isroot = true;
    //计算根结点的圈复杂度
    int complexity = CheckConditionStmt(node) + 1;
    if (complexity > 5) {
      SendInDefineWarning("高圈复杂度" + std::to_string(complexity),
                          node->getBeginLoc(), node->getEndLoc(),
                          std::vector<std::string>{std::to_string(complexity)},
                          100001);
    }
    EventCenter::g_eventCenter.EventTrigger(EventName::CycleComplexity, complexity);
  }
  //遍历所有子Stmt
  Stmt::child_iterator I = node->child_begin();
  Stmt::child_iterator Iend = node->child_end();
  while (I != Iend) {
    if (*I != nullptr) {
      const Stmt *stmt = *I;
      Check(stmt);
    }
    ++I;
  }
  //遍历所有子Decl(貌似只有DeclStmt有子Decl)
  if (const DeclStmt *declstmt = llvm::dyn_cast<DeclStmt>(node)) {
    for (const Decl *decl : declstmt->getDeclGroup()) {
      Check(decl);
    }
  }

  if (isroot) {
    m_currentRootComp = nullptr;
  }
  if (iscompstmt) {
    m_stmtdepth--;
  }
  //检查代码链式调用是否过深
  if ((current_calldepth == m_depthest_calldepth) && (current_calldepth > 1)) {
    EventCenter::g_eventCenter.EventTrigger(EventName::CallDepth,
                                            m_depthest_calldepth);
  }
  if ((current_calldepth == m_depthest_calldepth) && (current_calldepth > 3) && ismembercall) {
    SendInDefineWarning("高代码链式调用深度" + std::to_string(m_depthest_calldepth),
        node->getBeginLoc(), node->getEndLoc(),
        std::vector<std::string>{std::to_string(m_depthest_calldepth)}, 100003);
  }
  if (ismembercall) {
    m_calldepth--;
  }

  TryLeaveScoreUnit(node, enter);
  return 0;
}

int FixedChecker::CheckConditionStmt(const Stmt *stmt) { 
  int complexity = 0;
  const SwitchStmt *switchstmt = nullptr;
  if (stmt->getStmtClass() == Stmt::IfStmtClass ||
      stmt->getStmtClass() == Stmt::ForStmtClass ||
      stmt->getStmtClass() == Stmt::WhileStmtClass ||
      stmt->getStmtClass() == Stmt::DoStmtClass) {
    complexity++;
  } else if (stmt->getStmtClass() == Stmt::SwitchStmtClass) {
    switchstmt = llvm::dyn_cast<SwitchStmt>(stmt);  
  }
  Stmt::child_iterator I = stmt->child_begin();
  Stmt::child_iterator Iend = stmt->child_end();
  while (I != Iend) {
    if (*I != nullptr) {
      const Stmt *stmt = *I;
      complexity += CheckConditionStmt(stmt);
      if (switchstmt && (stmt->getStmtClass() == Stmt::CaseStmtClass || stmt->getStmtClass() == Stmt::DefaultStmtClass)) {
        complexity++;
      }
    }
    ++I;
  }
  return complexity;
}

int FixedChecker::CheckFunctionDecl(const FunctionDecl *funcDecl) { 
    int params = funcDecl->param_size();
    if (params > 3) {
      SendInDefineWarning("过多参数的函数" + std::to_string(params), funcDecl->getBeginLoc(), funcDecl->getEndLoc(),
                          std::vector<std::string>{std::to_string(params)}, 100004);
    }
    EventCenter::g_eventCenter.EventTrigger(EventName::ParamNum, params);
    return 0;
}

void FixedChecker::CatchDiagnostic(const Diagnostic &Info, DiagnosticsEngine::Level& dlevel) {
  m_diagnosticCollection.push_back(DiagnosticWithLevel(Info, dlevel));
}

std::set<int> m_maskDiagID{1142};

void FixedChecker::MatchDiagnosticCollection(ScoreUnitVector *globalST,
                                             std::string filename) {
  m_socreTracerList.ResetCurrent();
  for (int i = 0; i < m_diagnosticCollection.size(); i++) {
    DiagnosticWithLevel diagw = m_diagnosticCollection.at(i);
    auto dlevel = diagw.m_dlevel;
    if (m_maskDiagID.count(diagw.m_id)) {
      continue;
    }
    if (g_fileMask.count(diagw.m_fileID)) {
      continue;
    }
    m_socreTracerList.MoveCurrent(diagw.m_line, diagw.m_column, diagw.m_fileID, false);
    // 输出测试
    if (m_displayDiag) {
      gInfoLog << diagw.m_display;
    }
    //EventCenter触发
    switch (diagw.m_dlevel) { 
        case DiagnosticsEngine::Level::Error:
          EventCenter::g_eventCenter.EventTrigger(EventName::CaughtError, IErrorInfo(diagw.m_diagText, diagw.m_line, diagw.m_column, diagw.m_fileID, diagw.m_id, diagw.m_placeHolders));
          break;
        case DiagnosticsEngine::Level::Fatal:
          EventCenter::g_eventCenter.EventTrigger(
              EventName::CaughtWarning,
              IWarningInfo(diagw.m_diagText, diagw.m_line, diagw.m_column,
                           diagw.m_fileID, ScoreUnit::SECURITY, 2, diagw.m_id,
                           true, diagw.m_placeHolders));
          break;
        case DiagnosticsEngine::Level::Warning:
          EventCenter::g_eventCenter.EventTrigger(
              EventName::CaughtWarning,
              IWarningInfo(diagw.m_diagText, diagw.m_line, diagw.m_column,
                           diagw.m_fileID, ScoreUnit::SECURITY, 1, diagw.m_id,
                           true, diagw.m_placeHolders));
          break;
        case DiagnosticsEngine::Level::Note:
          EventCenter::g_eventCenter.EventTrigger(
              EventName::CaughtWarning,
              IWarningInfo(diagw.m_diagText, diagw.m_line, diagw.m_column,
                           diagw.m_fileID, ScoreUnit::SECURITY, 0, diagw.m_id,
                           true, diagw.m_placeHolders));
          break;
    }
  }
  m_socreTracerList.MoveCurrent(0, 0, -1, true);
  m_socreTracerList.Clear();
  m_diagnosticCollection.clear();
}

bool FixedChecker::TryEnterScoreUnit(const Decl *node) { 
    if (!node) {return false;}
    ScoreUnitVector *svector = nullptr;
    if (llvm::isa<FunctionDecl>(node) && node->hasBody()) {
      const FunctionDecl *funcdecl = llvm::dyn_cast<FunctionDecl>(node);
      if (funcdecl->isThisDeclarationADefinition()) {
        ScoreController::g_scoreController.EnterFunctionScoreUnitVector(
            svector, node->getBeginLoc(), node->getBody()->getEndLoc(), funcdecl->getNameAsString());
        m_socreTracerList.TraceUnit(node->getBeginLoc(), svector);
        unsigned beginOffset = g_currentsm->getFileOffset(node->getBeginLoc());
        unsigned endOffset = beginOffset;
        if (node->getBody()->getEndLoc().isValid()) {
          endOffset = g_currentsm->getFileOffset(node->getBody()->getEndLoc());
        }
        svector->m_charNumWeight = endOffset - beginOffset + 1;
      } else {
        return false; 
      }
    } else if (llvm::isa<CXXRecordDecl>(node)) {     
      const CXXRecordDecl* record = llvm::dyn_cast<CXXRecordDecl>(node);
      if (record->hasDefinition()) {
        ScoreController::g_scoreController.EnterClassScoreUnitVector(
            svector, node->getBeginLoc(), record->getDefinition()->getEndLoc(), record->getNameAsString());
        m_socreTracerList.TraceUnit(node->getBeginLoc(), svector);
        unsigned beginOffset = g_currentsm->getFileOffset(node->getBeginLoc());
        unsigned endOffset = beginOffset;
        if (record->getDefinition()->getEndLoc().isValid()) {
          endOffset = g_currentsm->getFileOffset(record->getDefinition()->getEndLoc());
        }
        svector->m_charNumWeight = endOffset - beginOffset + 1;
      }
    }
    else {
      return false; 
    }
    return true;
}

bool FixedChecker::TryEnterScoreUnit(const Stmt *node) { 
  return false; 
}

bool FixedChecker::TryLeaveScoreUnit(const Decl *node, bool toleave) {
  if (toleave) {
    if (llvm::isa<FunctionDecl>(node) && node->hasBody()) {
      m_socreTracerList.TraceUnit(node->getBody()->getEndLoc(), nullptr);
      ScoreController::g_scoreController.ExitScoreUnitVector();
    } else if (llvm::isa<CXXRecordDecl>(node)) {
      const CXXRecordDecl *record = llvm::dyn_cast<CXXRecordDecl>(node);
      if (record->hasDefinition()) {
        ScoreController::g_scoreController.ExitScoreUnitVector();
        m_socreTracerList.TraceUnit(record->getDefinition()->getEndLoc(), nullptr);
      }
    }
    return true;
  }
  return false;
}

bool FixedChecker::TryLeaveScoreUnit(const Stmt *node, bool toleave) {
  if (toleave) {
    m_socreTracerList.TraceUnit(node->getEndLoc(), nullptr);
    ScoreController::g_scoreController.ExitScoreUnitVector();
    return true;
  }
  return false;
}

unsigned FixedChecker::AddOffsetsToTotal(const Decl *decl) {
  unsigned len = 0;
  if (llvm::isa<FunctionDecl>(decl) && decl->hasBody()) {
    const FunctionDecl *funcdecl = llvm::dyn_cast<FunctionDecl>(decl);
    if (funcdecl->isThisDeclarationADefinition()) {
      unsigned beginOffset = g_currentsm->getFileOffset(decl->getBeginLoc());
      if (decl->getBody()->getEndLoc().isInvalid())
        return 0;
      unsigned endOffset = g_currentsm->getFileOffset(decl->getBody()->getEndLoc());
      len = endOffset - beginOffset;
    } else {
      unsigned beginOffset = g_currentsm->getFileOffset(decl->getBeginLoc());
      if (decl->getEndLoc().isInvalid())
        return 0;
      unsigned endOffset = g_currentsm->getFileOffset(decl->getEndLoc());
      len = endOffset - beginOffset;
    }
  } else if (llvm::isa<CXXRecordDecl>(decl)) {
    const CXXRecordDecl *record = llvm::dyn_cast<CXXRecordDecl>(decl);
    if (record->hasDefinition()) {
      unsigned beginOffset = g_currentsm->getFileOffset(decl->getBeginLoc());
      if (record->getDefinition()->getEndLoc().isInvalid())
        return 0;
      unsigned endOffset = g_currentsm->getFileOffset(record->getDefinition()->getEndLoc());
      len = endOffset - beginOffset;
    }
  } else if (decl->hasBody()) {
    unsigned beginOffset = g_currentsm->getFileOffset(decl->getBeginLoc());
    if (decl->getBody()->getEndLoc().isInvalid())
      return 0;
    unsigned endOffset = g_currentsm->getFileOffset(decl->getBody()->getEndLoc());
    len = endOffset - beginOffset;
  } else {
    unsigned beginOffset = g_currentsm->getFileOffset(decl->getBeginLoc());
    if (decl->getEndLoc().isInvalid())
      return 0;
    unsigned endOffset = g_currentsm->getFileOffset(decl->getEndLoc());
    len = endOffset - beginOffset;  
  }
  m_totalchar += len + 1;
  return len;
}

void FixedChecker::SendInDefineWarning(std::string str, SourceLocation beginLoc, SourceLocation endloc, std::vector<std::string> placeHolders, int id) {
  clang::PresumedLoc locInfo = g_currentsm->getPresumedLoc(beginLoc);
  std::string file = locInfo.getFilename();
  int fid = g_fixedFileManager.GetOrAddFiexdFileID(file);
  unsigned begin_line = locInfo.getLine();
  unsigned begin_colum = locInfo.getColumn();
  locInfo = g_currentsm->getPresumedLoc(endloc);
  unsigned end_line = locInfo.getLine();
  unsigned end_colum = locInfo.getColumn();
  EventCenter::g_eventCenter.EventTrigger(
      EventName::CaughtWarning,
      IWarningInfo(str, begin_line, begin_colum, fid, ScoreUnit::SECURITY, 0,
                   id, false, placeHolders));
  if (!m_displayDiag)
    return;
  gInfoLog << "【内部添加】" << str << "(" << file << "|" << begin_line
               << "," << begin_colum << "|" << end_line << "," << end_colum
               << ")\n";
}

  static std::string ArgumentKindToString(DiagnosticsEngine::ArgumentKind kind) {
  switch (kind) {
  case clang::DiagnosticsEngine::ak_std_string:
    return "std::string";
  case clang::DiagnosticsEngine::ak_c_string:
    return "const char*";
  case clang::DiagnosticsEngine::ak_sint:
    return "int";
  case clang::DiagnosticsEngine::ak_uint:
    return "unsigned";
  case clang::DiagnosticsEngine::ak_tokenkind:
    return "TokenKind";
  case clang::DiagnosticsEngine::ak_identifierinfo:
    return "IdentifierInfo*";
  case clang::DiagnosticsEngine::ak_addrspace:
    return "AddressSpace";
  case clang::DiagnosticsEngine::ak_qual:
    return "Qualifiers";
  case clang::DiagnosticsEngine::ak_qualtype:
    return "QualType";
  case clang::DiagnosticsEngine::ak_declarationname:
    return "DeclarationName";
  case clang::DiagnosticsEngine::ak_nameddecl:
    return "NamedDecl*";
  case clang::DiagnosticsEngine::ak_nestednamespec:
    return "NestedNameSpecifier*";
  case clang::DiagnosticsEngine::ak_declcontext:
    return "DeclContext*";
  case clang::DiagnosticsEngine::ak_qualtype_pair:
    return "QualTypePair";
  case clang::DiagnosticsEngine::ak_attr:
    return "Attr*";
  default:
    return "unknown";
  }
}

union U64toOtherType {
  U64toOtherType() {}
  uint64_t _ui64;
  Qualifiers _qualifier;
  QualType _qtype;
  DeclarationName _dname;
};

DiagnosticWithLevel::DiagnosticWithLevel(const Diagnostic &diag,
                                         DiagnosticsEngine::Level dlevel) {
  SourceLocation loc = diag.getLocation();
  FullSourceLoc fullLoc(loc, diag.getSourceManager());
  std::string fname = fullLoc.getManager().getFilename(fullLoc).str();
  unsigned line = fullLoc.getSpellingLineNumber();
  unsigned column = fullLoc.getSpellingColumnNumber();

  // 获取占位符
  U64toOtherType castunion;
  std::string placeholders = "";
  std::string display = "";
  NamedDecl *ndptr = nullptr;
  NestedNameSpecifier *nnsptr = nullptr;
  m_fileID = g_fixedFileManager.GetOrAddFiexdFileID(fname);
  m_id = diag.getID();

  for (unsigned i = 0, e = diag.getNumArgs(); i != e; ++i) {
    // 获取占位符的类型
    const DiagnosticsEngine::ArgumentKind kind = diag.getArgKind(i);
    diag.getRawArg(i);
    // 获取占位符类型的字符串表示
    std::string kname = ArgumentKindToString(kind);
    std::string arg = "---";
    switch (kind) {
    case clang::DiagnosticsEngine::ak_std_string:
      arg = diag.getArgStdStr(i);
      break;
    case clang::DiagnosticsEngine::ak_c_string:
      arg = diag.getArgCStr(i);
      break;
    case clang::DiagnosticsEngine::ak_sint:
      arg = std::to_string(diag.getArgSInt(i));
      break;
    case clang::DiagnosticsEngine::ak_uint:
      arg = std::to_string(diag.getArgUInt(i));
      break;
    case clang::DiagnosticsEngine::ak_tokenkind:
      break;
    case clang::DiagnosticsEngine::ak_identifierinfo: 
      break;
    case clang::DiagnosticsEngine::ak_addrspace:
      break;
    case clang::DiagnosticsEngine::ak_qual:
      castunion._ui64 = diag.getRawArg(i);
      arg = castunion._qualifier.getAsString();
      break;
    case clang::DiagnosticsEngine::ak_qualtype:
      castunion._ui64 = diag.getRawArg(i);
      arg = castunion._qtype.getAsString();
      break;
    case clang::DiagnosticsEngine::ak_declarationname:
      castunion._ui64 = diag.getRawArg(i);
      arg = castunion._dname.getAsString();
      break;
    case clang::DiagnosticsEngine::ak_nameddecl:
      ndptr = (NamedDecl *)diag.getRawArg(i);
      arg = ndptr->getNameAsString();
      break;
    case clang::DiagnosticsEngine::ak_nestednamespec:
      nnsptr = (NestedNameSpecifier *)diag.getRawArg(i);
      if (auto *ns = nnsptr->getAsNamespace()) {
        arg = ns->getNameAsString();
      }
      if (auto *type = nnsptr->getAsType()) {
        arg = type->getTypeClassName();
      }
      if (auto *prefix = nnsptr->getPrefix()) {
        arg = "递归名称NestedNameSpecifier";
      }
      break;
    case clang::DiagnosticsEngine::ak_declcontext:
      arg = "DeclContext";
      break;
    case clang::DiagnosticsEngine::ak_qualtype_pair:
      break;
    case clang::DiagnosticsEngine::ak_attr:
      arg = "Attr";
      break;
    }
    
    m_placeHolders.push_back(arg);
    placeholders +=
        "【" + std::to_string(i) + " , " + kname + " ," + arg + "】";
  }
  // 获取报错信息
  llvm::SmallString<100> message;
  diag.FormatDiagnostic(message);
  std::string msg = message.str().str();
  if (dlevel == DiagnosticsEngine::Warning) {
    display += "【外部捕捉】Warning: " + placeholders;
  } else if (dlevel == DiagnosticsEngine::Error) {
    display += "【外部捕捉】Error: " + placeholders;
  } else if (dlevel == DiagnosticsEngine::Note) {
    display += "【外部捕捉】Note: " + placeholders;
  } else if (dlevel == DiagnosticsEngine::Fatal) {
    display += "【外部捕捉】Fatal: " + placeholders;
  }
  display += "【ID:" + std::to_string(m_id) + "】";
  display += "[FID:" + std::to_string(m_fileID) + "] " + fname + ":" +
             std::to_string(line) + ":" +
             std::to_string(column) +
             ": " + msg + "\n";

  m_dlevel = dlevel;
  m_line = line;
  m_column = column;
  m_display = display;
  m_diagText = msg;
}

void FixedFileManager::AddFile(std::string& file) {
  m_fileMap.emplace(file, m_fileAmount);
  m_fileAmount++;
}

int FixedFileManager::GetFiexdFileID(std::string& file) { 
  if (m_fileMap.find(file) == m_fileMap.end()) {
    return -1;
  }
  else {
    return m_fileMap.find(file)->second;
  }
}

int FixedFileManager::GetOrAddFiexdFileID(std::string& file) { 
  if (m_fileMap.find(file) == m_fileMap.end()) {
    AddFile(file);
    return m_fileAmount - 1;
  } else {
    return m_fileMap.find(file)->second;
  }
}