include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/Source)

add_library(
  libCMDB
  source/Backtrace.cxx
  source/Breakpoint.cxx
  source/CMakeInstance.cxx
  source/Command.cxx
  source/CommandInterpreter.cxx
  source/Communicator.cxx
  source/IDECommunicator.cxx
  source/CMakeCommunicator.cxx
  source/Debugger.cxx
  source/ListFile.cxx
  source/Utility.cxx
)

find_package(glog REQUIRED)

add_executable(cmdb source/cmdb.cxx)

target_link_libraries(cmdb libCMDB CMakeLib glog::glog)

install(TARGETS cmdb)
set_target_properties(
  cmdb PROPERTIES RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/bin
)
set_target_properties(
  libCMDB PROPERTIES LIBRARY_OUTPUT_DIRECTORY
  ${CMAKE_BINARY_DIR}/lib
)

find_library(EDITLINE edit "/usr/lib" "C:/Program Files (x86)/WinEditLine/lib")

if(EDLITLINE-NOTFOUND)
  message(FATAL_ERROR "Can't find libedit")
endif()

if(MSVC)
  include_directories("C:/Program Files (x86)/WinEditLine/include")
endif()

target_link_libraries(libCMDB ${EDITLINE} glog::glog)
