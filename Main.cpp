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

// ����ת���ĺ���
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

// ����
static cl::opt<std::string> Folder("f", cl::desc("�����ļ���"), cl::Required);
static cl::opt<int> Port("port", cl::desc("ǰ�˶˿�"), cl::init(12345));
static cl::opt<bool> GenFile("genfile", cl::desc("����һ���ʹ����Ӧ���ļ�"),
                             cl::init(false));
static cl::opt<bool> onlyDiffFile("onlydiffF", cl::desc("������ļ��ڲ�������"),
                                  cl::init(false));
static cl::opt<bool> onlySameFile("onlysameF", cl::desc("ֻ����ļ��ڲ�������"),
                                  cl::init(false));
static cl::opt<bool> Astopt("astopt", cl::desc("�Ż�AST"), cl::init(false));
static cl::opt<bool> DisplayDiag("catchdiag", cl::desc("�ⲿ���񱨴�"), cl::init(false));
static cl::opt<bool> ShowDetailClone("detailclone", cl::desc("չʾ��ϸ�ظ���Ϣ"), cl::init(false));
static cl::opt<bool> ShowDetailFG("detailfg",
                                  cl::desc("չʾÿ��ָ�Ƶ���Ϣ"),
                                  cl::init(false));
static cl::opt<bool> ShowAST("showast",
                             cl::desc("չʾ�Ż�ǰ���Ż����AST"),
                             cl::init(false));
static cl::opt<bool> ShowCsetInfo("csetinfo", cl::desc("չʾ��¡���ϵ���ϸ��Ϣ"),
                                  cl::init(false));
static cl::opt<bool> NoLog("nolog", cl::desc("��log���"),
                           cl::init(false));
static cl::opt<bool> NoCheck("nocheck", cl::desc("�����д������"),
                             cl::init(false));
static cl::opt<bool> ShowSyms("showsym", cl::desc("��ʾsym��ʽ�淶ͳ����Ϣ"),
                              cl::init(false));
static cl::opt<bool> fileBased("filebased", cl::desc("һ���ļ�����һ��ָ��"),
                               cl::init(false));

static cl::opt<bool> clonemodel("clonemodel", cl::desc("ʹ�ÿ�¡����ģ��"),
                                cl::init(false));
static cl::opt<std::string> cloneIP("cloneIP", cl::desc("��¡ģ�ͷ����IP"), cl::Required);
static cl::opt<std::string> cloneport("cloneport", cl::desc("��¡ģ�ͷ���Ķ˿�"), cl::Required);

static cl::opt<bool> nodeCount("nodeCount", cl::desc("ͳ�ƽ�������Լ���Ӧ������"),
                               cl::init(false));
static cl::opt<bool> noSimHash("nosimhash", cl::desc("�������ظ��ȼ���"),
                               cl::init(false));

static cl::opt<bool> opNodeDiff("opnodediff", cl::desc("�Ƿ�ѷ��Ų�ͬ��BO/UO��Ϊ��ͬ��"),
                                cl::init(false));
static cl::opt<bool> opNodeType("opnodetype",
                                cl::desc("�Ƿ�ѷ��ŵ�TypeҲһ�����"),
                                cl::init(false));
static cl::opt<bool> arrayStmtType("arrtype",
                                   cl::desc("�Ƿ��ArraySubStmt��TypeҲһ�����"),
                                   cl::init(false));
static cl::opt<bool> noLiteral("noliteral",
                               cl::desc("�����ǳ����Ľڵ�"),
                               cl::init(false));
static cl::opt<bool> noDecl("nodecl", cl::desc("������Decl�Ľڵ�"),
                            cl::init(false));
static cl::opt<bool> noDeclStmt("nodeclstmt", cl::desc("������DeclStmt�Ľڵ�"),
                                cl::init(false));
static cl::opt<bool> noArrayStmt("noarrayexpr", cl::desc("������ArrayExpr�Ľڵ�"),
                                 cl::init(false));
