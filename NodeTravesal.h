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

extern bool g_opNodeDiff;//�Ƿ���Ϊ����ʽ����Ӱ�����
extern bool g_opNodeType; //�Ƿ���Ϊ����ʽ����Ӱ�����
extern bool g_noLiteral;  //�Ƿ���Ϊ����ʽ����Ӱ�����
extern bool g_noDecl;  //�Ƿ���Ϊ����ʽ����Ӱ�����
extern bool g_noDeclStmt;
extern bool g_noArraySExpr;
extern bool g_arrType;


//0ǰ�� 1����(�����զ����) 2���� 
extern void TravesalAST(const Stmt *root, std::string& output, int type);
extern void TravesalAST(const Decl *root, std::string& output, int type);

//ͳ��AST�ϵĽ������
extern void CountASTNode(const Stmt *root, std::map<std::string, int>& result);
extern void CountASTNode(const Decl *root, std::map<std::string, int> &result);


//���AST���ͳ�ƽ��
extern void CountASTOutput(std::map<std::string, int> &result);

//���AST���������
extern void TravesalASTOutput(std::map<std::string, std::string> &result,
                              int type);