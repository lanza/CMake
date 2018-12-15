#pragma once

#include <string>
#include <vector>

class ReplyBuilder;

class DelayedSlot {
private:
  ReplyBuilder &m_Builder;
  size_t m_Offset;

public:
  DelayedSlot(ReplyBuilder &builder, size_t offset);

  unsigned &operator*();
};

class ReplyBuilder {
private:
  std::vector<char> m_Reply;
  friend class DelayedSlot;

public:
  ReplyBuilder();

  void AppendData(const void *pData, size_t size);

  void AppendInt32(int value);
  void Reset();

  DelayedSlot AppendDelayedInt32(unsigned initialValue = 0);

  void AppendString(const char *pString);

  void AppendString(const std::string &string);

  const std::vector<char> &GetBuffer() const;
};
