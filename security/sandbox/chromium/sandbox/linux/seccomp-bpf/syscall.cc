



#include "sandbox/linux/seccomp-bpf/syscall.h"

#include <asm/unistd.h>
#include <errno.h>

#include "base/basictypes.h"

namespace sandbox {

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
            
            
            
            
            
          "1:push %esi; .cfi_adjust_cfa_offset 4\n"
            "push %edi; .cfi_adjust_cfa_offset 4\n"
            "push %ebx; .cfi_adjust_cfa_offset 4\n"
            "push %ebp; .cfi_adjust_cfa_offset 4\n"
            
            
            "movl  0(%edi), %ebx\n"
            "movl  4(%edi), %ecx\n"
            "movl  8(%edi), %edx\n"
            "movl 12(%edi), %esi\n"
            "movl 20(%edi), %ebp\n"
            "movl 16(%edi), %edi\n"
            
            "int  $0x80\n"
            
          "2:"
            
            
            "pop  %ebp; .cfi_adjust_cfa_offset -4\n"
            "pop  %ebx; .cfi_adjust_cfa_offset -4\n"
            "pop  %edi; .cfi_adjust_cfa_offset -4\n"
            "pop  %esi; .cfi_adjust_cfa_offset -4\n"
            "ret\n"
            ".cfi_endproc\n"
          "9:.size SyscallAsm, 9b-SyscallAsm\n"
#elif defined(__x86_64__)
            ".text\n"
            ".align 16, 0x90\n"
            ".type SyscallAsm, @function\n"
 "SyscallAsm:.cfi_startproc\n"
            
            
            
            
            "test %rax, %rax\n"
            "jge  1f\n"
            
            
            "call 0f;   .cfi_adjust_cfa_offset  8\n"
          "0:pop  %rax; .cfi_adjust_cfa_offset -8\n"
            "addq $2f-0b, %rax\n"
            "ret\n"
            
            
            
            
          "1:movq  0(%r12), %rdi\n"
            "movq  8(%r12), %rsi\n"
            "movq 16(%r12), %rdx\n"
            "movq 24(%r12), %r10\n"
            "movq 32(%r12), %r8\n"
            "movq 40(%r12), %r9\n"
            
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
#endif
  );  

intptr_t SandboxSyscall(int nr,
                        intptr_t p0, intptr_t p1, intptr_t p2,
                        intptr_t p3, intptr_t p4, intptr_t p5) {
  
  
  
  
  
  
  
  COMPILE_ASSERT(sizeof(void *) == sizeof(intptr_t),
                 pointer_types_and_intptr_must_be_exactly_the_same_size);

  const intptr_t args[6] = { p0, p1, p2, p3, p4, p5 };

  
  
  
#if defined(__i386__)
  intptr_t ret = nr;
  asm volatile(
    "call SyscallAsm\n"
    
    : "=a"(ret)
    : "0"(ret), "D"(args)
    : "cc", "esp", "memory", "ecx", "edx");
#elif defined(__x86_64__)
  intptr_t ret = nr;
  {
    register const intptr_t *data __asm__("r12") = args;
    asm volatile(
      "lea  -128(%%rsp), %%rsp\n"  
      "call SyscallAsm\n"
      "lea  128(%%rsp), %%rsp\n"
      
      : "=a"(ret)
      : "0"(ret), "r"(data)
      : "cc", "rsp", "memory",
        "rcx", "rdi", "rsi", "rdx", "r8", "r9", "r10", "r11");
  }
#elif defined(__arm__)
  intptr_t ret;
  {
    register intptr_t inout __asm__("r0") = nr;
    register const intptr_t *data __asm__("r6") = args;
    asm volatile(
      "bl SyscallAsm\n"
      
      : "=r"(inout)
      : "0"(inout), "r"(data)
      : "cc", "lr", "memory", "r1", "r2", "r3", "r4", "r5"
#if !defined(__thumb__)
      
      
      
      
      
        , "r7"
#endif  
      );
    ret = inout;
  }
#else
  errno = ENOSYS;
  intptr_t ret = -1;
#endif
  return ret;
}

}  
