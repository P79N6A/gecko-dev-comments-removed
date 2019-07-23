












































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_PPC_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_PPC_H__

#define MD_FLOATINGSAVEAREA_PPC_FPR_COUNT 32

typedef struct {
  

  u_int64_t fpregs[MD_FLOATINGSAVEAREA_PPC_FPR_COUNT];
  u_int32_t fpscr_pad;
  u_int32_t fpscr;      
} MDFloatingSaveAreaPPC;  


#define MD_VECTORSAVEAREA_PPC_VR_COUNT 32

typedef struct {
  

  u_int128_t save_vr[MD_VECTORSAVEAREA_PPC_VR_COUNT];
  u_int128_t save_vscr;  
  u_int32_t  save_pad5[4];
  u_int32_t  save_vrvalid;  
  u_int32_t  save_pad6[7];
} MDVectorSaveAreaPPC;  


#define MD_CONTEXT_PPC_GPR_COUNT 32




#pragma pack(push, 4)

typedef struct {
  


  u_int32_t             context_flags;

  u_int32_t             srr0;    

  u_int32_t             srr1;    

  

  u_int32_t             gpr[MD_CONTEXT_PPC_GPR_COUNT];
  u_int32_t             cr;      
  u_int32_t             xer;     
  u_int32_t             lr;      
  u_int32_t             ctr;     
  u_int32_t             mq;      
  u_int32_t             vrsave;  

  


  MDFloatingSaveAreaPPC float_save;
  MDVectorSaveAreaPPC   vector_save;
} MDRawContextPPC;  

#pragma pack(pop)





#define MD_CONTEXT_PPC                0x20000000
#define MD_CONTEXT_PPC_BASE           (MD_CONTEXT_PPC | 0x00000001)
#define MD_CONTEXT_PPC_FLOATING_POINT (MD_CONTEXT_PPC | 0x00000008)
#define MD_CONTEXT_PPC_VECTOR         (MD_CONTEXT_PPC | 0x00000020)

#define MD_CONTEXT_PPC_FULL           MD_CONTEXT_PPC_BASE
#define MD_CONTEXT_PPC_ALL            (MD_CONTEXT_PPC_FULL | \
                                       MD_CONTEXT_PPC_FLOATING_POINT | \
                                       MD_CONTEXT_PPC_VECTOR)

#endif 
