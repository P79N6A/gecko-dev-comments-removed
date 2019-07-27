





#include "sandbox/win/src/sidestep/preamble_patcher.h"

#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sidestep/mini_disassembler.h"


#define ASM_JMP32REL 0xE9
#define ASM_INT3 0xCC

namespace {




inline void* RawMemcpy(void* destination, const void* source, size_t bytes) {
  const char* from = reinterpret_cast<const char*>(source);
  char* to = reinterpret_cast<char*>(destination);

  for (size_t i = 0; i < bytes ; i++)
    to[i] = from[i];

  return destination;
}




inline void* RawMemset(void* destination, int value, size_t bytes) {
  char* to = reinterpret_cast<char*>(destination);

  for (size_t i = 0; i < bytes ; i++)
    to[i] = static_cast<char>(value);

  return destination;
}

}  

#define ASSERT(a, b) DCHECK_NT(a)

namespace sidestep {

SideStepError PreamblePatcher::RawPatchWithStub(
    void* target_function,
    void* replacement_function,
    unsigned char* preamble_stub,
    size_t stub_size,
    size_t* bytes_needed) {
  if ((NULL == target_function) ||
      (NULL == replacement_function) ||
      (NULL == preamble_stub)) {
    ASSERT(false, (L"Invalid parameters - either pTargetFunction or "
                   L"pReplacementFunction or pPreambleStub were NULL."));
    return SIDESTEP_INVALID_PARAMETER;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  unsigned char* target = reinterpret_cast<unsigned char*>(target_function);

  
  
  
  
  MiniDisassembler disassembler;
  unsigned int preamble_bytes = 0;
  while (preamble_bytes < 5) {
    InstructionType instruction_type =
      disassembler.Disassemble(target + preamble_bytes, &preamble_bytes);
    if (IT_JUMP == instruction_type) {
      ASSERT(false, (L"Unable to patch because there is a jump instruction "
                     L"in the first 5 bytes."));
      return SIDESTEP_JUMP_INSTRUCTION;
    } else if (IT_RETURN == instruction_type) {
      ASSERT(false, (L"Unable to patch because function is too short"));
      return SIDESTEP_FUNCTION_TOO_SMALL;
    } else if (IT_GENERIC != instruction_type) {
      ASSERT(false, (L"Disassembler encountered unsupported instruction "
                     L"(either unused or unknown"));
      return SIDESTEP_UNSUPPORTED_INSTRUCTION;
    }
  }

  if (NULL != bytes_needed)
    *bytes_needed = preamble_bytes + 5;

  
  
  
  
  if (preamble_bytes + 5 > stub_size) {
    NOTREACHED_NT();
    return SIDESTEP_INSUFFICIENT_BUFFER;
  }

  
  RawMemcpy(reinterpret_cast<void*>(preamble_stub),
            reinterpret_cast<void*>(target), preamble_bytes);

  
  
  
#pragma warning(push)
#pragma warning(disable:4244)
  
  int relative_offset_to_target_rest
    = ((reinterpret_cast<unsigned char*>(target) + preamble_bytes) -
        (preamble_stub + preamble_bytes + 5));
#pragma warning(pop)
  
  preamble_stub[preamble_bytes] = ASM_JMP32REL;
  
  RawMemcpy(reinterpret_cast<void*>(preamble_stub + preamble_bytes + 1),
            reinterpret_cast<void*>(&relative_offset_to_target_rest), 4);

  
  
  

  
  
  
  target[0] = ASM_JMP32REL;

  
#pragma warning(push)
#pragma warning(disable:4244)
  int offset_to_replacement_function =
    reinterpret_cast<unsigned char*>(replacement_function) -
    reinterpret_cast<unsigned char*>(target) - 5;
#pragma warning(pop)
  
  RawMemcpy(reinterpret_cast<void*>(target + 1),
            reinterpret_cast<void*>(&offset_to_replacement_function), 4);
  
  
  
  
  
  
  
  if (preamble_bytes > 5) {
    RawMemset(reinterpret_cast<void*>(target + 5), ASM_INT3,
              preamble_bytes - 5);
  }

  
  
  
  
  

  return SIDESTEP_SUCCESS;
}

};  

#undef ASSERT
