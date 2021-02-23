#include <iostream>

#include <Backtrace.h>

void Backtrace::Dump() {
  std::cout << m_filename << ":" << m_line << " - " << m_idk << "(" << m_args
            << ")\n";
}
