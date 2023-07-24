#pragma once
#include"json_struct.h"
#include<fstream>
#include<sstream>

//待返回的struct
template<typename T>
struct JsonObjectList
{
    std::vector<T> obj_list;
    JS_OBJECT(JS_MEMBER(obj_list));
};

template<typename T>
JsonObjectList<T> CreateObjList(std::string json_file_path) {
    JsonObjectList<T> json_obj_list;
    //读json文件
    std::ifstream infile = std::ifstream(json_file_path, std::ios::in);
    if (!infile) {
        std::cerr << "Can find json obj file!";
        exit(-1);
    }
    std::stringstream ss;
    ss << infile.rdbuf();
    std::string json_str = ss.str();

    //解析
    JS::ParseContext parseContext(json_str);
    if (parseContext.parseTo(json_obj_list) != JS::Error::NoError)
    {
        std::string errorStr = parseContext.makeErrorString();
        fprintf(stderr, "Error parsing struct %s\n", errorStr.c_str());
        exit(-1);
    }
    return json_obj_list;
};