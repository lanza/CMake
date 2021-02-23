#include "ReplyBuilder.h"
#include <string.h>

DelayedSlot::DelayedSlot(ReplyBuilder &builder, size_t offset)
    : m_Builder(builder), m_Offset(offset) {}

unsigned &DelayedSlot::operator*() {
  return *(unsigned *)(m_Builder.m_Reply.data() + m_Offset);
}

ReplyBuilder::ReplyBuilder() { m_Reply.reserve(128); }

void ReplyBuilder::AppendData(const void *pData, size_t size) {
  size_t offset = m_Reply.size();
  m_Reply.resize(offset + size);
  memcpy(m_Reply.data() + offset, pData, size);
}

void ReplyBuilder::AppendInt32(int value) {
  static_assert(sizeof(value) == 4, "Unexpected sizeof(int)");
  AppendData(&value, sizeof(value));
}

void ReplyBuilder::Reset() { m_Reply.resize(0); }

DelayedSlot ReplyBuilder::AppendDelayedInt32(unsigned initialValue) {
  DelayedSlot slot(*this, m_Reply.size());
  AppendInt32(initialValue);
  return slot;
}

void ReplyBuilder::AppendString(const char *pString) {
  if (!pString)
    pString = "";
  int len = strlen(pString);
  AppendInt32(len);
  AppendData(pString, len);
}

void ReplyBuilder::AppendString(const std::string &string) {
  int len = string.size();
  AppendInt32(len);
  AppendData(string.c_str(), len);
}

const std::vector<char> &ReplyBuilder::GetBuffer() const { return m_Reply; }
