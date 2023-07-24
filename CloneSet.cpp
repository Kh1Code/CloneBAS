#include"CloneSet.h"
#include"EigenWordGenerate.h"
#include"EigenWord.h"
#include"EventClient.h"
#include"FixedChecker.h"

CloneNodeSetVcetor CloneNodeSetVcetor::g_clonenodesetvec;
bool g_displaycsetInfo = false;

bool cmp(void* node1, void* node2)
{
	//先按行数排序，行数相同就按列数
	/*Node* n1 = (Node*)node1;
	Node* n2 = (Node*)node2;
	if (n1->m_coord.m_line == n2->m_coord.m_line)
		return n1->m_coord.m_colum < n2->m_coord.m_colum;
	else
		return n1->m_coord.m_line < n2->m_coord.m_line;*/
        return true;
}

DisJointSet::DisJointSet(int size)
{
	this->pre.reserve(size);
	this->pre.assign(size, -1);
}

void DisJointSet::join(int n1, int n2)
{
	int root1 = find(n1);
	int root2 = find(n2);
	if (root1 == root2) //n1,n2在同一集合
		return;
	else
	{
		if (pre[root1] < pre[root2]) //按高度合并，pre[root]的绝对值表示root的高度
			pre[root2] = root1;
		else
		{
			if (pre[root1] == pre[root2])
				pre[root2]--;  //root2高度加1
			pre[root1] = root2;
		}
	}
}

int DisJointSet::find(int n)
{
	if (pre[n] < 0)
		return n;
	else
		return pre[n] = find(pre[n]);
}

void DisJointSet::initSets()
{
	for (int i = 0; i < pre.size(); ++i)
	{
		if (pre[i] == -1) //说明这个节点没在任意一个集合里面
			continue;
		else
		{
			int root = this->find(i);
			sets[root].push_back(i);
		}
	}
}
bool DisJointSet::isInASet(int n) { return !(pre[n] == -1); }

std::vector<int> DisJointSet::getNumsInSameSet(int n) {
  int root = find(n);
  std::vector<int> res;
  for (int i = 0; i < pre.size(); ++i) {
    if (find(i) == root) {
      res.push_back(i);
    }
  }
  return res;
}

std::map<int, std::vector<int>> DisJointSet::getSets()
{
	return this->sets;
}

void CloneNodeSet::insertNode(void *node) {
	this->m_simnodes.push_back(node);
}

void CloneNodeSet::sortSimnodes()
{
	//std::sort(this->m_simnodes.begin(), this->m_simnodes.end(), cmp);
}

std::vector<void *> CloneNodeSet::getSimNodes() {
	return this->m_simnodes; 
}

CloneNodeSetVcetor::~CloneNodeSetVcetor()
{
	this->Reset();
}

std::vector<CloneNodeSet*> CloneNodeSetVcetor::getHighSimSet()
{
	return this->m_highsimvec;
}

std::vector<CloneNodeSet*> CloneNodeSetVcetor::getNormalSimSet()
{
	return this->m_normalsimvec;
}

std::vector<CloneNodeSet*> CloneNodeSetVcetor::getSimSet()
{
	return this->m_simvec;
}

void CloneNodeSetVcetor::addHighSimSet(CloneNodeSet* &clone_set)
{
	this->m_highsimvec.push_back(clone_set);
}

void CloneNodeSetVcetor::addNormalSimSet(CloneNodeSet* &clone_set)
{
	this->m_normalsimvec.push_back(clone_set);
}

void CloneNodeSetVcetor::addSimSet(CloneNodeSet* &clone_set)
{
	this->m_simvec.push_back(clone_set);
}

