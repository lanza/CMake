#include <Command.h>

void ContinueCommand::DoExecute() {
  (void)3;
  std::cout << "Got to continue command\n";
}
void QuitCommand::DoExecute() {
  (void)3;
  std::cout << "Got to quit command\n";
}
void BreakpointCommand::DoExecute() {
  (void)3;
  std::cout << "Got to breakpoint command\n";
}
