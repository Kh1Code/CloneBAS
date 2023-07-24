#pragma once
#include<iostream>
#include<string>
#include<map>
#include"MurmurHash3.h"
#include"SimHashConfig.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Lex/Lexer.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Execution.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Signals.h"
#include <clang/Frontend/CompilerInstance.h>
#include "Log.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

extern bool g_opNodeDiff;//是否认为运算式类型影响分类
extern bool g_opNodeType; //是否认为运算式类型影响分类
extern bool g_noLiteral;  //是否认为运算式类型影响分类
extern bool g_noDecl;  //是否认为运算式类型影响分类
extern bool g_noDeclStmt;
extern bool g_noArraySExpr;
extern bool g_arrType;


//0前序 1中序(多叉树咋中序) 2后序 
extern void TravesalAST(const Stmt *root, std::string& output, int type);
extern void TravesalAST(const Decl *root, std::string& output, int type);

//统计AST上的结点类型
extern void CountASTNode(const Stmt *root, std::map<std::string, int>& result);
extern void CountASTNode(const Decl *root, std::map<std::string, int> &result);


//输出AST结点统计结果
extern void CountASTOutput(std::map<std::string, int> &result);

//输出AST结点便利结果
extern void TravesalASTOutput(std::map<std::string, std::string> &result,
                              int type);