#include "InfoStruct.h"
std::string BoostStringToStdString(const boost::json::string &s) {
  return std::string(s.data(), s.size());
}

//序列化和反序列化函数们
// IErrorInfo序列化与反序列化
void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                IErrorInfo const &obj) {
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "IErrorInfo";
  jo["file"] = obj.m_file;
  jo["column"] = obj.m_colum;
  jo["line"] = obj.m_line;
  jo["id"] = obj.m_id;
  jo["msg"] = obj.m_message;
  boost::json::array json_array;
  for (const auto &item : obj.m_placeholders) {
    json_array.emplace_back(item);
  }
  jo["placeholders"] = json_array;
}

IErrorInfo tag_invoke(boost::json::value_to_tag<IErrorInfo>,
                      boost::json::value const &jv) {
  auto &jo = jv.as_object();
  int fid = jo.at("file").as_int64();
  std::string msg = BoostStringToStdString(jo.at("msg").as_string());
  int line = jo.at("line").as_int64();
  int column = jo.at("column").as_int64();
  int id = jo.at("id").as_int64();
  boost::json::array json_array = jo.at("placeholders").as_array();
  IErrorInfo einfo = IErrorInfo(msg, line, column, fid, id, std::vector<std::string>());
  for (auto item : json_array) {
    einfo.m_placeholders.push_back(BoostStringToStdString(item.as_string()));
  }
  return einfo;
}

// IWarningInfo序列化与反序列化
void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                IWarningInfo const &obj) {
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "IWarningInfo";
  jo["file"] = obj.m_file;
  jo["column"] = obj.m_colum;
  jo["line"] = obj.m_line;
  jo["id"] = obj.m_id;
  jo["msg"] = obj.m_message;
  jo["wlevel"] = obj.m_warningLevel;
  jo["wtype"] = obj.m_warningtype;
  boost::json::array json_array;
  for (const auto &item : obj.m_placeholders) {
    json_array.emplace_back(item);
  }
  jo["placeholders"] = json_array;
}

IWarningInfo tag_invoke(boost::json::value_to_tag<IWarningInfo>,
                        boost::json::value const &jv) {
  auto &jo = jv.as_object();
  int fid = jo.at("file").as_int64();
  std::string msg = BoostStringToStdString(jo.at("msg").as_string());
  int line = jo.at("msg").as_int64();
  int column = jo.at("column").as_int64();
  int id = jo.at("id").as_int64();
  int wlevel = jo.at("wlevel").as_int64();
  int wtype = jo.at("wtype").as_int64();
  boost::json::array json_array = jo.at("placeholders").as_array();
  IWarningInfo einfo = IWarningInfo(msg, line, column, fid, wtype, wlevel, id,
                                    false, std::vector<std::string>());
  for (auto item : json_array) {
    einfo.m_placeholders.push_back(BoostStringToStdString(item.as_string()));
  }
  return einfo;
}

// GlobalInfo序列化与反序列化
void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                GlobalInfo const &obj) {
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "GlobalInfo";
  jo["totalline"] = obj.m_totalline;
  jo["usefulline"] = obj.m_usefulline;
  jo["totalchar"] = obj.m_totalchar;
  jo["noramlsymbol"] = obj.m_noramlsymbol;
  jo["functionsymbol"] = obj.m_functionsymbol;
  jo["classsymbol"] = obj.m_classsymbol;
  jo["score"] =
      boost::json::array(std::begin(obj.m_scores), std::end(obj.m_scores));
}

GlobalInfo tag_invoke(boost::json::value_to_tag<GlobalInfo>,
                      boost::json::value const &jv) {
  GlobalInfo globalInfo;
  auto &jo = jv.as_object();
  globalInfo.m_totalline = jo.at("totalline").as_int64();
  globalInfo.m_usefulline = jo.at("usefulline").as_int64();
  globalInfo.m_totalchar = jo.at("totalchar").as_int64();
  globalInfo.m_noramlsymbol = jo.at("noramlsymbol").as_int64();
  globalInfo.m_functionsymbol = jo.at("functionsymbol").as_int64();
  globalInfo.m_classsymbol = jo.at("classsymbol").as_int64();
  auto sarray = jo.at("score").as_array();
  globalInfo.m_scores[0] = sarray.at(0).as_int64();
  globalInfo.m_scores[1] = sarray.at(1).as_int64();
  globalInfo.m_scores[2] = sarray.at(2).as_int64();
  globalInfo.m_scores[3] = sarray.at(3).as_int64();
  globalInfo.m_scores[4] = sarray.at(4).as_int64();
  return globalInfo;
}

