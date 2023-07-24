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

  // �첽д����
  void asyncWrite(const std::string &message);
  // ͬ������������
  std::string KeepReading(char end);
private:
  // �첽д�ص�����
  void handle_write(const boost::system::error_code &error,
                    std::size_t bytes_transferred);

  void CatchError(IErrorInfo errorInfo); //���󲶻�
  void CatchWarning(IWarningInfo warningInfo); //���沶��
  void CatchAnaStep(AnalyzeStep anaStep); //�������Ȳ���
  void CatchGlobalInfo(GlobalInfo gInfo); //ȫ����Ϣ����
  //�ظ����ϲ�ʹ�ò���(ֱ�ӷ��ͼ���)
  
  tcp::resolver m_resolver;
  tcp::socket m_socket;
};

extern EventClient* g_eventClient;