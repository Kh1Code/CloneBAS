#include "SimHasher.h"
#include "EventCenter.h"
#include<iomanip>
#include<iostream>


SimHasher SimHasher::g_simhasher;

void SimHasher::SimHash()
{
	this->SetAllFingerPrints();
	int cnt = g_allFingerPrints.size();
	Info("有" + std::to_string(cnt) + "个指纹");

	FingerPrint* temp1,* temp2;

	DisJointSet high_dis(cnt); //并高度相似的节点 {A,B}
	DisJointSet normal_dis(cnt); //并一般相似的节点 {A,C}
	//DisJointSet sim_dis(cnt); //并相似的节点 {A,B,C}  这玩意不要了...

	int high_sim = this->initDisJointSet(high_dis, FINGERPRINT_HIGH_DIF_BOUNARY); //获取高相似
	int normal_sim = this->initDisJointSet(normal_dis, FINGERPRINT_NORMAL_DIF_BOUNARY); //获取普通相似

	high_dis.initSets();
	normal_dis.initSets();
	//高度相似集合获取
	for (auto it : high_dis.getSets()) {
		CloneNodeSet* clone_set = new CloneNodeSet();
		for (int node_num : it.second) { //it.second里面是互相相似的节点序号的数组
			clone_set->insertNode(&g_allFingerPrints[node_num]->m_nodeInfo);
		}
		clone_set->sortSimnodes();
		CloneNodeSetVcetor::g_clonenodesetvec.addHighSimSet(clone_set);
	}
	//一般相似集合获取
	for (auto it : normal_dis.getSets()) {
		CloneNodeSet* clone_set = new CloneNodeSet();
		for (int node_num : it.second) { //it.second里面是互相相似的节点序号的数组
			clone_set->insertNode(&g_allFingerPrints[node_num]->m_nodeInfo);
		}
		clone_set->sortSimnodes();
		CloneNodeSetVcetor::g_clonenodesetvec.addNormalSimSet(clone_set);
	}

	//最终输出全篇相似度
	std::cout << "高相似度代码块数量: " << std::to_string(high_sim)
                    << std::endl;
    std::cout << "普通相似度代码块数量:" << std::to_string(normal_sim)
                    << std::endl;
}

void SimHasher::ReclaimMemory()
{
	DeleteAllEigenWord();
	DeleteAllFingerPrint();
}

void SimHasher::markSonToNotCmp(int fa1, int fa2)
{
	FingerPrint* temp1 = g_allFingerPrints[fa1];
	FingerPrint* temp2 = g_allFingerPrints[fa2];
	//处理temp1
	for (void* node : temp1->m_comp_son_nodes) { //遍历每个子节点
		for (int index = fa1 - 1; index > fa2; --index) {
			if (g_allFingerPrints[index]->m_nodeInfo.m_stmt == node) { //找到了子节点
				if (g_allFingerPrints[index]->to_be_cmp == false) { //之前已经被标记过，说明这个父节点被判断过了，直接跳过
					break;
				}
				else { //标记
					g_allFingerPrints[index]->to_be_cmp = false;
				}
			}
		}
	}

	//处理temp2
	for (void* node : temp2->m_comp_son_nodes) { //遍历每个子节点
		for (int index = fa2 - 1; index >= 0; --index) {
            if (g_allFingerPrints[index]->m_nodeInfo.m_stmt == node) { //找到了子节点
				if (g_allFingerPrints[index]->to_be_cmp == false) { //之前已经被标记过，说明这个父节点被判断过了，直接跳过
					break;
				}
				else { //标记
					g_allFingerPrints[index]->to_be_cmp = false;
				}
			}
		}
	}
}

bool SimHasher::checkToCompare(FingerPrint*& fp1, FingerPrint*& fp2) {
	//对于每一个指纹,不可以和自己的父节点指纹比较相似度,并且只和自己类型相同的指纹比较
	if (fp1->m_fptype != fp1->m_fptype) {
		return false;
	}
	//指纹token差值不能太大
    if (std::abs(fp1->m_token_num - fp2->m_token_num) > std::min(fp1->m_token_num, fp2->m_token_num) * 0.3) {
          return false;
     }

	for (const void *noeptr : fp1->m_fathernodes) {
		if (fp2->m_nodeInfo.m_stmt == noeptr) {
			return false;
		}
	}
	for (const void* noeptr : fp2->m_fathernodes) {
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

void SimHasher::SetAllFingerPrints()
{
	//std::vector<EigenWord*> eigen_words;
	//for (NodeInFile filenode : g_rootnodes) {
	//	//Info("正在获取" + filenode.filename + "的指纹");
	//	Node* node = filenode.node;
	//	eigen_words.reserve(24);
	//	node->GetEigenWord(eigen_words);
	//	eigen_words.clear();
	//}
}

bool SimHasher::checkCanAddToSet(int n, std::vector<int> &sim_set, int sigma) {
  //编号n对应的指纹与sim_set里【所有】编号对应的指纹间的海明距离都要不大于sigma才行
  auto fp1 = g_allFingerPrints[n];
  for (auto i : sim_set) {
    auto fp2 = g_allFingerPrints[i];
    if (GetHammingDis(fp1->m_fingerprint, fp2->m_fingerprint) > sigma)
      return false;
  }
  return true;
}

int SimHasher::initDisJointSet(DisJointSet &dis_set, int sigma) {
  int sim_cnt = 0;
  int cnt = g_allFingerPrints.size();
  FingerPrint *temp1;
  FingerPrint *temp2;
  for (int i = cnt - 1; i >= 0;
       --i) { //这里倒着遍历，是因为由于递归的栈特性，父节点一定在子节点之后
    //在每一轮内部的for循环中，要生成一个相似集合，且该集合中最大的指纹编号为ｉ

    if (dis_set.isInASet(
            i)) //如果i已经属于某个集合，那么对于后续的j，若j没有加进该集合，则一定不能加进去该集合(否则在前面的轮就已经加进去了),因此跳过
      continue;

    std::vector<int> same_set_with_i{
        i}; //如果i将构成一个集合，则一定会是新的集合
    for (int j = i - 1; j >= 0; --j) {
      if (dis_set.isInASet(
              j)) //如果j已经属于某个集合，则这个集合中一定有比i大的编号，则i和j必然并不到一起，否则的话i在前面的轮就已经并到这个集合里了
        continue;
      temp1 = g_allFingerPrints[i];
      temp2 = g_allFingerPrints[j];

      if (this->checkToCompare(temp1, temp2)) {
        if (temp1->to_be_cmp == false &&
            temp2->to_be_cmp == false) //两个节点都被标记为“不进行比较”，则跳过
          continue;

        // std::vector<int> same_set_with_i = dis_set.getNumsInSameSet(i);
        if (this->checkCanAddToSet(j, same_set_with_i, sigma)) {
          // CloneInfo cloneinfo(temp1->m_nodePtr, temp2->m_nodePtr,
          // thisdifbits);
          // EventCenter::g_eventCenter.EventTrigger(EventName::CloneCode,
          // cloneinfo);
          same_set_with_i.push_back(j);
          dis_set.join(i, j);
          sim_cnt++;
          //将两个节点的紧挨着的子节点标记为“不进行比较”
          markSonToNotCmp(i, j);
        }
      }
    }
  }
  return sim_cnt;
}


int SimHasher::GetHammingDis(std::bitset<VEC_LEN>& bs1, std::bitset<VEC_LEN>& bs2)
{
	return ((bs1)^(bs2)).count();
}

