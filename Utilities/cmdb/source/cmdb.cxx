#include <signal.h>

#include <iostream>
#include <string>
#include <vector>

#include <Args.h>
#include <Debugger.h>

#include <glog/logging.h>

void sigint_handler(int a) {
  exit(44);
}

int main(int argc, char const **argv) {
  google::InitGoogleLogging(argv[0]);

  signal(SIGINT, sigint_handler);

  Args args{argc, argv};
  args.Parse();
  Debugger debugger{args};

  debugger.RunMainLoop();
}
