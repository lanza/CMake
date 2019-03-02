#include <iostream>
#include <string>
#include <vector>

#include <Args.h>
#include <Debugger.h>

int SOCK = 0;

int main(int argc, char const **argv) {
  srand(time(NULL));
  SOCK = rand() % 9000 + 1000;

  Args args{argc, argv};
  Debugger debugger{args};

  debugger.RunMainLoop();
}
