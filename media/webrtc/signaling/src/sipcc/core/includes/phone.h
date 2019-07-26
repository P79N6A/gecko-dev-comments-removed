






































#ifndef PHONE_H
#define PHONE_H

#include "text_strings.h"

#include "sessionConstants.h"


typedef enum {
    DEVICE_RESET   = CC_DEVICE_RESET,
    DEVICE_RESTART = CC_DEVICE_RESTART,
    ICMP_UNREACHABLE_RESET = CC_DEVICE_ICMP_UNREACHABLE
} DeviceResetType;





typedef int32_t irx_lst_blk;
#if defined SIP_OS_WINDOWS




typedef int32_t irx_tsk_blk;
typedef int32_t irx_tmr_buf;

#include "tnpphone.h"
#endif


#define APP_MAX_SIZE  0x60000L
#define MAX_CTL_FILE_SIZE 0x8000L

#define disableInt IRXDISAB
#define enableInt IRXENAB
#define intrOff IRXHRDDIS
#define intrOn  IRXHRDENA

#define MAX_LINES_7940       2
#define MAX_LINES_7960       6
#define DSP_LOAD_ID_MAX      28
#define OLD_DSP_LOAD_ID_MAX  8




#define TICKS_PER_SECOND                 100      /* 10ms. per tick */
#define SECONDS_TO_TICKS(SECS)           ((SECS) * TICKS_PER_SECOND)
#define TICKS_TO_SECONDS(TICKS)          ((TICKS)/ TICKS_PER_SECOND)
#define MILLISECONDS_TO_TICKS(MILLISECS) (((MILLISECS)*(TICKS_PER_SECOND))/1000)

enum {
    DSP_RST,
    GPIO_A = 4,
    GPIO_B,
    GPIO_C,
    GPIO_D,
    gpioMax
};

typedef struct {
    uint8_t  loadid[256];
    uint8_t  apploadid[8];
    uint32_t chksum;    
    uint32_t applen;    
    uint32_t appcmp;    
    uint32_t cmpchksum; 
    uint32_t cmplen;    
} LoadHdr;

typedef struct {
    uint8_t  apploadid[DSP_LOAD_ID_MAX];
    uint32_t chksum;    
    uint32_t applen;    
    uint32_t appcmp;    
    uint32_t cmpchksum; 
    uint32_t cmplen;    

} DSPLoadHdr;

typedef struct {
    uint8_t  apploadid[OLD_DSP_LOAD_ID_MAX];
    uint32_t chksum;    
    uint32_t applen;    
    uint32_t appcmp;    
    uint32_t cmpchksum; 
    uint32_t cmplen;    

} LegacyDSPLoadHdr;

typedef struct {
    uint16_t valid;
    uint16_t notused[3];
    uint16_t hw_ver_hi;
    uint16_t hw_ver_low;
    uint8_t  mac[6];
    int8_t   model[20];
    uint16_t feature_bits;
    uint16_t ext;
    uint16_t license[20];
    int8_t   serial_number[20];
} MfgBlk;

#define GPIO_IO_SEL_SHIFT 8

#define GPIOA_LBK_ON      1
#define GPIOA_MSG_LED     4
#define GPIOA_SPKR_LED    5
#define GPIOA_MUTE_LED    6
#define GPIOA_HDST_LED    7
#define GPIOA_IO_MASK     ((1<<(GPIOA_LBK_ON+GPIO_IO_SEL_SHIFT))|\
                          (1<<(GPIOA_MSG_LED+GPIO_IO_SEL_SHIFT))|\
                          (1<<(GPIOA_SPKR_LED+GPIO_IO_SEL_SHIFT))|\
                          (1<<(GPIOA_MUTE_LED+GPIO_IO_SEL_SHIFT))|\
                          (1<<(GPIOA_HDST_LED+GPIO_IO_SEL_SHIFT)))

#define GPIOD_HANDSET     0
#define GPIOD_SPKER       1
#define GPIOD_PHY_MDC     3
#define GPIOD_PHY_MDIO    4
#define GPIOD_IO_RST_HI   5
#define GPIOD_IO_RST_LO   6
#define GPIOD_IO_MASK     0xff00

typedef volatile struct {
    uint16_t reg;
    uint16_t unused;
} s_rbbreg;

enum {
    rbbTimeout,
    rbbAPIWs,
    rbbWbufEn,
    rbbAccessFactor0,
    rbbAccessFactor1,
    rbbAccessFactor2,
    rbbAccessFactor3,
    rbbIntrSw,
    rbbMax
};

extern s_rbbreg RBBReg[rbbMax];

#define DCFG_PROGRAMMED   0x12300000L
#define DCFG_FIXED_VLAN   0x00000001L
#define DCFG_UI_SETTING   0x00000002L

#define IF_MAX 1
#define MAX_PORT 2
enum { PORT_UP, PORT_DOWN };

#define KA_TICKS 0  //Default to 10 seconds

