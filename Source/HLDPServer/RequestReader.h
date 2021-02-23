#pragma once

#include <string>
#include <vector>

#include <string.h>

class RequestReader {
private:
  std::vector<char> m_Request;
  int m_ReadPosition;

public:
  void *Reset(size_t payloadSize) {
    m_Request.resize(payloadSize);
    m_ReadPosition = 0;
    return m_Request.data();
  }

  bool ReadInt32(int *pValue) {
    static_assert(sizeof(*pValue) == 4, "Unexpected sizeof(int)");

    if ((m_ReadPosition + sizeof(*pValue)) > m_Request.size())
      return false;
    memcpy(pValue, m_Request.data() + m_ReadPosition, sizeof(*pValue));
    m_ReadPosition += sizeof(*pValue);
    return true;
  }

  bool ReadString(std::string *pStr) {
    int32_t size;
    if (!ReadInt32(&size))
      return false;
    if ((m_ReadPosition + size) > (int32_t)m_Request.size())
      return false;

    *pStr = std::string(m_Request.data() + m_ReadPosition,
                        m_Request.data() + m_ReadPosition + size);
    m_ReadPosition += size;
    return true;
  }
};
