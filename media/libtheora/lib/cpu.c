



















#include "cpu.h"

#if !defined(OC_X86_ASM)
static ogg_uint32_t oc_cpu_flags_get(void){
  return 0;
}
#else
# if !defined(_MSC_VER)
#  if defined(__amd64__)||defined(__x86_64__)


#   define cpuid(_op,_eax,_ebx,_ecx,_edx) \
  __asm__ __volatile__( \
   "cpuid\n\t" \
   :[eax]"=a"(_eax),[ebx]"=b"(_ebx),[ecx]"=c"(_ecx),[edx]"=d"(_edx) \
   :"a"(_op) \
   :"cc" \
  )
#  else

#   define cpuid(_op,_eax,_ebx,_ecx,_edx) \
  __asm__ __volatile__( \
   "xchgl %%ebx,%[ebx]\n\t" \
   "cpuid\n\t" \
   "xchgl %%ebx,%[ebx]\n\t" \
   :[eax]"=a"(_eax),[ebx]"=r"(_ebx),[ecx]"=c"(_ecx),[edx]"=d"(_edx) \
   :"a"(_op) \
   :"cc" \
  )
#  endif
# else






static void oc_cpuid_helper(ogg_uint32_t _cpu_info[4],ogg_uint32_t _op){
  _asm{
    mov eax,[_op]
    mov esi,_cpu_info
    cpuid
    mov [esi+0],eax
    mov [esi+4],ebx
    mov [esi+8],ecx
    mov [esi+12],edx
  }
}

#  define cpuid(_op,_eax,_ebx,_ecx,_edx) \
  do{ \
    ogg_uint32_t cpu_info[4]; \
    oc_cpuid_helper(cpu_info,_op); \
    (_eax)=cpu_info[0]; \
    (_ebx)=cpu_info[1]; \
    (_ecx)=cpu_info[2]; \
    (_edx)=cpu_info[3]; \
  }while(0)

static void oc_detect_cpuid_helper(ogg_uint32_t *_eax,ogg_uint32_t *_ebx){
  _asm{
    pushfd
    pushfd
    pop eax
    mov ebx,eax
    xor eax,200000h
    push eax
    popfd
    pushfd
    pop eax
    popfd
    mov ecx,_eax
    mov [ecx],eax
    mov ecx,_ebx
    mov [ecx],ebx
  }
}
# endif

static ogg_uint32_t oc_parse_intel_flags(ogg_uint32_t _edx,ogg_uint32_t _ecx){
  ogg_uint32_t flags;
  
  if(!(_edx&0x00800000))return 0;
  flags=OC_CPU_X86_MMX;
  if(_edx&0x02000000)flags|=OC_CPU_X86_MMXEXT|OC_CPU_X86_SSE;
  if(_edx&0x04000000)flags|=OC_CPU_X86_SSE2;
  if(_ecx&0x00000001)flags|=OC_CPU_X86_PNI;
  if(_ecx&0x00000100)flags|=OC_CPU_X86_SSSE3;
  if(_ecx&0x00080000)flags|=OC_CPU_X86_SSE4_1;
  if(_ecx&0x00100000)flags|=OC_CPU_X86_SSE4_2;
  return flags;
}

static ogg_uint32_t oc_parse_amd_flags(ogg_uint32_t _edx,ogg_uint32_t _ecx){
  ogg_uint32_t flags;
  
  if(!(_edx&0x00800000))return 0;
  flags=OC_CPU_X86_MMX;
  if(_edx&0x00400000)flags|=OC_CPU_X86_MMXEXT;
  if(_edx&0x80000000)flags|=OC_CPU_X86_3DNOW;
  if(_edx&0x40000000)flags|=OC_CPU_X86_3DNOWEXT;
  if(_ecx&0x00000040)flags|=OC_CPU_X86_SSE4A;
  if(_ecx&0x00000800)flags|=OC_CPU_X86_SSE5;
  return flags;
}

static ogg_uint32_t oc_cpu_flags_get(void){
  ogg_uint32_t flags;
  ogg_uint32_t eax;
  ogg_uint32_t ebx;
  ogg_uint32_t ecx;
  ogg_uint32_t edx;
# if !defined(__amd64__)&&!defined(__x86_64__)
  
#  if !defined(_MSC_VER)
  __asm__ __volatile__(
   "pushfl\n\t"
   "pushfl\n\t"
   "popl %[a]\n\t"
   "movl %[a],%[b]\n\t"
   "xorl $0x200000,%[a]\n\t"
   "pushl %[a]\n\t"
   "popfl\n\t"
   "pushfl\n\t"
   "popl %[a]\n\t"
   "popfl\n\t"
   :[a]"=r"(eax),[b]"=r"(ebx)
   :
   :"cc"
  );
#  else
  oc_detect_cpuid_helper(&eax,&ebx);
#  endif
  
  if(eax==ebx)return 0;
# endif
  cpuid(0,eax,ebx,ecx,edx);
  
  if(ecx==0x6C65746E&&edx==0x49656E69&&ebx==0x756E6547||
   
   ecx==0x3638784D&&edx==0x54656E69&&ebx==0x756E6547){
    
    cpuid(1,eax,ebx,ecx,edx);
    flags=oc_parse_intel_flags(edx,ecx);
  }
  
  else if(ecx==0x444D4163&&edx==0x69746E65&&ebx==0x68747541||
   
   ecx==0x43534e20&&edx==0x79622065&&ebx==0x646f6547){
    
    cpuid(0x80000000,eax,ebx,ecx,edx);
    if(eax<0x80000001)flags=0;
    else{
      cpuid(0x80000001,eax,ebx,ecx,edx);
      flags=oc_parse_amd_flags(edx,ecx);
    }
    
    cpuid(1,eax,ebx,ecx,edx);
    flags|=oc_parse_intel_flags(edx,ecx);
  }
  




  
  else if(ecx==0x736C7561&&edx==0x48727561&&ebx==0x746E6543){
    
    



    cpuid(1,eax,ebx,ecx,edx);
    flags=oc_parse_intel_flags(edx,ecx);
    if(eax>=0x80000001){
      








      
      cpuid(0x80000001,eax,ebx,ecx,edx);
      









      flags|=oc_parse_amd_flags(edx,ecx);
    }
  }
  else{
    
    flags=0;
  }
  return flags;
}
#endif