typedef struct {
    uint32_t Active,
             IP,
             SSRC,
             PktIn,
             OctIn,
             Delay,
             Cycle,
             Tick,
             Time,
             Mcast;
    uint16_t Seq,
             First;
    int32_t  Jitter,
             Trans;
} _s_talkers;

#define TX_CONNECT 1
#define RX_CONNECT 2


#define IF_2MPS  1L
#define IF_10MPS 2L


#define KEYSCAN         0x00000001L
#define HOOKSCAN        0x00000002L
#define TIMETICK        0x00000004L

#define ON 1
#define OFF 0
#define TALK 2
#define SIZEOF_UISET 16

typedef struct {
    uint8_t diag;
    uint32_t status;
    uint16_t vlan;
    uint16_t vvlan;
    uint16_t port;
    uint16_t uiset[SIZEOF_UISET];
    uint16_t vvlanAdmin;
    uint16_t vvlanIP;     
    uint8_t unused[2001]; 
} DeviceCfgBlk;



#define MAX_NAME (64)
#define BACKUP_SERVERS (7)
#define MAX_DHCP_TFTP_FILE_NAME (128)
#define DISKFULL       3      // file too large
#define NOERR          8      // no error
typedef struct
{
    int8_t   tftp_file[40];
    uint32_t status;
    uint32_t control;
    uint32_t appStatus;
    uint32_t appControl;
    uint32_t tftp_backup;
} DHCPBlkExt;

typedef struct
{
    uint32_t status;      
    uint32_t my_ip_addr;  
    uint32_t subnet_mask; 
    uint32_t defaultgw;   
    uint32_t dns_addr;
    uint32_t tftp_addr;
    uint8_t  my_mac_addr[6];
    int8_t   domain_name[MAX_NAME];
    int8_t   my_name[MAX_NAME];
    uint32_t dhcp_server;
    uint32_t spare;
    uint32_t dns_backup[BACKUP_SERVERS];
    uint32_t gw_backup[BACKUP_SERVERS];
    int8_t   tftp_name[MAX_NAME];
    union {
        int8_t tftp_file[MAX_DHCP_TFTP_FILE_NAME];
        DHCPBlkExt ext;
    } extFields;
} DHCPBlk;















typedef struct {
    DHCPBlk      dhcp_blk;           
    DeviceCfgBlk device_cfg_blk;     





    int8_t   prot_signature[0x0004]; 


    int8_t   prot_cfg_area[0x1654];  

    uint32_t prot_cfg_offset;        
} cfg_rom_t;

typedef struct {
    int8_t   prot_signature[0x0004]; 
    int8_t   personal_dir[0xDFFC];   
    int8_t   dial_signature[0x0004]; 
    int8_t   dial_plan[0x1FF0];      
    uint32_t unused_offset2;         
    uint32_t dial_plan_offset;       
    uint32_t prot_dir_offset;        
} dir_rom_t;
#define DIR_CONFIG_SIGNATURE "DIR0"
#define CONFIG_MAX (0x2000 - sizeof(DHCPBlk) - sizeof(DeviceCfgBlk))


extern volatile uint32_t WDTimer;
extern volatile uint16_t GPIO1[gpioMax];
extern DHCPBlk DHCPInfo;        
extern LoadHdr Load;            

#ifdef _WIN32
extern uint8_t LoadArea[0xFFFF];
extern uint8_t BootHeader[9];   
#else
extern uint8_t LoadArea;        
extern int8_t BootHeader;
#endif
extern LoadHdr LoadHeader;      
extern DHCPBlk DHCPInfoRom;     
extern DHCPBlk DHCPInfoTemp;    
extern MfgBlk  MfgInfo;         
extern LoadHdr App1Hdr;         
extern LoadHdr App2Hdr;         
extern uint8_t App1Rom;         
extern uint8_t App2Rom;         
extern DSPLoadHdr DspHeader;
extern dir_rom_t DirStorage;    
extern uint32_t DcmpVer;        
extern uint16_t DCVerReg;


typedef struct {
    int16_t displayContrast;
    int16_t ringType;
    int16_t handsetVolume;
    int16_t headsetVolume;
    int16_t speakerVolume;
    int16_t ringVolume;
    int16_t reserved[10];
} t_SettingsDesc;

extern int16_t SavePhoneSettings;


enum {
    STATE_INIT_CFG,
    STATE_CDP_CFG,
    STATE_IP_CFG,
    STATE_FILE_CFG,
    STATE_CFG_UPDATE,
    STATE_CONNECTING,
    STATE_REGISTER,
    STATE_REGISTER_REJ,
    STATE_LOADID_REQ,
    STATE_BOOT_DSP,
    STATE_LOADING,
    STATE_CONNECTED,
    STATE_RESETTING,
    STATE_IP_RELEASE,
    STATE_DUP_IP,
    STATE_DONE_LOADING,
    STATE_UNPROVISIONED,   
    maxPHNState
};

