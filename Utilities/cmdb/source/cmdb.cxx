#include <iostream>
#include <string>
#include <vector>

#include <Args.h>
#include <Debugger.h>

int main(int argc, char const** argv) {

  Args args{argc, argv};
  Debugger debugger{args};

  debugger.RunMainLoop();
}
