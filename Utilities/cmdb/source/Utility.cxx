#include <Utility.h>
#include <iterator>
#include <sstream>

std::vector<std::string> SplitString(std::string str, char delimiter) {
  std::vector<std::string> internal;
  std::stringstream ss(str);
  std::string tok;

  while (getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }

  return internal;
}

std::string JoinString(std::vector<std::string> strings,
                       char const *const delimiter) {
  std::ostringstream imploded;
  std::copy(strings.begin(), strings.end(),
            std::ostream_iterator<std::string>(imploded, delimiter));
  return imploded.str();
}
