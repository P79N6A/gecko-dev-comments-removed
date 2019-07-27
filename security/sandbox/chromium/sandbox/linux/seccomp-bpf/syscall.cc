



#include "sandbox/linux/seccomp-bpf/syscall.h"

#include <asm/unistd.h>
#include <errno.h>

#include "base/basictypes.h"
#include "base/logging.h"
#include "sandbox/linux/seccomp-bpf/linux_seccomp.h"

namespace sandbox {

namespace {

#if defined(ARCH_CPU_X86_FAMILY) || defined(ARCH_CPU_ARM_FAMILY) || \
    defined(ARCH_CPU_MIPS_FAMILY)

const int kInvalidSyscallNumber = 0x351d3;
#else
#error Unrecognized architecture
#endif

asm(
    
    
    
    
    
    
    
    
    
    
    
    
#if defined(__i386__)
    ".text\n"
    ".align 16, 0x90\n"
    ".type SyscallAsm, @function\n"
    "SyscallAsm:.cfi_startproc\n"
    
    
    
    
    "test %eax, %eax\n"
    "jge  1f\n"
    
    
    
    "call 0f;   .cfi_adjust_cfa_offset  4\n"
    "0:pop  %eax; .cfi_adjust_cfa_offset -4\n"
    "addl $2f-0b, %eax\n"
    "ret\n"
    
    
    
    
    
    "1:push %esi; .cfi_adjust_cfa_offset 4; .cfi_rel_offset esi, 0\n"
    "push %edi; .cfi_adjust_cfa_offset 4; .cfi_rel_offset edi, 0\n"
    "push %ebx; .cfi_adjust_cfa_offset 4; .cfi_rel_offset ebx, 0\n"
    "push %ebp; .cfi_adjust_cfa_offset 4; .cfi_rel_offset ebp, 0\n"
    
    
    "movl  0(%edi), %ebx\n"
    "movl  4(%edi), %ecx\n"
    "movl  8(%edi), %edx\n"
    "movl 12(%edi), %esi\n"
    "movl 20(%edi), %ebp\n"
    "movl 16(%edi), %edi\n"
    
    "int  $0x80\n"
    
    "2:"
    
    
    "pop  %ebp; .cfi_restore ebp; .cfi_adjust_cfa_offset -4\n"
    "pop  %ebx; .cfi_restore ebx; .cfi_adjust_cfa_offset -4\n"
    "pop  %edi; .cfi_restore edi; .cfi_adjust_cfa_offset -4\n"
    "pop  %esi; .cfi_restore esi; .cfi_adjust_cfa_offset -4\n"
    "ret\n"
    ".cfi_endproc\n"
    "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__x86_64__)
    ".text\n"
    ".align 16, 0x90\n"
    ".type SyscallAsm, @function\n"
    "SyscallAsm:.cfi_startproc\n"
    
    
    
    
    "test %rdi, %rdi\n"
    "jge  1f\n"
    
    
    "lea 2f(%rip), %rax\n"
    "ret\n"
    
    
    
    
    
    "1:movq %rdi, %rax\n"
    "movq  0(%rsi), %rdi\n"
    "movq 16(%rsi), %rdx\n"
    "movq 24(%rsi), %r10\n"
    "movq 32(%rsi), %r8\n"
    "movq 40(%rsi), %r9\n"
    "movq  8(%rsi), %rsi\n"
    
    "syscall\n"
    
    "2:ret\n"
    ".cfi_endproc\n"
    "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__arm__)
    
    
    
    
    
    
    
    
    ".text\n"
    ".align 2\n"
    ".type SyscallAsm, %function\n"
#if defined(__thumb__)
    ".thumb_func\n"
#else
    ".arm\n"
#endif
    "SyscallAsm:.fnstart\n"
    "@ args = 0, pretend = 0, frame = 8\n"
    "@ frame_needed = 1, uses_anonymous_args = 0\n"
#if defined(__thumb__)
    ".cfi_startproc\n"
    "push {r7, lr}\n"
    ".cfi_offset 14, -4\n"
    ".cfi_offset  7, -8\n"
    "mov r7, sp\n"
    ".cfi_def_cfa_register 7\n"
    ".cfi_def_cfa_offset 8\n"
#else
    "stmfd sp!, {fp, lr}\n"
    "add fp, sp, #4\n"
#endif
    
    
    
    
    "cmp r0, #0\n"
    "bge 1f\n"
    "adr r0, 2f\n"
    "b   2f\n"
    
    
    
    
    "1:ldr r5, [r6, #20]\n"
    "ldr r4, [r6, #16]\n"
    "ldr r3, [r6, #12]\n"
    "ldr r2, [r6, #8]\n"
    "ldr r1, [r6, #4]\n"
    "mov r7, r0\n"
    "ldr r0, [r6, #0]\n"
    
    "swi 0\n"


#if defined(__thumb__)
    "2:pop {r7, pc}\n"
    ".cfi_endproc\n"
#else
    "2:ldmfd sp!, {fp, pc}\n"
#endif
    ".fnend\n"
    "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__mips__)
    ".text\n"
    ".align 4\n"
    ".type SyscallAsm, @function\n"
    "SyscallAsm:.ent SyscallAsm\n"
    ".frame  $sp, 40, $ra\n"
    ".set   push\n"
    ".set   noreorder\n"
    "addiu  $sp, $sp, -40\n"
    "sw     $ra, 36($sp)\n"
    
    
    
    
    "bgez   $v0, 1f\n"
    " nop\n"
    "la     $v0, 2f\n"
    "b      2f\n"
    " nop\n"
    
    
    
    
    "1:lw     $a3, 28($a0)\n"
    "lw     $a2, 24($a0)\n"
    "lw     $a1, 20($a0)\n"
    "lw     $t0, 16($a0)\n"
    "sw     $a3, 28($sp)\n"
    "sw     $a2, 24($sp)\n"
    "sw     $a1, 20($sp)\n"
    "sw     $t0, 16($sp)\n"
    "lw     $a3, 12($a0)\n"
    "lw     $a2, 8($a0)\n"
    "lw     $a1, 4($a0)\n"
    "lw     $a0, 0($a0)\n"
    
    "syscall\n"
    
    
    "2:lw     $ra, 36($sp)\n"
    "jr     $ra\n"
    " addiu  $sp, $sp, 40\n"
    ".set    pop\n"
    ".end    SyscallAsm\n"
    ".size   SyscallAsm,.-SyscallAsm\n"
#elif defined(__aarch64__)
    ".text\n"
    ".align 2\n"
    ".type SyscallAsm, %function\n"
    "SyscallAsm:\n"
    ".cfi_startproc\n"
    "cmp x0, #0\n"
    "b.ge 1f\n"
    "adr x0,2f\n"
    "b 2f\n"
    "1:ldr x5, [x6, #40]\n"
    "ldr x4, [x6, #32]\n"
    "ldr x3, [x6, #24]\n"
    "ldr x2, [x6, #16]\n"
    "ldr x1, [x6, #8]\n"
    "mov x8, x0\n"
    "ldr x0, [x6, #0]\n"
    
    "svc 0\n"
    "2:ret\n"
    ".cfi_endproc\n"
    ".size SyscallAsm, .-SyscallAsm\n"
#endif
    );  

#if defined(__x86_64__)
extern "C" {
intptr_t SyscallAsm(intptr_t nr, const intptr_t args[6]);
}
#endif

}  

intptr_t Syscall::InvalidCall() {
  
  return Call(kInvalidSyscallNumber, 0, 0, 0, 0, 0, 0, 0, 0);
}

intptr_t Syscall::Call(int nr,
                       intptr_t p0,
                       intptr_t p1,
                       intptr_t p2,
                       intptr_t p3,
                       intptr_t p4,
                       intptr_t p5,
                       intptr_t p6,
                       intptr_t p7) {
  
  
  
  
  
  
  
  COMPILE_ASSERT(sizeof(void*) == sizeof(intptr_t),
                 pointer_types_and_intptr_must_be_exactly_the_same_size);

  
  
#if defined(__mips__)
  const intptr_t args[8] = {p0, p1, p2, p3, p4, p5, p6, p7};
#else
  DCHECK_EQ(p6, 0) << " Support for syscalls with more than six arguments not "
                      "added for this architecture";
  DCHECK_EQ(p7, 0) << " Support for syscalls with more than six arguments not "
                      "added for this architecture";
  const intptr_t args[6] = {p0, p1, p2, p3, p4, p5};
#endif  




#if defined(__i386__)
  intptr_t ret = nr;
  asm volatile(
      "call SyscallAsm\n"
      
      : "=a"(ret)
      : "0"(ret), "D"(args)
      : "cc", "esp", "memory", "ecx", "edx");
#elif defined(__x86_64__)
  intptr_t ret = SyscallAsm(nr, args);
#elif defined(__arm__)
  intptr_t ret;
  {
    register intptr_t inout __asm__("r0") = nr;
    register const intptr_t* data __asm__("r6") = args;
    asm volatile(
        "bl SyscallAsm\n"
        
        : "=r"(inout)
        : "0"(inout), "r"(data)
        : "cc",
          "lr",
          "memory",
          "r1",
          "r2",
          "r3",
          "r4",
          "r5"
#if !defined(__thumb__)
          
          
          
          
          
          
          ,
          "r7"
#endif  
        );
    ret = inout;
  }
#elif defined(__mips__)
  int err_status;
  intptr_t ret = Syscall::SandboxSyscallRaw(nr, args, &err_status);

  if (err_status) {
    
    
    
    ret = -ret;
  }
#elif defined(__aarch64__)
  intptr_t ret;
  {
    register intptr_t inout __asm__("x0") = nr;
    register const intptr_t* data __asm__("x6") = args;
    asm volatile("bl SyscallAsm\n"
                 : "=r"(inout)
                 : "0"(inout), "r"(data)
                 : "memory", "x1", "x2", "x3", "x4", "x5", "x8", "x30");
    ret = inout;
  }

#else
#error "Unimplemented architecture"
#endif
  return ret;
}

void Syscall::PutValueInUcontext(intptr_t ret_val, ucontext_t* ctx) {
#if defined(__mips__)
  
  
  if (ret_val <= -1 && ret_val >= -4095) {
    
    
    
    ret_val = -ret_val;
    SECCOMP_PARM4(ctx) = 1;
  } else
    SECCOMP_PARM4(ctx) = 0;
#endif
  SECCOMP_RESULT(ctx) = static_cast<greg_t>(ret_val);
}

#if defined(__mips__)
intptr_t Syscall::SandboxSyscallRaw(int nr,
                                    const intptr_t* args,
                                    intptr_t* err_ret) {
  register intptr_t ret __asm__("v0") = nr;
  
  register intptr_t err_stat __asm__("a3") = 0;
  {
    register const intptr_t* data __asm__("a0") = args;
    asm volatile(
        "la $t9, SyscallAsm\n"
        "jalr $t9\n"
        " nop\n"
        : "=r"(ret), "=r"(err_stat)
        : "0"(ret),
          "r"(data)
          
          
        : "memory", "ra", "t9", "a2");
  }

  
  *err_ret = err_stat;

  return ret;
}
#endif  

}  
