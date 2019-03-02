#pragma once
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> SplitString(std::string str, char delimiter) {
  std::vector<std::string> internal;
  std::stringstream ss(str);
  std::string tok;

  while (getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }

  return internal;
}
