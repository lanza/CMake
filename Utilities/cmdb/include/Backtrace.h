#pragma once
#include <string>

class Backtrace {

private:
  int m_index;
  std::string m_idk;
  std::string m_args;
  std::string m_filename;
  int m_line;
};
