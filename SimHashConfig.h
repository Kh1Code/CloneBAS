#pragma once
#include<vector>
#include<unordered_map>
#include<string>
#include<fstream>
#include"CloneSet.h"
class SimHashConfig
{
public:
	static SimHashConfig* g_simhashConfig;
	std::unordered_map<std::string, int> weight_table; // <节点名称,权重>
	std::unordered_map<std::string, int> token_boundary_table; // <token限制名称,限制的数量>
	int fingerprint_token_dif; //两个指纹token数的差值限制
    int fingerprint_high_difdis; //两个指纹高度相似海明距离
    int fingerprint_normal_difdis; //两个指纹普通相似海明距离
	SimHashConfig();
	~SimHashConfig() {
		delete g_simhashConfig;
	}

	void InitAll();
private:
	std::ifstream weight_config_file;
	std::ifstream token_boundary_config_file;
	std::ifstream fingerprint_token_dif_file;
	void initConfigFile();
	void initWeightTable();
	void initTokenBounary();
	void initFingerprintTokenDif();
};


#define COMPSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["COMPSTMT_CHAR_NUM_BOUNDARY"]) //复合语句计入指纹的token数限制
#define FIXEDCOMPSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FIXEDCOMPSTMT_CHAR_NUM_BOUNDARY"])
#define FIXEDSELECTSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FIXEDSELECTSTMT_CHAR_NUM_BOUNDARY"])
#define FIXEDLOOPSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FIXEDLOOPSTMT_CHAR_NUM_BOUNDARY"])
#define FUNC_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FUNC_CHAR_NUM_BOUNDARY"]) //函数计入指纹的token数限制
#define CLASS_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["CLASS_CHAR_NUM_BOUNDARY"]) //class/struct/union计入指纹的token限制

#define FINGERPRINT_TOKENNUM_DIF_BOUNARY (SimHashConfig::g_simhashConfig->fingerprint_token_dif)  //两个指纹token数的差值限制
#define FINGERPRINT_HIGH_DIF_BOUNARY (SimHashConfig::g_simhashConfig->fingerprint_high_difdis)  //两个指纹高度相似海明距离
#define FINGERPRINT_NORMAL_DIF_BOUNARY (SimHashConfig::g_simhashConfig->fingerprint_normal_difdis)  //两个指纹普通相似海明距离
