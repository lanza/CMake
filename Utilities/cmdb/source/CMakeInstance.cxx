#include <Args.h>
#include <CMakeInstance.h>

#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#define CMAKE_PATH "/Users/lanza/Projects/CMake/build/bin/cmake"

extern int SOCK;

void CMakeInstance::RunCMake(Args& args) {

  // +3 to account for "--debug-server-port", the port and the NULL
  char const** new_argv = new char const*[args.GetArgc() + 2];
  auto original_argv = args.GetArgv();
  auto original_argc = args.GetArgc();

  for (int i = 1; i < original_argc; i++) {
    new_argv[i + 1] = original_argv[i];
  }
  new_argv[0] = CMAKE_PATH;
  std::string argv1 = "--debug-server-port=";
  argv1 += std::to_string(SOCK);

  new_argv[1] = argv1.c_str();
  new_argv[original_argc + 1] = NULL;

  for (int i = 0; i < original_argc + 1; i++) {
    std::cout << new_argv[i] << std::endl;
  }
  execv(CMAKE_PATH, (char**)new_argv);
}

void CMakeInstance::Launch(Args& args) {

  m_pid = fork();
  if (m_pid == 0) {
    RunCMake(args);
  } else {
    //TODO(lanza): implement this
  }
}

void CMakeInstance::TargetDidStop(RequestReader& reader) {
}