void CloneNodeSetVcetor::Reset()
{
	for (auto setptr : m_highsimvec) {
		if (setptr != nullptr)
		{
			delete setptr;
			setptr = nullptr;
		}
	}
	m_highsimvec.clear();
	m_highsimvec.shrink_to_fit();

	for (auto setptr : m_normalsimvec) {
		if (setptr != nullptr)
		{
			delete setptr;
			setptr = nullptr;
		}
	}
	m_normalsimvec.clear();
	m_normalsimvec.shrink_to_fit();

	for (auto setptr : m_simvec) {
		if (setptr != nullptr)
		{
			delete setptr;
			setptr = nullptr;
		}
	}
	m_simvec.clear();
	m_simvec.shrink_to_fit();
}


std::ofstream* g_csetfile;
std::ofstream* g_csetposfile;
void CloneNodeSetVcetor::DisplayAndSend(bool display)
{
    std::map<int, int> nblockAmountMap;
	std::map<int, int> hblockAmountMap;
    if (g_displaycsetInfo) {
		g_csetfile = new std::ofstream("cset_info.txt");
        g_csetposfile = new std::ofstream("cset_pos_info.txt");
    }
	if (m_highsimvec.size() != 0) {
        if (display) {
          gInfoLog << "高度相似的节点集合如下: \n";
        }  
		for (int i = 0; i < m_highsimvec.size(); ++i) {
			if (display) {
              gInfoLog << "集合序号: " << std::to_string(i + 1) << "\n";
			}           
			if (g_displaycsetInfo) {
                *g_csetposfile << "【高度相似|序号:" << i + 1 << "】" << std::endl;
			}
            DisplayOneSet(m_highsimvec[i], display, 0, i + 1);
			int bsize = m_highsimvec[i]->getSimNodes().size();
            if (hblockAmountMap.count(bsize)) {
               hblockAmountMap[bsize]++;
            } else {
               hblockAmountMap.emplace(bsize, 1);
            }            
		}
	}
	if (m_normalsimvec.size() != 0) {
        if (display) {
          gInfoLog << "一般相似的节点集合如下: \n";
        }  
		for (int i = 0; i < m_normalsimvec.size(); ++i) {
            if (display) {
              gInfoLog << "集合序号: " << std::to_string(i + 1) << "\n";
			}  
			if (g_displaycsetInfo) {
                *g_csetposfile << "【一般相似|序号:" << i + 1 << "】" << std::endl;
			}
            DisplayOneSet(m_normalsimvec[i], display, 1, i + 1);
			int bsize = m_normalsimvec[i]->getSimNodes().size();
            if (nblockAmountMap.count(bsize)) {
               nblockAmountMap[bsize]++;
            } else {
               nblockAmountMap.emplace(bsize, 1);
            }        
		}
	}
    if (m_highsimvec.size() == 0 && m_normalsimvec.size() == 0) {
      return;
 	}
	m_allCSetInfo.m_avgLine = m_allCSetInfo.m_totalLine / m_allCSetInfo.m_blockNum;
    if (g_displaycsetInfo) {
      m_allCSetInfo.Display(0, -1, *g_csetfile);
      *g_csetfile << "普通重复代码块综合统计=>" << std::endl;
      for (auto ba : nblockAmountMap) {
        *g_csetfile << "代码块数: " << ba.first << " | 集合数: " << ba.second <<std::endl;
	  }
      *g_csetfile << "高度重复代码块综合统计=>" << std::endl;
      for (auto ba : hblockAmountMap) {
	    *g_csetfile << "代码块数: " << ba.first << " | 集合数: " << ba.second <<std::endl;
      }
	}
}

