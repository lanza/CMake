#pragma once
#include <string>
#include <vector>

class Args {
public:
  Args(int argc, char const **argv) : m_argc(argc), m_argv(argv) {}

  std::vector<std::string> const &GetArgs() {
    // Only build the vector if needed
    if (m_args.size() != (size_t)m_argc) {
      for (int i = 0; i < m_argc; i++) {
        m_args.push_back(m_argv[i]);
      }
    }

    return m_args;
  }
  int GetArgc() const { return m_argc; }
  char const **GetArgv() const { return m_argv; }

  bool GetDashM() const { return m_dash_m; };
  int GetPort() const { return m_port; }
  void Parse() {
    auto &args = GetArgs();
    auto beg = args.begin();
    auto end = args.end();

    for (auto beg = args.begin(); beg != end; beg++) {
      auto& arg = *beg;
      if (arg == "-p")
        m_port = stoi(*(++beg));
      if (arg == "-m")
        m_dash_m = true;
      if (arg == "-S" || arg == "-B")
        m_should_launch = true;
    }
  }
  bool GetShouldLaunch() const { return m_should_launch; }

private:
  std::vector<std::string> m_args;
  int const m_argc;
  char const **m_argv;

  int m_port = 0;
  bool m_dash_m = false;
  bool m_should_launch = false;
};
