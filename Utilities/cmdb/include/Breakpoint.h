#pragma once
#include <string>

class Breakpoint {

private:
  int m_id;
  std::string m_keyword;
  int m_line;
  std::string m_file;
};
