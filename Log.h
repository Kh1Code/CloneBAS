#pragma once
#include<iostream>
#include<string>
#include <chrono>
extern bool g_noLog;

class HighResolutionTimer {
public:
  void start() { startTime = std::chrono::high_resolution_clock::now(); }

  void end() { endTime = std::chrono::high_resolution_clock::now(); }

  double output() {
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    return duration.count();
  }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime,
      endTime;
};

class ILog {
public:
  template <typename T> 
  ILog &operator<<(const T &value) {
    if (g_noLog)
      return *this;
    std::cout << value;
    if (m_logfileptr) {
      *m_logfileptr << value;  
    }
    return *this;
  }

  void CreateLogFile();
private:
  std::string m_logname;
  std::ofstream *m_logfileptr = nullptr;
};

extern ILog gInfoLog;
