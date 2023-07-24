#include "FixedChecker.h"
#include <iostream>
#include <clang/Frontend/CompilerInstance.h>
#include <filesystem>
#include "SimHasher.h"
#include "NodeTravesal.h"
#include "CreateObjList.h"
#include "AnaHeader.h"
#include "EventClient.h"
#include "InfoController.h"
#include "FingerPrintGenerator.h"

class Timer
{
public:
  Timer() : start_(), end_() {}

  void Start() { QueryPerformanceCounter(&start_); }

  void Stop() { QueryPerformanceCounter(&end_); }

  double GetElapsedMilliseconds()
  {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (end_.QuadPart - start_.QuadPart) * 1000.0 / freq.QuadPart;
  }

private:
  LARGE_INTEGER start_;
  LARGE_INTEGER end_;
};

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

// 编码转换的函数
std::string gb2312_to_utf8(const std::string &str_gb2312)
{
  int len = MultiByteToWideChar(CP_ACP, 0, str_gb2312.c_str(), -1, nullptr, 0);
  if (len == 0)
  {
    return "";
  }

  std::wstring str_wide(len, L'\0');
  MultiByteToWideChar(CP_ACP, 0, str_gb2312.c_str(), -1, &str_wide[0], len);

  len = WideCharToMultiByte(CP_UTF8, 0, str_wide.c_str(), -1, nullptr, 0,
                            nullptr, nullptr);
  if (len == 0)
  {
    return "";
  }

  std::string str_utf8(len, '\0');
  WideCharToMultiByte(CP_UTF8, 0, str_wide.c_str(), -1, &str_utf8[0], len,
                      nullptr, nullptr);

  return str_utf8;
}

std::string utf8_to_gb2312(const std::string &str_utf8)
{
  int nLength =
      ::MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, NULL, 0);
  wchar_t *wszUnicode = new wchar_t[nLength + 1];
  ::MultiByteToWideChar(CP_UTF8, 0, str_utf8.c_str(), -1, wszUnicode, nLength);

  nLength =
      ::WideCharToMultiByte(CP_ACP, 0, wszUnicode, -1, NULL, 0, NULL, NULL);
  char *szAnsi = new char[nLength + 1];
  ::WideCharToMultiByte(CP_ACP, 0, wszUnicode, -1, szAnsi, nLength, NULL, NULL);

  std::string strRet(szAnsi);
  delete[] wszUnicode;
  delete[] szAnsi;
  return strRet;
}

static FixedSimplifier g_fixedSimplifier;
static FixedChecker g_fixedChecker;

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::OptionCategory ToolTemplateCategory("tool-template options");

// 参数
static cl::opt<std::string> Folder("f", cl::desc("分析文件夹"), cl::Required);
static cl::opt<int> Port("port", cl::desc("前端端口"), cl::init(12345));
static cl::opt<bool> GenFile("genfile", cl::desc("生成一个和代码对应的文件"),
                             cl::init(false));
static cl::opt<bool> onlyDiffFile("onlydiffF", cl::desc("不检测文件内部的相似"),
                                  cl::init(false));
static cl::opt<bool> onlySameFile("onlysameF", cl::desc("只检测文件内部的相似"),
                                  cl::init(false));
static cl::opt<bool> Astopt("astopt", cl::desc("优化AST"), cl::init(false));
static cl::opt<bool> DisplayDiag("catchdiag", cl::desc("外部捕获报错"), cl::init(false));
static cl::opt<bool> ShowDetailClone("detailclone", cl::desc("展示详细重复信息"), cl::init(false));
static cl::opt<bool> ShowDetailFG("detailfg",
                                  cl::desc("展示每个指纹的信息"),
                                  cl::init(false));
static cl::opt<bool> ShowAST("showast",
                             cl::desc("展示优化前和优化后的AST"),
                             cl::init(false));
