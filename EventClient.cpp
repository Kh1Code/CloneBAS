#include"EventClient.h"

EventClient *g_eventClient;

EventClient::EventClient(boost::asio::io_context &io_context,
                        const std::string &host,
            const std::string &port)
    : m_resolver(io_context), m_socket(io_context) {
  // 异步解析主机和端口
  tcp::resolver::query query(host, port);
  std::cout << "试图连接服务器 " << host << ":" << port << std::endl;
  m_resolver.async_resolve(query, [this](boost::system::error_code ec,
                                         tcp::resolver::results_type results) {
    if (!ec) {
      // 异步连接服务器
      boost::asio::async_connect(
          m_socket, results,
          [this](boost::system::error_code ec, tcp::endpoint endpoint) {
            if (!ec) {
              std::cout << "Connected to server "
                        << endpoint.address().to_string() << ":"
                        << endpoint.port() << std::endl;
            } else {
              std::cerr << "Failed to connect to server: " << ec.message()
                        << std::endl;
            }
          });
    } else {
      std::cerr << "Failed to resolve host: " << ec.message() << std::endl;
    }
  });
}

void EventClient::InitHandler() {
  EventCenter::g_eventCenter.AddEventListener(EventName::CaughtWarning,
                                              &EventClient::CatchWarning, this);
  EventCenter::g_eventCenter.AddEventListener(EventName::CaughtError,
                                              &EventClient::CatchError, this);
  EventCenter::g_eventCenter.AddEventListener(EventName::AnaStep,
                                              &EventClient::CatchAnaStep, this);
  EventCenter::g_eventCenter.AddEventListener(EventName::GlobalInfo,
                                              &EventClient::CatchGlobalInfo, this);
}

void EventClient::asyncWrite(const std::string &message) {
  if (message.size() > 4096) {
    //手动分段
    const std::string str1 = message.substr(0, 4096);
    const std::string str2 = message.substr(4096, message.size() - 4096);
    asyncWrite(str1);
    asyncWrite(str2);
    return;
  }
  // 异步写
  boost::asio::async_write(
      m_socket, boost::asio::buffer(message),
      [this](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (!ec) {
          std::cout << "Wrote " << bytes_transferred << " bytes to server"
                    << std::endl;
        } else {
          std::cerr << "Write error: " << ec.message() << std::endl;
        }
      });
}

std::string EventClient::KeepReading(char end) {
  boost::asio::streambuf receiveBuffer;
  std::string result = "";
  while (true) {
    boost::system::error_code error;
    boost::asio::read_until(m_socket, receiveBuffer, '!', error);

    if (!error) {
      std::istream is(&receiveBuffer);
      std::string message;
      std::getline(is, message);
      result += message;
      if (message.find(end) != std::string::npos) {
        break; // 接收到包含 "!" 的消息，退出循环
      }
    } else {
      break; // 读取发生错误，退出循环
    }
  }
  return result;
}

void EventClient::handle_write(const boost::system::error_code &error,
                               std::size_t bytes_transferred) {
  if (!error) {
    std::cout << "Success Sent: " << bytes_transferred << " bytes" << std::endl;
  }
}

void EventClient::CatchError(IErrorInfo errorInfo) {
  auto jv = boost::json::value_from(errorInfo);
  asyncWrite(serialize(jv));
}

void EventClient::CatchWarning(IWarningInfo warningInfo) {
  auto jv = boost::json::value_from(warningInfo);
  asyncWrite(serialize(jv));
}

void EventClient::CatchAnaStep(AnalyzeStep anaStep) {
  auto jv = boost::json::value_from(anaStep);
  asyncWrite(serialize(jv));
}

void EventClient::CatchGlobalInfo(GlobalInfo gInfo) {
  auto jv = boost::json::value_from(gInfo);
  asyncWrite(serialize(jv));
}
