#include "EigenWord.h"

bool g_compareDiffFileFG = false; //是否只匹配不同文件之间的结点
bool g_compareSameFileFG = false; //是否只匹配相同文件之间的结点
SourceManager *g_currentsm = nullptr;
ASTContext *g_currentASTContext = nullptr;

std::ofstream ffile("fingerprints.txt");
bool g_displayFingerPrints = false;

//存放所有代码块各自的指纹
std::vector<FingerPrint*> g_allFingerPrints;

//存放所有特征词，用于回收内存
std::vector<EigenWord*> g_allEigenword;

std::bitset<VEC_LEN> EigenWord::GetEigenWordHashBits() {
    uint64_t out[2];
    const char* str = this->m_word.c_str();
    MurmurHash3_x64_128(str, strlen(str), SEED, &out);
    std::bitset<VEC_LEN / 2> out1(out[0]);
    std::bitset<VEC_LEN / 2> out2(out[1]);
    std::string outstr = out1.to_string() + out2.to_string();
    std::bitset<VEC_LEN> bits(outstr.c_str());
    return bits;
}

void EigenWord::SetWeight(std::string &nodename) {
#define SIMHASH_CONFIG SimHashConfig::g_simhashConfig
  if (SIMHASH_CONFIG->weight_table.find(nodename) !=
      SIMHASH_CONFIG->weight_table.end()) {
    this->m_weight = SIMHASH_CONFIG->weight_table[nodename];
  } else {
    this->m_weight = SIMHASH_CONFIG->weight_table["DEFAULT"];
  }
#undef SIMHASH_CONFIG
}

void EigenWord::SetWeightVec() {
    std::bitset<VEC_LEN> bits = this->GetEigenWordHashBits();
    for (int i = 0; i < VEC_LEN; ++i) {
        this->m_weightVec[VEC_LEN - 1 - i] = (bits[i] == 0 ? - this->m_weight : this->m_weight);
    }
}

FingerPrint* GetFingerPrint(std::vector<EigenWord*>& eigen_wordvec, FingerPrint::FP_Type fptype, int token_num) {
    std::string display;
    FingerPrint* out_fingerprint = new FingerPrint();
    std::vector<int> sum_vec(VEC_LEN); //求和的结果
    for (EigenWord* eword : eigen_wordvec) {
        display = display + "[" + eword->m_word + "|" +  std::to_string(eword->m_weight) + "]";
        for (int i = 0; i < VEC_LEN; ++i) {
            sum_vec[i] += eword->m_weightVec[i];
        }
    }
    display += "\n【";
    for (int i = 0; i < VEC_LEN; ++i) {
      if (sum_vec[i] <= 1) {
        (out_fingerprint->m_fingerprint)[i] = 0;
        display += "0";
      } else {
        (out_fingerprint->m_fingerprint)[i] = 1;
        display += "1";
      }
    }
    display += "|eigen: " + std::to_string(eigen_wordvec.size()) + "|offsets: " + std::to_string(token_num) + "】";
    if (g_displayFingerPrints) {
      ffile << display << "\n\n"; 
    }
    out_fingerprint->m_fptype = fptype;
    out_fingerprint->m_token_num = token_num;
    out_fingerprint->m_nodeInfo.m_tokNums = token_num;
    return out_fingerprint;
}

void AddFingerPrint(FingerPrint*& fingerprient) {
    g_allFingerPrints.push_back(fingerprient);
}

void AddEigenWord(std::string word, std::string& node_name,
                  std::vector<EigenWord *> &ewordvec) {
    EigenWord *eword = new EigenWord(word, node_name);
    ewordvec.push_back(eword);
    g_allEigenword.push_back(eword);
}

void DeleteAllEigenWord()
{
    for (EigenWord* eword : g_allEigenword) {
        if (eword != nullptr) {
            delete eword;
            eword = nullptr;
        }
    }
    g_allEigenword.clear();
    g_allEigenword.shrink_to_fit();
}

void DeleteAllFingerPrint()
{
    for (FingerPrint* fingerprint : g_allFingerPrints) {
        if (fingerprint != nullptr) {
            delete fingerprint;
            fingerprint = nullptr;
        }
    }
    g_allFingerPrints.clear();
    g_allFingerPrints.shrink_to_fit();
}

#include"FixedNode.h"

void StmtNodeInfo::Init(const Stmt *node, SourceManager *SM) {
  m_stmt = node;
  if (SM != nullptr) {
    if (node->getStmtClass() == Stmt::FixedStmtClass) {
      const FixedParentStmt *fixed_node =
          (FixedParentStmt *)llvm::dyn_cast<FixedStmt>(node);
      m_beginlocInfo = SM->getPresumedLoc(fixed_node->getBeginLoc());
      m_endlocInfo = SM->getPresumedLoc(fixed_node->getEndLoc());
    } else {
      m_beginlocInfo = SM->getPresumedLoc(node->getBeginLoc());
      m_endlocInfo = SM->getPresumedLoc(node->getEndLoc());
    }
    m_useSM = true;
  }
}