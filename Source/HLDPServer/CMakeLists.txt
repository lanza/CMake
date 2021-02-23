add_library(
  HLDPServer
  BasicBreakpointManager.cxx
  ExpressionBase.cxx
  HLDPServer.cxx
  RAIIScope.cxx
  ReplyBuilder.cxx
)

target_link_libraries(HLDPServer PUBLIC CMakeLib cmsys glog::glog)
