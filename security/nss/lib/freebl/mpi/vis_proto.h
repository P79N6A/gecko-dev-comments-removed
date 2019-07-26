








#ifndef VIS_PROTO_H
#define VIS_PROTO_H

#pragma ident	"@(#)vis_proto.h	1.3	97/03/30 SMI"

#ifdef __cplusplus
extern "C" {
#endif


int vis_edge8(void * , void * );
int vis_edge8l(void * , void * );
int vis_edge16(void * , void * );
int vis_edge16l(void * , void * );
int vis_edge32(void * , void * );
int vis_edge32l(void * , void * );


int vis_edge8cc(void * , void * );
int vis_edge8lcc(void * , void * );
int vis_edge16cc(void * , void * );
int vis_edge16lcc(void * , void * );
int vis_edge32cc(void * , void * );
int vis_edge32lcc(void * , void * );


void *vis_alignaddr(void * , int );
void *vis_alignaddrl(void * , int );
double vis_faligndata(double , double );


int vis_fcmple16(double , double );
int vis_fcmpne16(double , double );
int vis_fcmple32(double , double );
int vis_fcmpne32(double , double );
int vis_fcmpgt16(double , double );
int vis_fcmpeq16(double , double );
int vis_fcmpgt32(double , double );
int vis_fcmpeq32(double , double );


#if 0
double vis_fmul8x16(float , double );
#endif
double vis_fmul8x16_dummy(float , int , double );
double vis_fmul8x16au(float , float );
double vis_fmul8x16al(float , float );
double vis_fmul8sux16(double , double );
double vis_fmul8ulx16(double , double );
double vis_fmuld8ulx16(float , float );
double vis_fmuld8sux16(float , float );


double vis_fpadd16(double , double );
float vis_fpadd16s(float , float );
double vis_fpadd32(double , double );
float vis_fpadd32s(float , float );
double vis_fpsub16(double , double );
float vis_fpsub16s(float , float );
double vis_fpsub32(double , double );
float vis_fpsub32s(float , float );


float vis_fpack16(double );
double vis_fpack32(double , double );
float vis_fpackfix(double );


double vis_fpack16_pair(double , double );
double vis_fpackfix_pair(double , double );
void vis_st2_fpack16(double, double, double *);
void vis_std_fpack16(double, double, double *);
void vis_st2_fpackfix(double, double, double *);

double vis_fpack16_to_hi(double , double );
double vis_fpack16_to_lo(double , double );


double vis_pdist(double , double , double );


double vis_fpmerge(float , float );


double vis_fexpand(float );
double vis_fexpand_hi(double );
double vis_fexpand_lo(double );


double vis_fnor(double , double );
float vis_fnors(float , float );
double vis_fandnot(double , double );
float vis_fandnots(float , float );
double vis_fnot(double );
float vis_fnots(float );
double vis_fxor(double , double );
float vis_fxors(float , float );
double vis_fnand(double , double );
float vis_fnands(float , float );
double vis_fand(double , double );
float vis_fands(float , float );
double vis_fxnor(double , double );
float vis_fxnors(float , float );
double vis_fsrc(double );
float vis_fsrcs(float );
double vis_fornot(double , double );
float vis_fornots(float , float );
double vis_for(double , double );
float vis_fors(float , float );
double vis_fzero(void);
float vis_fzeros(void);
double vis_fone(void);
float vis_fones(void);


void vis_stdfa_ASI_PST8P(double , void * , int );
void vis_stdfa_ASI_PST8PL(double , void * , int );
void vis_stdfa_ASI_PST8P_int_pair(void * , void * ,
                                  void * , int );
void vis_stdfa_ASI_PST8S(double , void * , int );
void vis_stdfa_ASI_PST16P(double , void * , int );
void vis_stdfa_ASI_PST16S(double , void * , int );
void vis_stdfa_ASI_PST32P(double , void * , int );
void vis_stdfa_ASI_PST32S(double , void * , int );


void vis_stdfa_ASI_FL8P(double , void * );
void vis_stdfa_ASI_FL8P_index(double , void * , long );
void vis_stdfa_ASI_FL8S(double , void * );
void vis_stdfa_ASI_FL16P(double , void * );
void vis_stdfa_ASI_FL16P_index(double , void * , long );
void vis_stdfa_ASI_FL16S(double , void * );
void vis_stdfa_ASI_FL8PL(double , void * );
void vis_stdfa_ASI_FL8SL(double , void * );
void vis_stdfa_ASI_FL16PL(double , void * );
void vis_stdfa_ASI_FL16SL(double , void * );


double vis_lddfa_ASI_FL8P(void * );
double vis_lddfa_ASI_FL8P_index(void * , long );
double vis_lddfa_ASI_FL8P_hi(void * , unsigned int );
double vis_lddfa_ASI_FL8P_lo(void * , unsigned int );
double vis_lddfa_ASI_FL8S(void * );
double vis_lddfa_ASI_FL16P(void * );
double vis_lddfa_ASI_FL16P_index(void * , long );
double vis_lddfa_ASI_FL16S(void * );
double vis_lddfa_ASI_FL8PL(void * );
double vis_lddfa_ASI_FL8SL(void * );
double vis_lddfa_ASI_FL16PL(void * );
double vis_lddfa_ASI_FL16SL(void * );


void vis_write_gsr(unsigned int );
unsigned int vis_read_gsr(void);


#if !defined(_NO_LONGLONG)
unsigned long vis_array8(unsigned long long , int );
unsigned long vis_array16(unsigned long long , int );
unsigned long vis_array32(unsigned long long , int );
#endif 


float vis_read_hi(double );
float vis_read_lo(double );
double vis_write_hi(double , float );
double vis_write_lo(double , float );
double vis_freg_pair(float , float );
float vis_to_float(unsigned int );
double vis_to_double(unsigned int , unsigned int );
double vis_to_double_dup(unsigned int );
#if !defined(_NO_LONGLONG)
double vis_ll_to_double(unsigned long long );
#endif 


void vis_error(char * , int );
void vis_sim_init(void);


#define vis_fmul8x16(farg,darg) vis_fmul8x16_dummy((farg),0,(darg))


#define vis_st_u8      vis_stdfa_ASI_FL8P
#define vis_st_u8_i    vis_stdfa_ASI_FL8P_index
#define vis_st_u8_le   vis_stdfa_ASI_FL8PL
#define vis_st_u16     vis_stdfa_ASI_FL16P
#define vis_st_u16_i   vis_stdfa_ASI_FL16P_index
#define vis_st_u16_le  vis_stdfa_ASI_FL16PL

#define vis_ld_u8      vis_lddfa_ASI_FL8P
#define vis_ld_u8_i    vis_lddfa_ASI_FL8P_index
#define vis_ld_u8_le   vis_lddfa_ASI_FL8PL
#define vis_ld_u16     vis_lddfa_ASI_FL16P
#define vis_ld_u16_i   vis_lddfa_ASI_FL16P_index
#define vis_ld_u16_le  vis_lddfa_ASI_FL16PL

#define vis_pst_8      vis_stdfa_ASI_PST8P
#define vis_pst_16     vis_stdfa_ASI_PST16P
#define vis_pst_32     vis_stdfa_ASI_PST32P

#define vis_st_u8s     vis_stdfa_ASI_FL8S
#define vis_st_u8s_le  vis_stdfa_ASI_FL8SL
#define vis_st_u16s    vis_stdfa_ASI_FL16S
#define vis_st_u16s_le vis_stdfa_ASI_FL16SL

#define vis_ld_u8s     vis_lddfa_ASI_FL8S
#define vis_ld_u8s_le  vis_lddfa_ASI_FL8SL
#define vis_ld_u16s    vis_lddfa_ASI_FL16S
#define vis_ld_u16s_le vis_lddfa_ASI_FL16SL

#define vis_pst_8s     vis_stdfa_ASI_PST8S
#define vis_pst_16s    vis_stdfa_ASI_PST16S
#define vis_pst_32s    vis_stdfa_ASI_PST32S


#define vis_fcmplt16(a,b) vis_fcmpgt16((b),(a))
#define vis_fcmplt32(a,b) vis_fcmpgt32((b),(a))
#define vis_fcmpge16(a,b) vis_fcmple16((b),(a))
#define vis_fcmpge32(a,b) vis_fcmple32((b),(a))

#ifdef __cplusplus
} 
#endif 

#endif
