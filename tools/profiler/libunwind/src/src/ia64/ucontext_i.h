


























#define SIG_BLOCK	0
#define SIG_UNBLOCK	1
#define SIG_SETMASK	2

#define IA64_SC_FLAG_SYNCHRONOUS_BIT	63

#define SC_FLAGS 0x000
#define SC_NAT	0x008
#define SC_BSP	0x048
#define SC_RNAT	0x050
#define SC_UNAT	0x060
#define SC_FPSR	0x068
#define SC_PFS	0x070
#define SC_LC	0x078
#define SC_PR	0x080
#define SC_BR	0x088
#define SC_GR	0x0c8
#define SC_FR	0x1d0
#define SC_MASK	0x9d0


#define rTMP	r10
#define rPOS	r11
#define rCPOS	r14
#define rNAT	r15
#define rFLAGS	r16

#define rB5	r18
#define rB4	r19
#define rB3	r20
#define rB2	r21
#define rB1	r22
#define rB0	r23
#define rRSC	r24
#define rBSP	r25
#define rRNAT	r26
#define rUNAT	r27
#define rFPSR	r28
#define rPFS	r29
#define rLC	r30
#define rPR	r31