static cl::opt<float> threshold("threshold",
                                cl::desc("ʹ��ģ�ͱȽ�ʱ�����ֵ"),
                                cl::init(0.6f));

static cl::opt<bool> displayGlobalScoreUnit("dscoreg",
                                            cl::desc("չʾȫ�ַ�������Ԫ����ϸ��Ϣ"),
                                            cl::init(false));
static cl::opt<bool> displayAllScoreUnit("dscorea",
                                         cl::desc("չʾ���з�������Ԫ����ϸ��Ϣ"),
                                         cl::init(false));

// ��������Ļ���map
std::map<std::string, std::string> *preOrderResult;
std::map<std::string, std::string> *inOrderResult;
std::map<std::string, std::string> *postOrderResult;
std::map<std::string, std::string> *treeStructResult;
std::map<std::string, int> *Countresult;

int g_totalFile = 0;
int g_currentFile = 0;

// �ⲿ���񱨴�
class MyDiagnosticConsumer : public clang::DiagnosticConsumer
{
public:
  void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                        const clang::Diagnostic &Info) override
  {
    g_fixedChecker.CatchDiagnostic(Info, DiagLevel);
  }
};

// AST Matcher�Ļص�����
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
      g_fileMaskCache.emplace(fid); // ���뻺��

      g_currentsm = &sm;
      const Stmt *stmt = CS;
      // ��ʾ�Ż�ǰAST
      if (ShowAST)
        DisplayStmt(stmt, 1);

      // �Ż�AST
      g_fixedSimplifier.m_doASTopt = Astopt;
      g_fixedSimplifier.Simplify(stmt);

      // ��ʾ�Ż���AST
      if (ShowAST)
        DisplayStmt(stmt, 1);

      // simhash����������
      if (!noSimHash)
      {
        std::vector<EigenWord *> new_words;
        GenStmtEigenWord(stmt, new_words, 1);
        if (fileBased || GenFile)
        {
          SavaFileEigenWords(new_words, filename);
        }
      }

      // ʹ��ģ�Ͳ�������
      if (clonemodel)
      {
        GenStmtFingerASTSeq(stmt, 1);
      }

      // ͳ��AST�����Ϣ
      if (nodeCount)
      {
        CountASTNode(stmt, *Countresult);
      }
    }
  }
};

ScoreUnitVector *globalUnit = nullptr;

