#include"FingerPrintGenerator.h"
#include "FixedNode.h"
#include "NodeTravesal.h"
#include <cmath>

EventClient *g_cloneClient;
std::vector<SeqFingerPrint *> g_allSeqFingerPrints;
float g_threshold = 0.6f;
bool g_showfg = false;

//�����Ҫ����ǩ���ĸ��ڵ�ջ��������ָ��֪���Լ��ĸ��ڵ��ǣ�
std::vector<const Stmt *> g_fingerprint_fnodestack;

void GenStmtFingerASTSeq(const Stmt* S, int depth) {
  g_fingerprint_fnodestack.push_back(S);
  FixedParentStmt *fixed_node = nullptr;
  const FixedStmt *fixed_com_node = llvm::dyn_cast<FixedStmt>(S);
  switch (S->getStmtClass()) {
  case Stmt::FixedStmtClass:
    //��ʱֻ����ȫAST�Ż��������
    fixed_node = (FixedParentStmt *)const_cast<FixedStmt *>(fixed_com_node);
    int cnum = fixed_node->GetCharNum();
    if (fixed_node->m_fixedType ==
        FixedParentStmt::FixedType::FixedCompoundStmtClass) {
      if (cnum > FIXEDCOMPSTMT_TOKEN_NUM_BOUNDARY)
        GenSeqFingerPrint(S);
    }
    if (fixed_node->m_fixedType ==
        FixedParentStmt::FixedType::FixedLoopStmtClass) {
      if (cnum > FIXEDLOOPSTMT_TOKEN_NUM_BOUNDARY)
        GenSeqFingerPrint(S);
    }
    if (fixed_node->m_fixedType ==
        FixedParentStmt::FixedType::FixedSelectionStmtClass) {
      if (cnum > FIXEDSELECTSTMT_TOKEN_NUM_BOUNDARY)
        GenSeqFingerPrint(S);
    }
  }
  for (Stmt::const_child_iterator I = S->child_begin(); I != S->child_end();
       ++I) {
    if (*I != nullptr) {
      GenStmtFingerASTSeq(*I, depth + 1);
    }
  }
  g_fingerprint_fnodestack.pop_back();
}

void GenSeqFingerPrint(const Stmt *S){
  SeqFingerPrint *out_fingerprint = new SeqFingerPrint();
  TravesalAST(S, out_fingerprint->m_preseq, 0);
  TravesalAST(S, out_fingerprint->m_postseq, 2);
  out_fingerprint->m_nodeInfo.Init(S, g_currentsm);
  for (const Stmt *node : g_fingerprintnodestack) {
    out_fingerprint->m_fathernodes.push_back(node);
  }
  g_allSeqFingerPrints.push_back(out_fingerprint);
}
void SendSeqToServer() {
  boost::json::object val;
  boost::json::object dataset;
  boost::json::array pre_array;
  boost::json::array post_array;
  for (SeqFingerPrint* sfg : g_allSeqFingerPrints) {
    pre_array.emplace_back(sfg->m_preseq);
    post_array.emplace_back(sfg->m_postseq);
  }
  dataset["Source_pre"] = pre_array;
  dataset["Source_post"] = post_array;
  val["dataset"] = dataset;
  gInfoLog << "�ܹ����: " << g_allSeqFingerPrints.size() << " ��������\n"; 
  if (g_allSeqFingerPrints.size() > 0) {
    g_cloneClient->asyncWrite(serialize(val) + "!");
  }
}

//���º����������Ǵ�simhash�Ǳ߸��ƹ�����
int initDisJointSet(DisJointSet &dis_set, float threshold);
bool checkToCompare(SeqFingerPrint *&fp1, SeqFingerPrint *&fp2);
float getCosSim(float *vec1, float *vec2, int len);
bool checkCanAddToSet(int n, std::vector<int> &sim_set, float threshold);

void ProcessFingerPrints(std::string json) { 
    json.erase(json.end() - 1);

    //����json
    boost::json::value jsonValue = boost::json::parse(json);
    auto &jo = jsonValue.as_object();
    boost::json::array fgs_array = jo.at("Represent").as_array();
    int ind = 0;
    gInfoLog << "�ܹ����յ�" << fgs_array.size() << " ָ��\n";
    for (auto fg : fgs_array) {
      boost::json::array fga = fg.as_array();
      int i = 0;
      float *fgvec = g_allSeqFingerPrints[ind]->m_fingerprint;
      for (const auto &value : fga) {
        fgvec[i] = value.as_double();
        i++;
      }
      ind++;
    }

    if (g_showfg) {
      WriteFGtoFile();
    }
    //ִ���ظ��ȱȽϺͼ�������
    int cnt = g_allSeqFingerPrints.size();
    Info("��" + std::to_string(cnt) + "��ָ��");
    HighResolutionTimer timer;
    timer.start();

    FingerPrint *temp1, *temp2;

    DisJointSet high_dis(cnt);   //���߶����ƵĽڵ� {A,B}

    int high_sim = initDisJointSet(high_dis, g_threshold); //��ȡ������

    high_dis.initSets();
    //�߶����Ƽ��ϻ�ȡ
    for (auto it : high_dis.getSets()) {
      CloneNodeSet *clone_set = new CloneNodeSet();
      for (int node_num : it.second) { // it.second�����ǻ������ƵĽڵ���ŵ�����
        clone_set->insertNode(&g_allSeqFingerPrints[node_num]->m_nodeInfo);
      }
      clone_set->sortSimnodes();
      CloneNodeSetVcetor::g_clonenodesetvec.addHighSimSet(clone_set);
    }

    timer.end();
    gInfoLog << "���ƴ��������: " << std::to_string(high_sim)
             << "\n�ܹ���ʱ: " << timer.output() << " ms\n";
}

