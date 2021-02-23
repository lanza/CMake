#pragma once
#include <string>
#include <vector>

std::vector<std::string> SplitString(std::string str, char delimiter);
std::string JoinString(std::vector<std::string> strings,
                       char const *const delimiter);