int CloneNodeSetVcetor::GetDiffFileCloneScore() {
  int score = 0;
  if (m_highsimvec.size() != 0) {
    for (int i = 0; i < m_highsimvec.size(); ++i) {
      score += GetDiffFileOneSetCloneScore(m_highsimvec[i]) * 2;
    }
  }
  if (m_normalsimvec.size() != 0) {
    for (int i = 0; i < m_normalsimvec.size(); ++i) {
      score += GetDiffFileOneSetCloneScore(m_normalsimvec[i]);
    }
  }
  return score;
}
int CloneNodeSetVcetor::GetDiffFileOneSetCloneScore(CloneNodeSet *cloneset) {
  int toknum = 0;
  const char *filename = nullptr;
  std::vector<void *> sim_node_vec = cloneset->getSimNodes();
  for (int j = 0; j < sim_node_vec.size(); ++j) {
    const Stmt *node = ((StmtNodeInfo *)sim_node_vec[j])->m_stmt;
    toknum += ((StmtNodeInfo *)sim_node_vec[j])->m_tokNums;
  }
  toknum /= sim_node_vec.size();
  return toknum;
}
void CloneNodeSetVcetor::DisplayOneSet(CloneNodeSet *cloneset, bool display, int sim, int index) {
  //重复集合在这里
  CloneSetInfo clonesetInfo;
  clonesetInfo.m_degree = sim;

  std::vector<void *> sim_node_vec = cloneset->getSimNodes();
  CloneSetSInfo sinfo;
  for (int j = 0; j < sim_node_vec.size(); ++j) {
    const Stmt *node = ((StmtNodeInfo*)sim_node_vec[j])->m_stmt;
    if (((StmtNodeInfo *)sim_node_vec[j])->m_useSM) {
	  //内部结点的处理
      PresumedLoc &beginlocInfo = ((StmtNodeInfo *)sim_node_vec[j])->m_beginlocInfo;
      PresumedLoc &endlocInfo = ((StmtNodeInfo *)sim_node_vec[j])->m_endlocInfo;
      std::string file = beginlocInfo.getFilename();
      int fid = g_fixedFileManager.GetOrAddFiexdFileID(file);
      int bline = beginlocInfo.getLine();
      int bcolumn = beginlocInfo.getColumn();
      int eline = endlocInfo.getLine();
      int ecolumn = endlocInfo.getColumn();
      if (display) {
        gInfoLog << "【Start Loc: " << file << "- 行号:" << bline
                     << "- 列号" << bcolumn << "\n| End Loc: " << file
                     << "- 行号:" << eline << "- 列号" << ecolumn
                     << "\n| Token Nums: "
                     << ((StmtNodeInfo *)sim_node_vec[j])->m_tokNums
                     << "】\n"; // 打印信息   
	  }
	  if (g_displaycsetInfo) {
			//打印信息到文件
		  *g_csetposfile << "File: " << file << " |开始行号:" << bline << "|开始列号" << bcolumn << "|结束行号:" << eline << "|结束列号" << ecolumn
                     << "|Char Nums: " << ((StmtNodeInfo *)sim_node_vec[j])->m_tokNums << "\n"; // 打印信息
		} 
	  //计算这个集合的统计信息
      int mline = eline - bline + 1;
      sinfo.m_totalLine += mline;
      if (mline > sinfo.m_maxLine) {
        sinfo.m_maxLine = mline;
	  }
      if (mline < sinfo.m_minLine) {
        sinfo.m_minLine = mline;
	  }
	  //记录重复集合的一个位置
      clonesetInfo.m_cloneSets.push_back(
              ClonePosition(fid, bline, bcolumn, eline, ecolumn));
    } else {
	  //自定义节点的输出
    }
  }
  //集合统计信息的总和数据
  sinfo.m_blockNum = sim_node_vec.size();
  sinfo.m_avgLine = sinfo.m_totalLine / sinfo.m_blockNum;
  if (g_displaycsetInfo) {
    sinfo.Display(index, sim, *g_csetfile);
  }
  //集成到整体信息里
  if (sinfo.m_maxLine > m_allCSetInfo.m_maxLine) {
    m_allCSetInfo.m_maxLine = sinfo.m_maxLine;
  }
  if (sinfo.m_minLine < m_allCSetInfo.m_minLine) {
    m_allCSetInfo.m_minLine = sinfo.m_minLine;
  }
  m_allCSetInfo.m_totalLine += sinfo.m_totalLine;
  m_allCSetInfo.m_blockNum += sinfo.m_blockNum;

  //发送至前端
  auto jv = boost::json::value_from(clonesetInfo);
  auto str = serialize(jv);
  g_eventClient->asyncWrite(str);
}