void WriteFGtoFile() {
  std::ofstream file("modelfg.txt");
  for (SeqFingerPrint *sfg : g_allSeqFingerPrints) {
    file << "PREORDER: " << sfg->m_preseq << "\n";
    file << "POSTORDER: " << sfg->m_postseq << "\n";
    file << "FINGERPRINT:[ ";
    for (int i = 0; i < SEQ_FG_LEN; i++) {
      file << sfg->m_fingerprint[i] << " ";
    }
    file << "] \n";
    PresumedLoc &beginlocInfo = sfg->m_nodeInfo.m_beginlocInfo;
    PresumedLoc &endlocInfo = sfg->m_nodeInfo.m_endlocInfo;
    std::string filen = beginlocInfo.getFilename();
    int bline = beginlocInfo.getLine();
    int bcolumn = beginlocInfo.getColumn();
    int eline = endlocInfo.getLine();
    int ecolumn = endlocInfo.getColumn();
    file << "POS: "
         << "��Start Loc: " << filen << "- �к�:" << bline << "- �к�" << bcolumn
         << "| End Loc: " << filen << "- �к�:" << eline << "- �к�" << ecolumn
         << "��\n"; 
  }
}

int initDisJointSet(DisJointSet &dis_set, float threshold) {
  int sim_cnt = 0;
  int cnt = g_allSeqFingerPrints.size();
  SeqFingerPrint *temp1;
  SeqFingerPrint *temp2;
  for (int i = cnt - 1; i >= 0;
       --i) { //���ﵹ�ű���������Ϊ���ڵݹ��ջ���ԣ����ڵ�һ�����ӽڵ�֮��
    //��ÿһ���ڲ���forѭ���У�Ҫ����һ�����Ƽ��ϣ��Ҹü���������ָ�Ʊ��Ϊ��

    if (dis_set.isInASet(
            i)) //���i�Ѿ�����ĳ�����ϣ���ô���ں�����j����jû�мӽ��ü��ϣ���һ�����ܼӽ�ȥ�ü���(������ǰ����־��Ѿ��ӽ�ȥ��),�������
      continue;

    std::vector<int> same_set_with_i{
        i}; //���i������һ�����ϣ���һ�������µļ���
    for (int j = i - 1; j >= 0; --j) {
      if (dis_set.isInASet(
              j)) //���j�Ѿ�����ĳ�����ϣ������������һ���б�i��ı�ţ���i��j��Ȼ������һ�𣬷���Ļ�i��ǰ����־��Ѿ����������������
        continue;
      temp1 = g_allSeqFingerPrints[i];
      temp2 = g_allSeqFingerPrints[j];

      if (checkToCompare(temp1, temp2)) {
        if (temp1->to_be_cmp == false &&
            temp2->to_be_cmp == false) //�����ڵ㶼�����Ϊ�������бȽϡ���������
          continue;

        // std::vector<int> same_set_with_i = dis_set.getNumsInSameSet(i);
        if (checkCanAddToSet(j, same_set_with_i, threshold)) {
          // CloneInfo cloneinfo(temp1->m_nodePtr, temp2->m_nodePtr,
          // thisdifbits);
          // EventCenter::g_eventCenter.EventTrigger(EventName::CloneCode,
          // cloneinfo);
          same_set_with_i.push_back(j);
          dis_set.join(i, j);
          sim_cnt++;
        }
      }
    }
  }
  return sim_cnt;
}

bool checkToCompare(SeqFingerPrint *&fp1, SeqFingerPrint *&fp2) {
    for (const void *noeptr : fp1->m_fathernodes) {
      if (fp2->m_nodeInfo.m_stmt == noeptr) {
        return false;
      }
    }
    for (const void *noeptr : fp2->m_fathernodes) {
      if (fp1->m_nodeInfo.m_stmt == noeptr) {
        return false;
      }
    }
    if (g_compareDiffFileFG) {
      if (fp1->m_nodeInfo.m_beginlocInfo.getFilename() ==
          fp2->m_nodeInfo.m_beginlocInfo.getFilename()) {
        return false;
      }
    }
    if (g_compareSameFileFG) {
      if (fp1->m_nodeInfo.m_beginlocInfo.getFilename() !=
          fp2->m_nodeInfo.m_beginlocInfo.getFilename()) {
        return false;
      }
    }
    return true;
  }

bool checkCanAddToSet(int n, std::vector<int> &sim_set,
                                   float threshold) {
    //���n��Ӧ��ָ����sim_set����С���Ŷ�Ӧ��ָ�Ƽ�ĺ������붼Ҫ������sigma����
    //gInfoLog << "checkCanAddToSet " << "\n";
    auto fp1 = g_allSeqFingerPrints[n];
    for (auto i : sim_set) {
      auto fp2 = g_allSeqFingerPrints[i];
      float cossim = getCosSim(fp1->m_fingerprint, fp2->m_fingerprint, SEQ_FG_LEN) * 0.5f + 0.5f;
      //gInfoLog << cossim << " " << threshold << "\n";
      if (cossim < threshold) {
        return false;
      }
    }
    return true;
  }

float getCosSim(float *vec1, float *vec2, int len) {
    //�Ƚ������������������ƶ�
  float dotProduct = 0.0;
  float norm1 = 0.0;
  float norm2 = 0.0;

  for (std::size_t i = 0; i < len; i++) {
    dotProduct += vec1[i] * vec2[i];
    norm1 += vec1[i] * vec1[i];
    norm2 += vec2[i] * vec2[i];
  }

  norm1 = std::sqrt(norm1);
  norm2 = std::sqrt(norm2);

  if (norm1 == 0.0 || norm2 == 0.0) {
    return 0.0;
  }

  return dotProduct / (norm1 * norm2);
}