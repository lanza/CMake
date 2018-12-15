#pragma once
#include <string>
#include <vector>

class Args {
public:
  Args(int argc, char const** argv) :m_argc(argc), m_argv(argv) {}

  std::vector<std::string> const& GetArgs() {
    // Only build the vector if needed
    if (m_args.size() != m_argc) {
      for (int i = 0; i < m_argc; i++) {
        m_args.push_back(m_argv[i]); 
      }
    }

    return m_args;
  }
  int const GetArgc() const { return m_argc; }
  char const** GetArgv() const { return m_argv; }
private:
  std::vector<std::string> m_args;
  int const m_argc;
  char const** m_argv;
};