// AST Matcher�Ļص�����
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
        g_fileMaskCache.emplace(fid); // ���뻺��

        g_currentsm = &sm;
        // ���ӵĴ���ɸ��ͷ�����Ԫ
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
    gInfoLog << "+��ʼ�����ļ�: " << filename << "\n";
    EnterAnalyzeStep("�����ļ�:" + filename, g_totalFile, g_currentFile);
    g_currentFile++;
    g_currentDiagEngine = &Context.getDiagnostics();

    SourceManager &sm = Context.getSourceManager();
    // AST Matcher��
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
    // ���ֺͶ���Ԫ����
    if (!NoCheck)
    {
      UserFinder.matchAST(Context);
    }
    // AST�Ż�
    StmtFinder.matchAST(Context);
    // ϵͳ���������������
    g_fixedChecker.MatchDiagnosticCollection(globalUnit, filename); // clang��������
    // ˢ�»���
    for (int fid : g_fileMaskCache)
    {
      g_fileMask.emplace(fid);
    }
    g_fileMaskCache.clear();

    gInfoLog << "-���������ļ�: " << filename << "\n";
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
  gInfoLog << "---------------��÷����ļ��б�-------------------\n";
  std::string path = Folder.getValue();
  gInfoLog << "·��: " << path << "\n";
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
  gInfoLog << "---------------���շ����ļ��б�-------------------\n";
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
  // ����argv
  HighResolutionTimer timer;
  auto optionsParser = CommonOptionsParser::create(argc, argv, MyToolCategory);

  // ȫ��config
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

  // ���̼䷢����Ϣ��client
  boost::asio::io_context io_context_e;
  g_eventClient = new EventClient(io_context_e, "localhost", std::to_string(Port));
  g_eventClient->InitHandler();
  io_context_e.run();

  // ���ӵ�Զ�̵Ĳ����˿�¡ģ�͵ķ�����
  boost::asio::io_context io_context_c;
  if (clonemodel)
  {
    g_cloneClient = new EventClient(io_context_c, cloneIP, cloneport);
    io_context_c.run();
  }

  // ֪ͨ��ʼ����
  EnterAnalyzeStep("Begin", 0, 0);

  // ��ʼ���������ظ���config
  InitScoreUnitConfigs();
  SimHashConfig::g_simhashConfig->InitAll();

  // ��ʼ���ڴ�
  if (nodeCount)
    Countresult = new std::map<std::string, int>();

  // ����Ҫ������ļ��б�
  std::vector<std::string> Files;
  GetFilesInDir(Files);

  std::vector<std::string> CommandLine;
  auto Database = std::make_shared<clang::tooling::FixedCompilationDatabase>(
      ".", CommandLine);

  // ����ClangTool����
  ClangTool tool(*Database, Files);
  tool.appendArgumentsAdjuster(
      getInsertArgumentAdjuster("-Wextra", ArgumentInsertPosition::END));
  tool.appendArgumentsAdjuster(
      getInsertArgumentAdjuster("-Wall", ArgumentInsertPosition::END));

  // ���������Ϣ������
  MyDiagnosticConsumer myDiagnosticConsumer;
  tool.setDiagnosticConsumer(&myDiagnosticConsumer);

  // ����Դ����
  gInfoLog << "==========================================\n";
  gInfoLog << "Starting tool version: " << VERSION << "\n";
  int result = tool.run(newFrontendActionFactory<CppAnaAction>().get());
  gInfoLog << "Tool finished with result: " << result << "\n";
  gInfoLog << "==========================================\n";

  g_compareDiffFileFG = onlyDiffFile;
  g_compareSameFileFG = onlySameFile;

  // �ظ��ȷ���
  if (!noSimHash)
  {
    EnterAnalyzeStep("�ظ��ȷ���", 0, 0);
    gInfoLog << "�ظ��ȷ���====================>\n";
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

  // ʹ��ģ�͵��ظ��ȷ���,���ɲ���������
  if (clonemodel)
  {
    // �������е�������
    gInfoLog << "�ظ��ȷ���(ģ��)====================>\n";
    SendSeqToServer();
    io_context_c.run();
    timer.start();
    // �̴߳ӷ�����������Ϣ��ֱ��ĩβΪֹ
    std::string fingerprints = g_cloneClient->KeepReading('!');
    timer.end();
    gInfoLog << "����������ָ�ƺ�ʱ: " << timer.output() << " ms\n";
    ProcessFingerPrints(fingerprints);
    CloneNodeSetVcetor::g_clonenodesetvec.DisplayAndSend(ShowDetailClone);
  }
  // ����
  if (!NoCheck)
  {
    // ��������淶
    EnterAnalyzeStep("��������淶", 0, 0);
    gInfoLog << "��������淶====================>\n";
    g_infoController.CheckAllSymbols();

    EnterAnalyzeStep("��������", 0, 0);
    gInfoLog << "��������====================>\n";
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

    // ��Ϣ��Ϻʹ���
    g_infoController.m_globalInfo.m_totalchar = g_fixedChecker.m_totalchar;
    g_infoController.CalculateTotalScore();
    g_infoController.SendGlobalInfo();
    io_context_e.run();
  }

  // �����ļ���
  gInfoLog << "�����ļ���====================>\n";
  GenAndSendFileTable();

  // ���
  if (nodeCount)
    CountASTOutput(*Countresult);

  // ֪ͨ��������
  EnterAnalyzeStep("End", 0, 0);
  return result;
}

// ������main����
int main(int argc, const char **argv)
{
  Execute(argc, argv);
  return 0;
}