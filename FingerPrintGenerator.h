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
  float* m_fingerprint;   //指纹
  StmtNodeInfo m_nodeInfo;                 //该指纹对应的结点
  std::vector<const void *> m_fathernodes; //该指纹父节点的指纹
  std::string m_preseq;
  std::string m_postseq;

  bool to_be_cmp = true; //该指纹是否要被比较
};

extern bool g_showfg;
extern float g_threshold;
extern EventClient *g_cloneClient;
extern std::vector<SeqFingerPrint *> g_allSeqFingerPrints;

//生成指纹的序列信息
extern void GenStmtFingerASTSeq(const Stmt *S, int depth);

//生成指定的一个序列
extern void GenSeqFingerPrint(const Stmt *S);

//发送序列到远程服务端
extern void SendSeqToServer();

//解析远程服务器返回的序列
extern void ProcessFingerPrints(std::string json);

//输出序列信息到文件
extern void WriteFGtoFile();