





#ifndef jit_none_SharedICHelpers_none_h
#define jit_none_SharedICHelpers_none_h

namespace js {
namespace jit {

static const size_t ICStackValueOffset = 0;
static const uint32_t STUB_FRAME_SIZE = 0;
static const uint32_t STUB_FRAME_SAVED_STUB_OFFSET = 0;

inline void EmitRestoreTailCallReg(MacroAssembler&) { MOZ_CRASH(); }
inline void EmitRepushTailCallReg(MacroAssembler&) { MOZ_CRASH(); }
inline void EmitCallIC(CodeOffsetLabel*, MacroAssembler&) { MOZ_CRASH(); }
inline void EmitEnterTypeMonitorIC(MacroAssembler&, size_t v = 0) { MOZ_CRASH(); }
inline void EmitReturnFromIC(MacroAssembler&) { MOZ_CRASH(); }
inline void EmitChangeICReturnAddress(MacroAssembler&, Register) { MOZ_CRASH(); }
inline void EmitTailCallVM(JitCode*, MacroAssembler&, uint32_t) { MOZ_CRASH(); }
inline void EmitCreateStubFrameDescriptor(MacroAssembler&, Register) { MOZ_CRASH(); }
inline void EmitCallVM(JitCode*, MacroAssembler&) { MOZ_CRASH(); }
inline void EmitEnterStubFrame(MacroAssembler&, Register) { MOZ_CRASH(); }
inline void EmitLeaveStubFrame(MacroAssembler&, bool v = false) { MOZ_CRASH(); }
inline void EmitStowICValues(MacroAssembler&, int) { MOZ_CRASH(); }
inline void EmitUnstowICValues(MacroAssembler&, int, bool v = false) { MOZ_CRASH(); }
inline void EmitCallTypeUpdateIC(MacroAssembler&, JitCode*, uint32_t) { MOZ_CRASH(); }
inline void EmitStubGuardFailure(MacroAssembler&) { MOZ_CRASH(); }

template <typename T> inline void EmitPreBarrier(MacroAssembler&, T, MIRType) { MOZ_CRASH(); }

} 
} 

#endif 
