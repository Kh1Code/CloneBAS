#pragma once
#define _CPPUNWIND
#include <boost/asio.hpp>
#include <iostream>
#include "InfoStruct.h"
#include "EventCenter.h"

using boost::asio::ip::tcp;

class EventClient {
public:
  EventClient(boost::asio::io_context &io_context, const std::string &host, const std::string &port);

  void InitHandler();

  // 异步写函数
  void asyncWrite(const std::string &message);
  // 同步阻塞读函数
  std::string KeepReading(char end);
private:
  // 异步写回调函数
  void handle_write(const boost::system::error_code &error,
                    std::size_t bytes_transferred);

  void CatchError(IErrorInfo errorInfo); //错误捕获
  void CatchWarning(IWarningInfo warningInfo); //警告捕获
  void CatchAnaStep(AnalyzeStep anaStep); //分析进度捕获
  void CatchGlobalInfo(GlobalInfo gInfo); //全局信息捕获
  //重复集合不使用捕获(直接发送即可)
  
  tcp::resolver m_resolver;
  tcp::socket m_socket;
};

extern EventClient* g_eventClient;