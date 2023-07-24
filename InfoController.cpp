#include "InfoController.h"
#include "EventClient.h"

InfoController g_infoController;

void InfoController::SendGlobalInfo()
{
  EventCenter::g_eventCenter.EventTrigger(EventName::GlobalInfo, m_globalInfo);
}

void InfoController::CalculateTotalScore()
{
  int t = ScoreController::g_scoreController.CalculateTotalScore();
  int n = ScoreController::g_scoreController.CalculateTotalScore(
      ScoreUnit::ScoreUnitType::NORMATIVE);
  int e = ScoreController::g_scoreController.CalculateTotalScore(
      ScoreUnit::ScoreUnitType::EFFICIENCY);
  int m = ScoreController::g_scoreController.CalculateTotalScore(
      ScoreUnit::ScoreUnitType::MAINTAINABILITY);
  int s = ScoreController::g_scoreController.CalculateTotalScore(
      ScoreUnit::ScoreUnitType::SECURITY);
  // �ĸ�ά�ȵķ���(NORMATIVE/*�淶��N*/  EFFICIENCY/*ִ��Ч��E*/,
  // MAINTAINABILITY/*��ά����M*/, SECURITY/*��ȫ��(�������)S*/)+�ܷ�()
  m_globalInfo.m_scores[0] = n;
  m_globalInfo.m_scores[1] = e;
  m_globalInfo.m_scores[2] = m;
  m_globalInfo.m_scores[3] = s;
  m_globalInfo.m_scores[4] = t;
}

void InfoController::CheckAllSymbols()
{
  CheckSymbols(m_normalSymbols, ValueType::VAR);
  CheckSymbols(m_functionSymbols, ValueType::FUNCTION);
  CheckSymbols(m_classSymbols, ValueType::RECORD);
}

void InfoController::TryTraceSymbol(const Decl *sym)
{
  if (sym->isImplicit())
  {
    return;
  }
  if (const VarDecl *vardecl = llvm::dyn_cast<VarDecl>(sym))
  {
    // ��ͨ����(ȫ��/�ֲ�)
    m_normalSymbols.push_back(vardecl->getNameAsString());
    m_globalInfo.m_noramlsymbol++;
  }
  else if (const FieldDecl *fielddecl = llvm::dyn_cast<FieldDecl>(sym))
  {
    // ��ͨ����(��Ա-record����)
    m_normalSymbols.push_back(fielddecl->getNameAsString());
    m_globalInfo.m_noramlsymbol++;
  }
  else if (const FunctionDecl *funcdecl = llvm::dyn_cast<FunctionDecl>(sym))
  {
    if (funcdecl->hasBody() && funcdecl->isThisDeclarationADefinition())
    {
      // ����һ����������(��¼����)
      m_functionSymbols.push_back(funcdecl->getNameAsString());
      m_globalInfo.m_functionsymbol++;
    }
  }
  else if (const CXXRecordDecl *recorddecl = llvm::dyn_cast<CXXRecordDecl>(sym))
  {
    if (recorddecl->hasDefinition())
    {
      // ����һ���ඨ��(��¼����)
      m_classSymbols.push_back(recorddecl->getNameAsString());
      m_globalInfo.m_classsymbol++;
    }
  }
}

void InfoController::CheckSymbols(std::vector<std::string> &syms,
                                  ValueType vtype)
{
}
