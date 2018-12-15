#include <Args.h>
#include <CMakeInstance.h>

#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#define CMAKE_PATH "/Users/lanza/Projects/CMake/build/bin/cmake"

void CMakeInstance::RunCMake(Args& args) {

  // +3 to account for "--debug-server-port", the port and the NULL
  char const** new_argv = new char const*[args.GetArgc() + 2];
  auto original_argv = args.GetArgv();
  auto original_argc = args.GetArgc();

  for (int i = 1; i < original_argc; i++) {
    new_argv[i + 1] = original_argv[i];
  }
  new_argv[0] = CMAKE_PATH;
  new_argv[1] = "--debug-server-port=9286";
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