// AnalyzeStep序列化与反序列化
void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                AnalyzeStep const &obj) {
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "AnalyzeStep";
  jo["stepname"] = obj.m_stepName;
  jo["currentStep"] = obj.m_currentStep;
  jo["totalStep"] = obj.m_totalStep;
}

AnalyzeStep tag_invoke(boost::json::value_to_tag<AnalyzeStep>,
                       boost::json::value const &jv) {
  AnalyzeStep step;
  auto &jo = jv.as_object();
  step.m_stepName = jo.at("stepname").as_string();
  step.m_currentStep = jo.at("currentStep").as_int64();
  step.m_totalStep = jo.at("totalStep").as_int64();
  return step;
}

void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                ClonePosition const &obj) {
  auto &jo = jv.emplace_object();
  jo["file"] = obj.m_file;
  jo["beginLine"] = obj.m_beginLine;
  jo["beginColumn"] = obj.m_beginColumn;
  jo["endLine"] = obj.m_endLine;
  jo["endColumn"] = obj.m_endColumn;
}

ClonePosition tag_invoke(boost::json::value_to_tag<ClonePosition>,
    boost::json::value const& jv) {
  ClonePosition cpos;
  auto &jo = jv.as_object();
  cpos.m_file = jo.at("file").as_int64();
  cpos.m_beginLine = jo.at("beginLine").as_int64();
  cpos.m_beginColumn = jo.at("beginColumn").as_int64();
  cpos.m_endLine = jo.at("endLine").as_int64();
  cpos.m_endColumn = jo.at("endColumn").as_int64();
  return cpos;
}

extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       CloneSetInfo const &obj) {
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "CloneSetInfo";
  jo["degree"] = obj.m_degree;
  boost::json::array json_array;
  for (const auto &item : obj.m_cloneSets) {
    json_array.emplace_back(boost::json::value_from(item));
  }
  jo["pos"] = json_array;
}
extern CloneSetInfo tag_invoke(boost::json::value_to_tag<CloneSetInfo>,
                               boost::json::value const &jv) {
  CloneSetInfo cset;
  auto &jo = jv.as_object();
  cset.m_degree = jo.at("degree").as_int64();
  boost::json::array json_array = jo.at("pos").as_array();
  for (auto item : json_array) {
    cset.m_cloneSets.push_back(boost::json::value_to<ClonePosition>(item));
  }
  return cset;
}

extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       CodeNamingInfo const &obj){
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "CodeNamingInfo";
  jo["type"] = obj.m_type;
  boost::json::array json_array_na;
  for (const auto &item : obj.m_namingAmount) {
    json_array_na.emplace_back(item);
  }
  jo["nameamount"] = json_array_na;
  boost::json::array json_array_nt;
  for (const auto &item : obj.m_namingType) {
    json_array_nt.emplace_back(item);
  }
  jo["nametype"] = json_array_nt;
}
extern CodeNamingInfo tag_invoke(boost::json::value_to_tag<CodeNamingInfo>,
                                 boost::json::value const &jv) {
  CodeNamingInfo cni;
  auto &jo = jv.as_object();
  cni.m_type = jo.at("type").as_int64();
  boost::json::array json_array = jo.at("nameamount").as_array();
  for (auto item : json_array) {
    cni.m_namingAmount.push_back(item.as_int64());
  }
  json_array = jo.at("nametype").as_array();
  for (auto item : json_array) {
    cni.m_namingType.push_back(item.as_int64());
  }
  return cni;
}
extern void tag_invoke(boost::json::value_from_tag, boost::json::value &jv,
                       FileTable const &obj) {
  auto &jo = jv.emplace_object();
  jo["TYPE"] = "FileTable";
  boost::json::array json_array;
  for (const auto &item : obj.m_files) {
    json_array.emplace_back(item);
  }
  boost::json::array json_array2;
  for (const auto &item : obj.m_filesID) {
    json_array2.emplace_back(item);
  }
  jo["files"] = json_array;
  jo["fid"] = json_array2;
}
extern FileTable tag_invoke(boost::json::value_to_tag<FileTable>,
                            boost::json::value const &jv) {
  FileTable ft;
  auto &jo = jv.as_object();
  boost::json::array json_array = jo.at("files").as_array();
  for (auto item : json_array) {
    ft.m_files.push_back(BoostStringToStdString(item.as_string()));
  }
  boost::json::array json_array2 = jo.at("fid").as_array();
  for (auto item : json_array2) {
    ft.m_filesID.push_back(item.as_int64());
  }
  return ft;
}

