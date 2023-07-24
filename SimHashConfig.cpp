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
         std::cout << "找不到node_weight_config.txt配置文件\n";
		exit(EXIT_FAILURE);
	}

	this->token_boundary_config_file = std::ifstream("token_boundary_config.txt", std::ios::in);
	if (!this->token_boundary_config_file) {
          std::cout << "找不到token_boundary_config.txt配置文件\n";
		exit(EXIT_FAILURE);
	}

	this->fingerprint_token_dif_file = std::ifstream("dif_config.txt", std::ios::in);
	if (!this->fingerprint_token_dif_file) {
          std::cout << "找不到dif_config.txt配置文件\n";
		exit(EXIT_FAILURE);
	}
}

void SimHashConfig::initWeightTable()
{
	std::string file_line;
	std::string node_name;
	int weight = 0;
	//这个时候已经读过了配置文件中的默认权重，开始读每种节点的权重
	while (std::getline(this->weight_config_file, file_line)) {
		if (std::isalpha(file_line[0])) { // 这一行是节点名字
			node_name = file_line;
            weight_table.emplace(node_name, weight);
		}
		else if (std::isdigit(file_line[0])) { //这一行是权重
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