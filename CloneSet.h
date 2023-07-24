#pragma once
//各种信息(如果有必要的话)的集合

#include<set>
#include<vector>
#include<map>
#include<string>
#include<fstream>
#define HIGH_SIM 0  //高相似度
#define NORMAL_SIM 1  //一般相似度

//比较两个节点的顺序，用于排序
bool cmp(void* node1, void* node2);

extern bool g_displaycsetInfo;
//记录一个重复集合的综合信息
struct CloneSetSInfo {
  int m_maxLine = -1;
  int m_avgLine = 0;
  int m_minLine = 999999;
  int m_totalLine = 0;
  int m_blockNum = 0;
  void Display(int index, int sim, std::ofstream& file) { 
	  if (sim == 0) {
         file << "高度重复集合[" << index << "]" << std::endl;
	  }
      else if(sim == 1) {
         file << "普通重复集合[" << index << "]" << std::endl;
	  }
	  else {
         file << "整体数据" << std::endl;
	  }
      file << "最大行数: " << m_maxLine << std::endl;
      file << "最小行数: " << m_minLine << std::endl;
      file << "总行数: " << m_totalLine << std::endl;
      file << "平均行数: " << m_avgLine << std::endl;
      file << "总代码块数: " << m_blockNum << std::endl;
      file << std::endl;  
  }
};

//并查集
class DisJointSet
{
public:
	DisJointSet(int size); //初始化pre数组大小,并初始化pre所有元素为-1
	void join(int n1, int n2); //合并
	int find(int n); //查找根节点
	void initSets(); //初始化sets
	bool isInASet(int n); //n是否已经被分到了一个集合里
	std::vector<int> getNumsInSameSet(int n); //获取与n在一个集合里的数字
	std::map<int, std::vector<int>> getSets(); //返回sets
private:
	std::vector<int> pre; //pre[i]表示i号节点的前驱，若pre[i]<0，表示该节点为根
	std::map<int, std::vector<int>> sets;  //sets[i]表示与第i号节点在同一集合里面的所有节点
};

//重复节点集合,一个集合里存放互相相似的节点，如{A,B,C}
class CloneNodeSet
{
public:
    void insertNode(void *node);       //向m_simnodes插入节点
	void sortSimnodes(); //将m_simnodes排序
    std::vector<void *> getSimNodes(); //返回m_simnodes
  private:
    std::vector<void *> m_simnodes; //互相相似的节点(Node*)
};

//存放重复节点集合的向量，如{ {A,B,C}, {D,E} }
class CloneNodeSetVcetor
{
public:
	~CloneNodeSetVcetor();
	static CloneNodeSetVcetor g_clonenodesetvec;
	std::vector<CloneNodeSet*> getHighSimSet(); //返回m_highsimvec
	std::vector<CloneNodeSet*> getNormalSimSet(); //m_normalsimvec
	std::vector<CloneNodeSet*> getSimSet(); //m_simvec
	void addHighSimSet(CloneNodeSet* &clone_set); //向m_highsimvec添加集合
	void addNormalSimSet(CloneNodeSet* &clone_set);
	void addSimSet(CloneNodeSet* &clone_set);
	void Reset();
    void DisplayAndSend(bool display);
	//测试函数
    int GetDiffFileCloneScore();
private:
    void DisplayOneSet(CloneNodeSet *cloneset, bool display, int sim, int index);
    int GetDiffFileOneSetCloneScore(CloneNodeSet *cloneset);
	std::vector<CloneNodeSet*> m_highsimvec;  //高度相似的集合
	std::vector<CloneNodeSet*> m_normalsimvec;  //一般相似的集合
	std::vector<CloneNodeSet*> m_simvec;  //只要相似都归为一起的集合
    CloneSetSInfo m_allCSetInfo;
};

//
////所有文件的CloneNodeSetVcetor(每个文件一个CloneNodeSetVcetor)
//class CloneSetVecOfAllFile :DebugController
//{
//public:
//	~CloneSetVecOfAllFile();
//	static CloneSetVecOfAllFile g_allclonesetvec;
//	std::vector<CloneNodeSet*> getHighSimSet(std::string& filename); //返回指定文件中的m_highsimvec
//	std::vector<CloneNodeSet*> getNormalSimSet(std::string& filename); //m_normalsimvec
//	std::vector<CloneNodeSet*> getSimSet(std::string& filename); //m_simvec
//
//private:
//	//文件名对应CloneNodeSetVcetor*
//	std::map<std::string, CloneNodeSetVcetor*> m_allclonesetvec;
//	//std::vector<double> m_allsimrate; //所有相似度
//};