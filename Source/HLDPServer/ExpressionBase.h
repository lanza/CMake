#pragma once

#include "cmCommand.h"

#include "cmStatePrivate.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmVariableWatch.h"
#include "cmsys/String.h"

#include "RAIIScope.h"

#include <string>
#include <vector>

using UniqueScopeID = int;
using UniqueExpressionID = int;

class cmTarget;

namespace sp {

class RAIIScope;

class ExpressionBase {
public:
  UniqueExpressionID AssignedID = -1;
  std::string Name, Value, Type;
  int ChildCountOrMinusOneIfNotYetComputed = 0;

public:
  std::vector<UniqueExpressionID> RegisteredChildren;
  bool ChildrenRegistered = false;
  virtual std::vector<std::unique_ptr<ExpressionBase>> CreateChildren() {
    return std::vector<std::unique_ptr<ExpressionBase>>();
  }

  virtual bool UpdateValue(const std::string &value, std::string &error) {
    (void)value;
    error = "This expression cannot be edited.";
    return false;
  }

public:
  virtual ~ExpressionBase() {}
};

class SimpleExpression : public ExpressionBase {
public:
  SimpleExpression(const std::string &name, const std::string &type,
                   const std::string &value) {
    Name = name;
    Type = type;
    Value = value;
  }
};

class VariableExpression : public ExpressionBase {
private:
  const RAIIScope &m_Scope;

public:
  VariableExpression(const RAIIScope &scope, const std::string &name,
                     const std::string *pValue)
      : m_Scope(scope) {
    Name = name;
    Type = "(CMake Expression)";
    Value = pValue->c_str();
  }

  virtual bool UpdateValue(const std::string &value,
                           std::string &error) override;
};

class EnvironmentVariableExpression : public ExpressionBase {
private:
  std::string m_VarName;

public:
  EnvironmentVariableExpression(const std::string &name,
                                const std::string &value,
                                bool fromEnvList = false)
      : m_VarName(name) {
    if (!fromEnvList)
      Name = "ENV{" + name + "}";
    else
      Name = "[" + name + "]";

    Type = "(Environment Variable)";
    Value = value;
  }

  virtual bool UpdateValue(const std::string &value,
                           std::string &error) override {
    (void)error;
    cmSystemTools::PutEnv(m_VarName + "=" + value);
    return true;
  }
};

class EnvironmentMetaExpression : public ExpressionBase {
public:
  EnvironmentMetaExpression() {
    Name = "$ENV";
    Type = "(CMake Environment)";
    Value = "<...>";
    ChildCountOrMinusOneIfNotYetComputed = -1;
  }

  virtual std::vector<std::unique_ptr<ExpressionBase>> CreateChildren() {
    std::vector<std::unique_ptr<ExpressionBase>> result;
    for (const auto &kv : cmSystemTools::GetEnvironmentVariables()) {
      auto idx = kv.find('=');
      if (idx == std::string::npos)
        continue;
      result.push_back(std::make_unique<EnvironmentVariableExpression>(
          kv.substr(0, idx), kv.substr(idx + 1), true));
    }
    return result;
  }
};

class TargetExpression : public ExpressionBase {
private:
  cmTarget *m_pTarget;

public:
  TargetExpression(cmTarget *pTarget) : m_pTarget(pTarget) {
    Type = "(CMake target)";
    Name = pTarget->GetName();
    Value = "target";
    ChildCountOrMinusOneIfNotYetComputed = -1;
  }

  virtual std::vector<std::unique_ptr<ExpressionBase>>
  CreateChildren() override {
    std::vector<std::unique_ptr<ExpressionBase>> result;
    for (const auto &kv : m_pTarget->GetProperties().GetList())
      result.push_back(std::make_unique<SimpleExpression>(
          kv.first, "(property entry)", kv.second));
    return result;
  }
};

} // namespace sp
