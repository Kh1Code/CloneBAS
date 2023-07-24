#pragma once
#include"SimHashConfig.h"
#include"EigenWord.h"

class SimHasher{
public:
	static SimHasher g_simhasher;
	static int GetHammingDis(std::bitset<VEC_LEN>& bs1, std::bitset<VEC_LEN>& bs2); //获取两个bitset的海明距离
	void SimHash(); //进行哈希，返回相似度
	void ReclaimMemory(); //回收相关内存
private:
	//传入两个父节点指纹序号，将他们对应的子节点标记为“不进行比较”,保证传进来参数时，fa1>fa2
	void markSonToNotCmp(int fa1, int fa2); 
	bool checkToCompare(FingerPrint*& fp1, FingerPrint*& fp2); //判断传入的两个指纹是否后续要进行比较
	void SetAllFingerPrints(); //遍历语法树，添加所有指纹
	bool checkCanAddToSet(int n, std::vector<int>& sim_set, int sigma); //判断n是否在sigma的海明距离阈值下可以加进相似集合里
	int initDisJointSet(DisJointSet& dis_set, int sigma); //在sigma的海明距离阈值下，构造传入的并查集。返回有多少对相似
};