static cl::opt<bool> ShowCsetInfo("csetinfo", cl::desc("展示克隆集合的详细信息"),
                                  cl::init(false));
static cl::opt<bool> NoLog("nolog", cl::desc("无log输出"),
                           cl::init(false));
static cl::opt<bool> NoCheck("nocheck", cl::desc("不进行代码分析"),
                             cl::init(false));
static cl::opt<bool> ShowSyms("showsym", cl::desc("显示sym格式规范统计信息"),
                              cl::init(false));
static cl::opt<bool> fileBased("filebased", cl::desc("一个文件生成一个指纹"),
                               cl::init(false));

static cl::opt<bool> clonemodel("clonemodel", cl::desc("使用克隆检测的模型"),
                                cl::init(false));
static cl::opt<std::string> cloneIP("cloneIP", cl::desc("克隆模型服务的IP"), cl::Required);
static cl::opt<std::string> cloneport("cloneport", cl::desc("克隆模型服务的端口"), cl::Required);

static cl::opt<bool> nodeCount("nodeCount", cl::desc("统计结点数量以及对应的类型"),
                               cl::init(false));
static cl::opt<bool> noSimHash("nosimhash", cl::desc("不进行重复度计算"),
                               cl::init(false));

static cl::opt<bool> opNodeDiff("opnodediff", cl::desc("是否把符号不同的BO/UO视为不同的"),
                                cl::init(false));
static cl::opt<bool> opNodeType("opnodetype",
                                cl::desc("是否把符号的Type也一并输出"),
                                cl::init(false));
static cl::opt<bool> arrayStmtType("arrtype",
                                   cl::desc("是否把ArraySubStmt的Type也一并输出"),
                                   cl::init(false));
static cl::opt<bool> noLiteral("noliteral",
                               cl::desc("不考虑常量的节点"),
                               cl::init(false));
static cl::opt<bool> noDecl("nodecl", cl::desc("不考虑Decl的节点"),
                            cl::init(false));
static cl::opt<bool> noDeclStmt("nodeclstmt", cl::desc("不考虑DeclStmt的节点"),
                                cl::init(false));
static cl::opt<bool> noArrayStmt("noarrayexpr", cl::desc("不考虑ArrayExpr的节点"),
                                 cl::init(false));
static cl::opt<float> threshold("threshold",
                                cl::desc("使用模型比较时候的阈值"),
                                cl::init(0.6f));

static cl::opt<bool> displayGlobalScoreUnit("dscoreg",
                                            cl::desc("展示全局分数度量元的详细信息"),
                                            cl::init(false));
static cl::opt<bool> displayAllScoreUnit("dscorea",
                                         cl::desc("展示所有分数度量元的详细信息"),
                                         cl::init(false));

// 三种排序的缓存map
std::map<std::string, std::string> *preOrderResult;
std::map<std::string, std::string> *inOrderResult;
std::map<std::string, std::string> *postOrderResult;
std::map<std::string, std::string> *treeStructResult;
std::map<std::string, int> *Countresult;

int g_totalFile = 0;
int g_currentFile = 0;

// 外部捕获报错
class MyDiagnosticConsumer : public clang::DiagnosticConsumer
{
public:
  void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                        const clang::Diagnostic &Info) override
  {
    g_fixedChecker.CatchDiagnostic(Info, DiagLevel);
  }
};

