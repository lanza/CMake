#pragma once

#include <type_traits>

namespace sp {

static const char HLDPBanner[] = "HLDPServer High-Level Debug Protocol";
static const int HLDPVersion = 1;

enum class HLDPPacketType {
  Invalid,
  scError,     // Payload: error:string
  scHandshake, // Payload: version:int32, subexpression delimiter:string
  csHandshake, // No payload

  scTargetStopped, // Payload: TargetStopReason:int32, IntArg:int32,
                   // StringArg:string, [array of BacktraceEntry]
  scTargetRunning, // No payload 5

  // No payload for the flow control packets
  csContinue, // 6
  csStepIn,   // 7
  csStepOut,
  csStepOver,
  csBreakIn, // Requests the target to stop ASAP.

  csSetNextStatement, // Payload: file:string, one-based line:int32. Treated as
                      // a flow control statement, i.e. will return
                      // scTargetRunning followed by scTargetStopped

  csTerminate,
  csDetach, // 13

  // Expression commands can only be executed when the target is stopped.
  // All expressions are automatically deleted once the target resumes or
  // performs a step.
  csCreateExpression,  // Payload: Unique Frame ID:int32, Expression:string
  scExpressionCreated, // Payload: ID:int32, name:string, type:string,
                       // value:string, Flags: int32, ChildCount:int32
                       // (ChildCount = -1 indicates that the exact count will
                       // be computed later)
  csQueryExpressionChildren,   // Payload: ID:int32
  scExpressionChildrenQueried, // Payload: array of [ID:int32, name:string,
                               // type:string, value:string, Flags: int32,
                               // ChildCount:int32]
  csSetExpressionValue,        // Payload: ID:int32, value:string
  scExpressionUpdated,         // No payload 19

  // Breakpoint commands can be executed without stopping the target
  BeforeFirstBreakpointRelatedCommand,
  csCreateBreakpoint,         // Payload: file:string, one-based line:string
  csCreateFunctionBreakpoint, // Payload: function name:string
  csCreateDomainSpecificBreakpoint, // Payload: domain-specific (see below)
  scBreakpointCreated,              // Payload: breakpoint ID:int32
  csDeleteBreakpoint,               // Payload: breakpoint ID:int32
  csUpdateBreakpoint,  // Payload: breakpoint ID:int32, updated field: int32,
                       // IntArg1: int32, IntArg2: int32, StringArg:string
  csQueryBreakpoint,   // TBD
  scBreakpointQueried, // TBD
  scBreakpointUpdated, // No payload. Sent as a reply to csDeleteBreakpoint and
                       // csUpdateBreakpoint
  AfterLastBreakpointRelatedCommand,

  scDebugMessage, // Payload: Stream:int32, text:string. Stream is //31
                  // implementation-specific.
  scTargetExited, // Payload: exit code

  ciUpdateFrame, // Sent from client to IDE to update the current frame
  ciVariableEvaluation,
  ciStepOver,
  ciStepOut,
  ciStepIn,
  ciContnue,
  csGetAllVariables, // Payload: None
  scAllVariables,    // Payload: Size:int32, vector of scExpressionCreated
};

enum class TargetStopReason {
  InitialBreakIn = 0,
  Breakpoint, // IntArg will contain breakpoint ID
  BreakInRequested,
  StepComplete,
  UnspecifiedEvent,
  Exception,
  SetNextStatement,
};

enum class BreakpointField : unsigned {
  IsEnabled, // IntArg1 = enabled flag
};

struct HLDPPacketHeader {
  unsigned Type;
  unsigned PayloadSize;
};

enum class CMakeDomainSpecificBreakpointType : unsigned {
  VariableAccessed = 0,
  VariableUpdated,
  MessageSent,
  TargetCreated,
};

template <typename Enumeration>
auto as_integer(Enumeration const value) ->
    typename std::underlying_type_t<Enumeration> {
  return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

} // namespace sp
