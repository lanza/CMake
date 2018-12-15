
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/Source)
add_library(libCMDB 
  Source/Breakpoint.cxx
  Source/CMakeInstance.cxx
  Source/Command.cxx
  Source/CommandInterpreter.cxx
  Source/Communicator.cxx
  Source/Debugger.cxx
  Source/ListFile.cxx
  )
add_executable(cmdb Source/cmdb.cxx)
target_link_libraries(cmdb 
  libCMDB
  CMakeLib
  )
