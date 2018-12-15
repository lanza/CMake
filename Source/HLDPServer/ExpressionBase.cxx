#include "ExpressionBase.h"

bool sp::VariableExpression::UpdateValue(const std::string &value,
                                         std::string &error) {
  auto &entry = cmDefinitions::GetInternal(Name, m_Scope.Position->Vars,
                                           m_Scope.Position->Root, false);

  if (entry.Exists) {
    const_cast<cmDefinitions::Def &>(entry).assign(value);
    return true;
  } else {
    error = "Unable to find variable: " + Name;
    return false;
  }
}
