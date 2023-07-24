#include "EigenWord.h"
#include<vector>
#include "EventClient.h"
#define SEQ_FG_LEN 512

struct SeqFingerPrint {
  SeqFingerPrint() { 
      m_fingerprint = new float[SEQ_FG_LEN];
  }  
  ~SeqFingerPrint() {
    delete[] m_fingerprint;
    int size = m_fathernodes.size();
    if (size > 0) {
      m_fathernodes.clear();
      m_fathernodes.shrink_to_fit();
    }
  }
  float* m_fingerprint;   //ָ��
  StmtNodeInfo m_nodeInfo;                 //��ָ�ƶ�Ӧ�Ľ��
  std::vector<const void *> m_fathernodes; //��ָ�Ƹ��ڵ��ָ��
  std::string m_preseq;
  std::string m_postseq;

  bool to_be_cmp = true; //��ָ���Ƿ�Ҫ���Ƚ�
};

extern bool g_showfg;
extern float g_threshold;
extern EventClient *g_cloneClient;
extern std::vector<SeqFingerPrint *> g_allSeqFingerPrints;

//����ָ�Ƶ�������Ϣ
extern void GenStmtFingerASTSeq(const Stmt *S, int depth);

//����ָ����һ������
extern void GenSeqFingerPrint(const Stmt *S);

//�������е�Զ�̷����
extern void SendSeqToServer();

//����Զ�̷��������ص�����
extern void ProcessFingerPrints(std::string json);

//���������Ϣ���ļ�
extern void WriteFGtoFile();