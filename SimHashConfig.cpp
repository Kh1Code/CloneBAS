#include "SimHashConfig.h"
#include<bitset>
#include<iostream>

SimHashConfig* SimHashConfig::g_simhashConfig = new SimHashConfig();

SimHashConfig::SimHashConfig()
{
	
}

void SimHashConfig::InitAll() {
  initConfigFile();
  initWeightTable();
  initTokenBounary();
  initFingerprintTokenDif();
}
void SimHashConfig::initConfigFile()
{
	std::cout << " initConfigFile " << std::endl;
	this->weight_config_file = std::ifstream("node_weight_config.txt", std::ios::in);
	if (!this->weight_config_file) {
         std::cout << "�Ҳ���node_weight_config.txt�����ļ�\n";
		exit(EXIT_FAILURE);
	}

	this->token_boundary_config_file = std::ifstream("token_boundary_config.txt", std::ios::in);
	if (!this->token_boundary_config_file) {
          std::cout << "�Ҳ���token_boundary_config.txt�����ļ�\n";
		exit(EXIT_FAILURE);
	}

	this->fingerprint_token_dif_file = std::ifstream("dif_config.txt", std::ios::in);
	if (!this->fingerprint_token_dif_file) {
          std::cout << "�Ҳ���dif_config.txt�����ļ�\n";
		exit(EXIT_FAILURE);
	}
}

void SimHashConfig::initWeightTable()
{
	std::string file_line;
	std::string node_name;
	int weight = 0;
	//���ʱ���Ѿ������������ļ��е�Ĭ��Ȩ�أ���ʼ��ÿ�ֽڵ��Ȩ��
	while (std::getline(this->weight_config_file, file_line)) {
		if (std::isalpha(file_line[0])) { // ��һ���ǽڵ�����
			node_name = file_line;
            weight_table.emplace(node_name, weight);
		}
		else if (std::isdigit(file_line[0])) { //��һ����Ȩ��
			weight = std::atoi(file_line.c_str());
		}
	}
	this->weight_config_file.close();
}

void SimHashConfig::initTokenBounary()
{
	std::string bounary_name;
	int value;
	while (this->token_boundary_config_file >> bounary_name >> value) {
		this->token_boundary_table[bounary_name] = value;
	}
	this->token_boundary_config_file.close();
}

void SimHashConfig::initFingerprintTokenDif()
{
    fingerprint_token_dif_file >> this->fingerprint_token_dif;
    fingerprint_token_dif_file >> this->fingerprint_high_difdis;
    fingerprint_token_dif_file >> this->fingerprint_normal_difdis;
	this->fingerprint_token_dif_file.close();
}