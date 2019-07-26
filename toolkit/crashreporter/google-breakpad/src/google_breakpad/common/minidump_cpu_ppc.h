












































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_PPC_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_PPC_H__

#define MD_FLOATINGSAVEAREA_PPC_FPR_COUNT 32

typedef struct {
  

  uint64_t fpregs[MD_FLOATINGSAVEAREA_PPC_FPR_COUNT];
  uint32_t fpscr_pad;
  uint32_t fpscr;      
} MDFloatingSaveAreaPPC;  


#define MD_VECTORSAVEAREA_PPC_VR_COUNT 32

typedef struct {
  

  uint128_struct save_vr[MD_VECTORSAVEAREA_PPC_VR_COUNT];
  uint128_struct save_vscr;  
  uint32_t       save_pad5[4];
  uint32_t       save_vrvalid;  
  uint32_t       save_pad6[7];
} MDVectorSaveAreaPPC;  


#define MD_CONTEXT_PPC_GPR_COUNT 32




#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

typedef struct {
  


  uint32_t              context_flags;

  uint32_t              srr0;    

  uint32_t              srr1;    

  

  uint32_t              gpr[MD_CONTEXT_PPC_GPR_COUNT];
  uint32_t              cr;      
  uint32_t              xer;     
  uint32_t              lr;      
  uint32_t              ctr;     
  uint32_t              mq;      
  uint32_t              vrsave;  

  


  MDFloatingSaveAreaPPC float_save;
  MDVectorSaveAreaPPC   vector_save;
} MDRawContextPPC;  

#if defined(__SUNPRO_C) || defined(__SUNPRO_CC)
#pragma pack(0)
#else
#pragma pack(pop)
#endif





#define MD_CONTEXT_PPC                0x20000000
#define MD_CONTEXT_PPC_BASE           (MD_CONTEXT_PPC | 0x00000001)
#define MD_CONTEXT_PPC_FLOATING_POINT (MD_CONTEXT_PPC | 0x00000008)
#define MD_CONTEXT_PPC_VECTOR         (MD_CONTEXT_PPC | 0x00000020)

#define MD_CONTEXT_PPC_FULL           MD_CONTEXT_PPC_BASE
#define MD_CONTEXT_PPC_ALL            (MD_CONTEXT_PPC_FULL | \
                                       MD_CONTEXT_PPC_FLOATING_POINT | \
                                       MD_CONTEXT_PPC_VECTOR)

#endif 
