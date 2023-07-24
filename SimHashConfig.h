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
	std::unordered_map<std::string, int> weight_table; // <�ڵ�����,Ȩ��>
	std::unordered_map<std::string, int> token_boundary_table; // <token��������,���Ƶ�����>
	int fingerprint_token_dif; //����ָ��token���Ĳ�ֵ����
    int fingerprint_high_difdis; //����ָ�Ƹ߶����ƺ�������
    int fingerprint_normal_difdis; //����ָ����ͨ���ƺ�������
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


#define COMPSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["COMPSTMT_CHAR_NUM_BOUNDARY"]) //����������ָ�Ƶ�token������
#define FIXEDCOMPSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FIXEDCOMPSTMT_CHAR_NUM_BOUNDARY"])
#define FIXEDSELECTSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FIXEDSELECTSTMT_CHAR_NUM_BOUNDARY"])
#define FIXEDLOOPSTMT_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FIXEDLOOPSTMT_CHAR_NUM_BOUNDARY"])
#define FUNC_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["FUNC_CHAR_NUM_BOUNDARY"]) //��������ָ�Ƶ�token������
#define CLASS_TOKEN_NUM_BOUNDARY (SimHashConfig::g_simhashConfig->token_boundary_table["CLASS_CHAR_NUM_BOUNDARY"]) //class/struct/union����ָ�Ƶ�token����

#define FINGERPRINT_TOKENNUM_DIF_BOUNARY (SimHashConfig::g_simhashConfig->fingerprint_token_dif)  //����ָ��token���Ĳ�ֵ����
#define FINGERPRINT_HIGH_DIF_BOUNARY (SimHashConfig::g_simhashConfig->fingerprint_high_difdis)  //����ָ�Ƹ߶����ƺ�������
#define FINGERPRINT_NORMAL_DIF_BOUNARY (SimHashConfig::g_simhashConfig->fingerprint_normal_difdis)  //����ָ����ͨ���ƺ�������
