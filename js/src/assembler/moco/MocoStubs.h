








































#ifndef _include_assembler_moco_stubs_h_
#define _include_assembler_moco_stubs_h_

namespace JSC {

class JITCode {
public:
  JITCode(void* start, size_t size)
    : m_start(start), m_size(size)
  { }
  JITCode() { }
  void*  start() const { return m_start; }
  size_t size() const { return m_size; }
private:
  void*  m_start;
  size_t m_size;
};

class CodeBlock {
public:
  CodeBlock(JITCode& jc)
    : m_jitcode(jc)
  {
  }
  JITCode& getJITCode() { return m_jitcode; }
private:
  JITCode& m_jitcode;
};

} 

#endif 

