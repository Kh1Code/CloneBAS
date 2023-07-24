#include "InfoStruct.h"
#include "EventCenter.h"
#include "ScoreController.h"

class InfoController
{
public:
  GlobalInfo m_globalInfo;
  void SendGlobalInfo();
  void CalculateTotalScore();
  void CheckAllSymbols();

  void TryTraceSymbol(const Decl *sym);

  std::vector<std::string> m_normalSymbols;   // ��ͨ����
  std::vector<std::string> m_functionSymbols; // ��������
  std::vector<std::string> m_classSymbols;    // ��¼����

  bool m_showSymInfo = false;

private:
  void CheckSymbols(std::vector<std::string> &syms, ValueType vtype);
};

extern InfoController g_infoController;