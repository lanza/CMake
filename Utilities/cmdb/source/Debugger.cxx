#include <Args.h>
#include <Debugger.h>

#include <sys/wait.h>
#include <thread>

Debugger::Debugger(Args args)
    : m_command_interpreter_sp(new CommandInterpreter()),
      m_cmake_instance_sp(new CMakeInstance()),
      m_cmake_communicator_sp(new CMakeCommunicator()), m_args(args) {
  m_command_interpreter_sp->SetDebugger(this);
  m_cmake_instance_sp->SetCMakeCommunicatorSP(m_cmake_communicator_sp);
}

void Debugger::RunMainLoop() {
  LaunchCMakeInstance();
  if (m_args.GetDashM()) {
    m_cmake_instance_sp->SetHasIDE(true);
    m_ide_communicator_sp = std::make_shared<IDECommunicator>();
    LaunchIDEThread();
  }
  RunCommandInterpreter();
  CleanUp();
}

void IDEThread(Debugger *d) {
  IDECommunicatorSP comm_sp = d->GetIDECommunicatorSP();
  if (!comm_sp->ConnectToIDE(6992)) {
    std::cout << "ide setup failed\n";
    // ide setup failed
    return;
  }
  while (true) {
    RequestReader rr;
    auto res = comm_sp->ReceivePacket(rr);
    (void)res;
    // TODO(lanza) We don't currently do anything with incoming packets
  }
}

void Debugger::LaunchIDEThread() {
  m_ide_thread = std::thread(IDEThread, this);
}

void Debugger::CleanUp() {
  pid_t pid = m_cmake_instance_sp->GetPID();
  int status;
  waitpid(pid, &status, 0);
  if (m_ide_communicator_sp)
    m_ide_communicator_sp->DestroySocket();
}

void Debugger::LaunchCMakeInstance() {

  m_cmake_instance_sp->SetPortNumber(m_args.GetPort());
  if (m_args.GetShouldLaunch()) {
    m_cmake_instance_sp->Launch(m_args);
  }
  m_cmake_communicator_sp->ConnectToCMakeInstance(m_cmake_instance_sp);
  m_command_interpreter_sp->SetCMakeCommunicatorSP(m_cmake_communicator_sp);
}

void Debugger::RunCommandInterpreter() {
  m_command_interpreter_sp->RunMainLoop();
}
