#include "Logger.h"

using namespace Log;
double pi_d = 3.1415926535897932384626433832795;
float pi_f = 3.1415926535897932384626433832795f;

int main()
{
  Logger::setLogLevel(Logger::LogLevel::TRACE);
  LOG_TRACE << "LOG_TRACE";
  LOG_DEBUG << "LOG_DEBUG" << 1;

  LOG_INFO << pi_d;
  LOG_WARN << pi_d;

  LOG_ERROR << "LOG_ERROR";

  LOG_FATAL << "LOG_FATAL";

  // LOG_SYSERR << "LOG_SYSERR";
  // LOG_SYSFATAL << "LOG_SYSFATAL";
  return 0;
}