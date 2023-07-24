#pragma once
//������Ϣ(����б�Ҫ�Ļ�)�ļ���

#include<set>
#include<vector>
#include<map>
#include<string>
#include<fstream>
#define HIGH_SIM 0  //�����ƶ�
#define NORMAL_SIM 1  //һ�����ƶ�

//�Ƚ������ڵ��˳����������
bool cmp(void* node1, void* node2);

extern bool g_displaycsetInfo;
//��¼һ���ظ����ϵ��ۺ���Ϣ
struct CloneSetSInfo {
  int m_maxLine = -1;
  int m_avgLine = 0;
  int m_minLine = 999999;
  int m_totalLine = 0;
  int m_blockNum = 0;
  void Display(int index, int sim, std::ofstream& file) { 
	  if (sim == 0) {
         file << "�߶��ظ�����[" << index << "]" << std::endl;
	  }
      else if(sim == 1) {
         file << "��ͨ�ظ�����[" << index << "]" << std::endl;
	  }
	  else {
         file << "��������" << std::endl;
	  }
      file << "�������: " << m_maxLine << std::endl;
      file << "��С����: " << m_minLine << std::endl;
      file << "������: " << m_totalLine << std::endl;
      file << "ƽ������: " << m_avgLine << std::endl;
      file << "�ܴ������: " << m_blockNum << std::endl;
      file << std::endl;  
  }
};

//���鼯
class DisJointSet
{
public:
	DisJointSet(int size); //��ʼ��pre�����С,����ʼ��pre����Ԫ��Ϊ-1
	void join(int n1, int n2); //�ϲ�
	int find(int n); //���Ҹ��ڵ�
	void initSets(); //��ʼ��sets
	bool isInASet(int n); //n�Ƿ��Ѿ����ֵ���һ��������
	std::vector<int> getNumsInSameSet(int n); //��ȡ��n��һ�������������
	std::map<int, std::vector<int>> getSets(); //����sets
private:
	std::vector<int> pre; //pre[i]��ʾi�Žڵ��ǰ������pre[i]<0����ʾ�ýڵ�Ϊ��
	std::map<int, std::vector<int>> sets;  //sets[i]��ʾ���i�Žڵ���ͬһ������������нڵ�
};

//�ظ��ڵ㼯��,һ���������Ż������ƵĽڵ㣬��{A,B,C}
class CloneNodeSet
{
public:
    void insertNode(void *node);       //��m_simnodes����ڵ�
	void sortSimnodes(); //��m_simnodes����
    std::vector<void *> getSimNodes(); //����m_simnodes
  private:
    std::vector<void *> m_simnodes; //�������ƵĽڵ�(Node*)
};

//����ظ��ڵ㼯�ϵ���������{ {A,B,C}, {D,E} }
class CloneNodeSetVcetor
{
public:
	~CloneNodeSetVcetor();
	static CloneNodeSetVcetor g_clonenodesetvec;
	std::vector<CloneNodeSet*> getHighSimSet(); //����m_highsimvec
	std::vector<CloneNodeSet*> getNormalSimSet(); //m_normalsimvec
	std::vector<CloneNodeSet*> getSimSet(); //m_simvec
	void addHighSimSet(CloneNodeSet* &clone_set); //��m_highsimvec��Ӽ���
	void addNormalSimSet(CloneNodeSet* &clone_set);
	void addSimSet(CloneNodeSet* &clone_set);
	void Reset();
    void DisplayAndSend(bool display);
	//���Ժ���
    int GetDiffFileCloneScore();
private:
    void DisplayOneSet(CloneNodeSet *cloneset, bool display, int sim, int index);
    int GetDiffFileOneSetCloneScore(CloneNodeSet *cloneset);
	std::vector<CloneNodeSet*> m_highsimvec;  //�߶����Ƶļ���
	std::vector<CloneNodeSet*> m_normalsimvec;  //һ�����Ƶļ���
	std::vector<CloneNodeSet*> m_simvec;  //ֻҪ���ƶ���Ϊһ��ļ���
    CloneSetSInfo m_allCSetInfo;
};

//
////�����ļ���CloneNodeSetVcetor(ÿ���ļ�һ��CloneNodeSetVcetor)
//class CloneSetVecOfAllFile :DebugController
//{
//public:
//	~CloneSetVecOfAllFile();
//	static CloneSetVecOfAllFile g_allclonesetvec;
//	std::vector<CloneNodeSet*> getHighSimSet(std::string& filename); //����ָ���ļ��е�m_highsimvec
//	std::vector<CloneNodeSet*> getNormalSimSet(std::string& filename); //m_normalsimvec
//	std::vector<CloneNodeSet*> getSimSet(std::string& filename); //m_simvec
//
//private:
//	//�ļ�����ӦCloneNodeSetVcetor*
//	std::map<std::string, CloneNodeSetVcetor*> m_allclonesetvec;
//	//std::vector<double> m_allsimrate; //�������ƶ�
//};