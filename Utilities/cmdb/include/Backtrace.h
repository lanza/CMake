#pragma once
#include <string>

struct Backtrace {
  Backtrace(int index, std::string idk, std::string args, std::string filename,
            int line)
      : m_index{index}, m_idk{idk}, m_args{args},
        m_filename{filename}, m_line{line} {}

  void Dump();

  int m_index;
  std::string m_idk;
  std::string m_args;
  std::string m_filename;
  int m_line;
};
