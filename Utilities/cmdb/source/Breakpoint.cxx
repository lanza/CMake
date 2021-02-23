
#include <Breakpoint.h>
#include <iostream>

void Breakpoint::Dump() {
  std::cout << "ID: " << m_id << ", " << m_keyword << std::endl;
}
