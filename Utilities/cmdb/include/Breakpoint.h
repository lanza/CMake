#pragma once
#include <string>

class Breakpoint {
public:
  Breakpoint(int id, std::string result) : m_id{id}, m_keyword{result} {}

  void Dump();

private:
  int m_id;
  std::string m_keyword;
  int m_line;
  std::string m_file;
};