// AST Matcher的回调函数
class RootCompStmtCallback : public MatchFinder::MatchCallback
{
public:
  virtual void run(const MatchFinder::MatchResult &Result)
  {
    ASTContext *context = Result.Context;
    if (const CompoundStmt *CS = Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt"))
    {
      if (!CS->getBeginLoc().isValid())
      {
        return;
      }
      if (!(Result.SourceManager->isWrittenInMainFile(CS->getBeginLoc()) ||
            (!Result.SourceManager->isInSystemHeader(CS->getBeginLoc()))))
      {
        return;
      }

      g_currentASTContext = context;
      SourceManager &sm = context->getSourceManager();
      std::string filename = sm.getPresumedLoc(CS->getBeginLoc()).getFilename();
      int fid = g_fixedFileManager.GetOrAddFiexdFileID(filename);
      if (g_fileMask.count(fid))
      {
        return;
      }
      g_fileMaskCache.emplace(fid); // 加入缓存

      g_currentsm = &sm;
      const Stmt *stmt = CS;
      // 显示优化前AST
      if (ShowAST)
        DisplayStmt(stmt, 1);

      // 优化AST
      g_fixedSimplifier.m_doASTopt = Astopt;
      g_fixedSimplifier.Simplify(stmt);

      // 显示优化后AST
      if (ShowAST)
        DisplayStmt(stmt, 1);

      // simhash生成特征词
      if (!noSimHash)
      {
        std::vector<EigenWord *> new_words;
        GenStmtEigenWord(stmt, new_words, 1);
        if (fileBased || GenFile)
        {
          SavaFileEigenWords(new_words, filename);
        }
      }

      // 使用模型产生序列
      if (clonemodel)
      {
        GenStmtFingerASTSeq(stmt, 1);
      }

      // 统计AST结点信息
      if (nodeCount)
      {
        CountASTNode(stmt, *Countresult);
      }
    }
  }
};

ScoreUnitVector *globalUnit = nullptr;

// AST Matcher的回调函数
class UserCodeDeclCallback : public MatchFinder::MatchCallback
{
public:
  virtual void run(const MatchFinder::MatchResult &Result)
  {
    ASTContext *context = Result.Context;
    if (const Decl *D = Result.Nodes.getNodeAs<Decl>("decl"))
    {
      if (D->getLocation().isInvalid() || D->getBeginLoc().isInvalid())
      {
        return;
      }
      if (Result.SourceManager->isWrittenInMainFile(D->getLocation()) ||
          (!Result.SourceManager->isInSystemHeader(D->getLocation())))
      {
        g_currentASTContext = context;
        SourceManager &sm = context->getSourceManager();
        std::string filename =
            sm.getPresumedLoc(D->getBeginLoc()).getFilename();
        int fid = g_fixedFileManager.GetOrAddFiexdFileID(filename);
        if (g_fileMask.count(fid))
        {
          return;
        }
        g_fileMaskCache.emplace(fid); // 加入缓存

        g_currentsm = &sm;
        // 附加的错误筛查和分数单元
        g_fixedChecker.Check(D);
        g_fixedChecker.AddOffsetsToTotal(D);
      }
    }
  }
};

class CppAnaASTConsumer : public ASTConsumer
{
public:
  void HandleTranslationUnit(ASTContext &Context) override
  {
    ScoreController::g_scoreController.EnterGlobalScoreUnitVector(globalUnit);
    clang::SourceManager &SM = Context.getSourceManager();

    const clang::FileEntry *FE = SM.getFileEntryForID(SM.getMainFileID());
    std::string filename = utf8_to_gb2312(FE->getName().str());
    gInfoLog << "+开始分析文件: " << filename << "\n";
    EnterAnalyzeStep("分析文件:" + filename, g_totalFile, g_currentFile);
    g_currentFile++;
    g_currentDiagEngine = &Context.getDiagnostics();

    SourceManager &sm = Context.getSourceManager();
    // AST Matcher绑定
    StatementMatcher RootStmtMatcher =
        compoundStmt(unless(hasAncestor(compoundStmt().bind("compoundStmt"))))
            .bind("compoundStmt");
    DeclarationMatcher UserMatcher =
        decl(hasParent(translationUnitDecl()))
            .bind("decl");
    RootCompStmtCallback StmtCallback;
    UserCodeDeclCallback UserCallback;
    MatchFinder UserFinder;
    UserFinder.addMatcher(UserMatcher, &UserCallback);
    MatchFinder StmtFinder;
    StmtFinder.addMatcher(RootStmtMatcher, &StmtCallback);
    // 评分和度量元捕获
    if (!NoCheck)
    {
      UserFinder.matchAST(Context);
    }
    // AST优化
    StmtFinder.matchAST(Context);
    // 系统错误置入分数向量
    g_fixedChecker.MatchDiagnosticCollection(globalUnit, filename); // clang警告置入
    // 刷新缓存
    for (int fid : g_fileMaskCache)
    {
      g_fileMask.emplace(fid);
    }
    g_fileMaskCache.clear();

    gInfoLog << "-结束分析文件: " << filename << "\n";
  }
};

class CppAnaAction : public ASTFrontendAction
{
public:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override
  {
    return std::make_unique<CppAnaASTConsumer>();
  }
};

static cl::OptionCategory MyToolCategory("my-tool options");

void GetFilesInDir(std::vector<std::string> &Files)
{
  std::error_code EC;
  gInfoLog << "---------------获得分析文件列表-------------------\n";
  std::string path = Folder.getValue();
  gInfoLog << "路径: " << path << "\n";
  for (std::filesystem::recursive_directory_iterator F(path, EC), E;
       F != E; F.increment(EC))
  {
    auto Status = F->status();
    if (llvm::sys::path::extension(F->path().string()) == ".cpp" ||
        llvm::sys::path::extension(F->path().string()) == ".c")
    {
      gInfoLog << F->path().string() << "\n";
      std::string utf8_file = gb2312_to_utf8(F->path().string());
      Files.push_back(utf8_file);
    }
  }
  g_totalFile = Files.size();
  g_currentFile = 1;
  gInfoLog << "-------------------------------------------------\n";
}

void GenAndSendFileTable()
{
  FileTable filetable;
  gInfoLog << "---------------最终分析文件列表-------------------\n";
  for (auto ff : g_fixedFileManager.m_fileMap)
  {
    filetable.m_files.push_back(utf8_to_gb2312(ff.first));
    filetable.m_filesID.push_back(ff.second);
    gInfoLog << "[" << ff.second << "]" << ff.first << "\n";
  }
  gInfoLog << "--------------------------------------------------\n";
  auto jv = boost::json::value_from(filetable);
  g_eventClient->asyncWrite(serialize(jv));
}

int Execute(int argc, const char **argv)
{
  // 处理argv
  HighResolutionTimer timer;
  auto optionsParser = CommonOptionsParser::create(argc, argv, MyToolCategory);

  // 全局config
  g_opNodeDiff = opNodeDiff;
  g_opNodeType = opNodeType;
  g_noLiteral = noLiteral;
  g_noDecl = noDecl;
  g_noDeclStmt = noDeclStmt;
  g_noArraySExpr = noArrayStmt;
  g_arrType = arrayStmtType;

  g_displayFingerPrints = ShowDetailFG;
  g_fixedChecker.m_displayDiag = DisplayDiag;
  g_infoController.m_showSymInfo = ShowSyms;
  g_displaycsetInfo = ShowCsetInfo;
  g_threshold = threshold;
  g_showfg = ShowDetailFG;

  g_noLog = NoLog;

  // 进程间发送消息的client
  boost::asio::io_context io_context_e;
  g_eventClient = new EventClient(io_context_e, "localhost", std::to_string(Port));
  g_eventClient->InitHandler();
  io_context_e.run();

  // 连接到远程的部署了克隆模型的服务器
  boost::asio::io_context io_context_c;
  if (clonemodel)
  {
    g_cloneClient = new EventClient(io_context_c, cloneIP, cloneport);
    io_context_c.run();
  }

  // 通知开始分析
  EnterAnalyzeStep("Begin", 0, 0);

  // 初始化分数和重复度config
  InitScoreUnitConfigs();
  SimHashConfig::g_simhashConfig->InitAll();

  // 初始化内存
  if (nodeCount)
    Countresult = new std::map<std::string, int>();

  // 定义要处理的文件列表
  std::vector<std::string> Files;
  GetFilesInDir(Files);

  std::vector<std::string> CommandLine;
  auto Database = std::make_shared<clang::tooling::FixedCompilationDatabase>(
      ".", CommandLine);

  // 构造ClangTool对象
  ClangTool tool(*Database, Files);
  tool.appendArgumentsAdjuster(
      getInsertArgumentAdjuster("-Wextra", ArgumentInsertPosition::END));
  tool.appendArgumentsAdjuster(
      getInsertArgumentAdjuster("-Wall", ArgumentInsertPosition::END));

  // 设置诊断信息处理函数
  MyDiagnosticConsumer myDiagnosticConsumer;
  tool.setDiagnosticConsumer(&myDiagnosticConsumer);

  // 分析源代码
  gInfoLog << "==========================================\n";
  gInfoLog << "Starting tool version: " << VERSION << "\n";
  int result = tool.run(newFrontendActionFactory<CppAnaAction>().get());
  gInfoLog << "Tool finished with result: " << result << "\n";
  gInfoLog << "==========================================\n";

  g_compareDiffFileFG = onlyDiffFile;
  g_compareSameFileFG = onlySameFile;

  // 重复度分析
  if (!noSimHash)
  {
    EnterAnalyzeStep("重复度分析", 0, 0);
    gInfoLog << "重复度分析====================>\n";
    SimHasher simhasher;
    if (fileBased)
    {
      GenAndCompareFingerPrints();
    }
    else if (GenFile)
    {
      GenCodeWordFileOnce();
    }
    else
    {
      simhasher.SimHash();
    }
    CloneNodeSetVcetor::g_clonenodesetvec.DisplayAndSend(ShowDetailClone);
  }

  // 使用模型的重复度分析,生成并发送序列
  if (clonemodel)
  {
    // 发送序列到服务器
    gInfoLog << "重复度分析(模型)====================>\n";
    SendSeqToServer();
    io_context_c.run();
    timer.start();
    // 线程从服务器接受信息，直到末尾为止
    std::string fingerprints = g_cloneClient->KeepReading('!');
    timer.end();
    gInfoLog << "服务器生成指纹耗时: " << timer.output() << " ms\n";
    ProcessFingerPrints(fingerprints);
    CloneNodeSetVcetor::g_clonenodesetvec.DisplayAndSend(ShowDetailClone);
  }
  // 评分
  if (!NoCheck)
  {
    // 检查命名规范
    EnterAnalyzeStep("检查命名规范", 0, 0);
    gInfoLog << "检查命名规范====================>\n";
    g_infoController.CheckAllSymbols();

    EnterAnalyzeStep("代码评分", 0, 0);
    gInfoLog << "代码评分====================>\n";
    if (displayAllScoreUnit)
    {
    }
    else if (displayGlobalScoreUnit)
    {
      globalUnit->Display();
    }
    globalUnit->m_charNumWeight = g_fixedChecker.m_totalchar;
    ScoreController::g_scoreController.m_globalUnits = globalUnit;
    ScoreController::g_scoreController.Display();

    // 信息结合和传递
    g_infoController.m_globalInfo.m_totalchar = g_fixedChecker.m_totalchar;
    g_infoController.CalculateTotalScore();
    g_infoController.SendGlobalInfo();
    io_context_e.run();
  }

  // 发送文件表
  gInfoLog << "发送文件表====================>\n";
  GenAndSendFileTable();

  // 输出
  if (nodeCount)
    CountASTOutput(*Countresult);

  // 通知结束分析
  EnterAnalyzeStep("End", 0, 0);
  return result;
}

// 测试用main函数
int main(int argc, const char **argv)
{
  Execute(argc, argv);
  return 0;
}