enum { CDP_CACHE_P0_CLEAR, CDP_CACHE_P1_CLEAR, CDP_CACHE_CLEAN };


#define DC_NONE 0
#define DC_C3PO 1
#define DC_ETH  3
#define DC_2MPS 7

#define NO_TAGGING 4095
#define NO_VID     4096




#define  SYSBUF_CNT  50
#define  DSPBUF_CNT  30
#define  SIPTMRBUF_CNT 32
#define  SIPBUF_CNT MAX(MAX_TEL_LINES, 32)
#define  GSMBUF_CNT  32
#define  GUITMRBUF_CNT  5
#define  TCPBUF_CNT    6  // for use by telnet etc
#define  PKTBUF_TX_CNT 10
#define  ENET_RX_BUF_CNT 10
#define  PKTBUF_CNT  (ENET_RX_BUF_CNT + PKTBUF_TX_CNT)  //20






#define  SYSBUF_SIZ  512
#define  GUITMRBUF_SIZ 32
#define  PKTBUF_SIZ  3072
#define  TCPBUF_SIZ 2560
#define  DSPBUF_SIZ 96
#define  GUIBUF_SIZ 512










#define  SYS_BUFFER    1     //System Buffer
#define  XMT_BUFFER    2     //Ether Transmit Packet Ram Buffer
#define  RCV_BUFFER    3     //Ether Receive Packet Ram Buffer
#define  TCP_BUFFER    4
#define  DSP_BUFFER    5
#define  GUITMR_BUFFER 6
#define  CPR_BUFFER    256

extern uint16_t ResetPend;
extern uint32_t gtick;
extern uint16_t ResetTimer; 
extern uint8_t LinkState[MAX_PORT];
extern uint16_t LinkStatus[MAX_PORT];
extern uint8_t CurrentLinkState[MAX_PORT];
extern uint16_t CurrentMode[MAX_PORT];

extern int16_t CallInProg;
extern int32_t BugTrapped;

extern MfgBlk *pMfgInfo;
extern DeviceCfgBlk *pDeviceInfo, *pDeviceInfoRom;
extern LoadHdr *pMyLoad, *pPriLoad, *pSecLoad;
extern int8_t *pMyPlatform;
extern int8_t *pMyPhoneModel;
extern DSPLoadHdr *pMyDSPLoad;

extern int32_t PhoneStim;
extern int32_t PhoneHookStim;

extern uint32_t IfActive;
extern volatile uint16_t VVLan, DCDetect, C3PODetected;
extern uint32_t MaxPort;

extern uint32_t BCastMax;
extern boolean Is7940;
extern boolean Is79x0G;

void PHNChangeState(uint16_t state);
uint16_t PHNGetState(void);
int32_t PHNStateToStringIndex(uint16_t PhoneState);
void PHNSpeakerMode(uint16_t mode);
void PHNMuteMode(uint16_t mode);
void PHNHeadsetMode(uint16_t mode);
void PHNRequestFile(int8_t *pName, uint8_t *pDest, int32_t maxSize);
void PHNMessageLedMode(uint16_t mode);
void PHNRingerLedMode(uint16_t mode);
void phone_reset(DeviceResetType resetType);
void platform_reset_req(DeviceResetType resetType);
void platform_sync_cfg_vers(char *configVersionStamp,
                            char *dialplanVersionStamp, char *);
int32_t phone_get_keypress_rand_seed(void);
int16_t phone_call_in_progress(void);

void tweak_watchdog(void);
extern uint32_t set_wd_timeout(uint16_t new_time);
extern void restore_wd_timeout(uint32_t prev_time);

#define DEFAULT_WATCHDOG_TIME 8

#define PROGRAM_HW_WATCHDOG_IN_MILLISECONDS(x); WDTimer = 0x00550000; \
            WDTimer = 0x01AA0000 + ((x)); //Program the watchdog

#define HIT_THE_WATCHDOG() tweak_watchdog()


void BugInfoValidate(void);
uint32_t BugInfoAbortPtr(void);


int8_t *task2string(int32_t task_id);

#define EXCEPTION_BUGCODE   0xdd //BugTrap Code:0xddxx for exceptions
#define INITSTKSZ   500
#define SVCSTKSZ    40      // interrupt off (svc) stack size





#define MISC_LN 8
typedef struct
{
    void    *Next;
    uint32_t Cmd;
    uint16_t RetID;
    uint16_t Port;
    uint16_t Len;
    uint8_t *Data;
    union {
        void *UsrPtr;
        uint32_t UsrInfo;
    } Usr;
    uint8_t Misc[MISC_LN];
 
} phn_syshdr_t;




typedef phn_syshdr_t SysHdr;


extern DHCPBlk *pDHCPInfoTemp;

extern SysHdr *pCFGTftpReq;

typedef struct {
    char newcfgVerStamp[64];
    char newdialplanVerStamp[64];
} PhoneSyncCfgVer;

#endif 
