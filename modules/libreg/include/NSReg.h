








































#ifndef _NSREG_H_
#define _NSREG_H_

typedef void (*nr_RegPackCallbackFunc) (void *userData, int32 bytes, int32 totalBytes);

typedef int32   REGERR;
typedef int32   RKEY;
typedef uint32  REGENUM;
typedef void *  HREG;

typedef struct _reginfo
{
   uint16  size;        
   uint16  entryType;
   uint32  entryLength;
} REGINFO;

#define REGERR_OK           (0)
#define REGERR_FAIL         (1)
#define REGERR_NOMORE       (2)
#define REGERR_NOFIND       (3)
#define REGERR_BADREAD      (4)
#define REGERR_BADLOCN      (5)
#define REGERR_PARAM        (6)
#define REGERR_BADMAGIC     (7)
#define REGERR_BADCHECK     (8)
#define REGERR_NOFILE       (9)
#define REGERR_MEMORY       (10)
#define REGERR_BUFTOOSMALL  (11)
#define REGERR_NAMETOOLONG  (12)
#define REGERR_REGVERSION   (13)
#define REGERR_DELETED      (14)
#define REGERR_BADTYPE      (15)
#define REGERR_NOPATH       (16)
#define REGERR_BADNAME      (17)
#define REGERR_READONLY     (18)
#define REGERR_BADUTF8      (19)



#define MAXREGPATHLEN   (2048)

#define MAXREGNAMELEN   (512)

#define MAXREGVALUELEN  (0x7FFF)


#define ROOTKEY_USERS                   (0x01)
#define ROOTKEY_COMMON                  (0x02)
#define ROOTKEY_CURRENT_USER            (0x03)
#define ROOTKEY_PRIVATE                 (0x04)


#define REGENUM_NORMAL                  (0x00)
#define REGENUM_CHILDREN                REGENUM_NORMAL
#define REGENUM_DESCEND                 (0x01)
#define REGENUM_DEPTH_FIRST             (0x02)


#define REGTYPE_ENTRY                 (0x0010)
#define REGTYPE_ENTRY_STRING_UTF      (REGTYPE_ENTRY + 1)
#define REGTYPE_ENTRY_INT32_ARRAY     (REGTYPE_ENTRY + 2)
#define REGTYPE_ENTRY_BYTES           (REGTYPE_ENTRY + 3)
#define REGTYPE_ENTRY_FILE            (REGTYPE_ENTRY + 4)

#define REG_DELETE_LIST_KEY  "Mozilla/XPInstall/Delete List"
#define REG_REPLACE_LIST_KEY "Mozilla/XPInstall/Replace List"
#define REG_UNINSTALL_DIR    "Mozilla/XPInstall/Uninstall/"
#define REG_REPLACE_SRCFILE  "ReplacementFile"
#define REG_REPLACE_DESTFILE "DestinationFile"

#define UNINSTALL_NAV_STR "_"


#define UNIX_GLOBAL_FLAG     "MOZILLA_SHARED_REGISTRY"


#define VR_INTERFACE(type)     type

PR_BEGIN_EXTERN_C




















VR_INTERFACE(int) NR_RegSetBufferSize(
         HREG hReg,        
         int  bufsize
       );













VR_INTERFACE(REGERR) NR_RegOpen(
         const char *filename, 
         HREG *hReg            
       );











VR_INTERFACE(REGERR) NR_RegClose(
         HREG hReg         
       );










VR_INTERFACE(REGERR) NR_RegFlush(
         HREG hReg         
       );












VR_INTERFACE(REGERR) NR_RegIsWritable(
         HREG hReg         
       );

VR_INTERFACE(REGERR) NR_RegPack(
         HREG hReg,         
         void *userData,
         nr_RegPackCallbackFunc fn
       );














VR_INTERFACE(REGERR) NR_RegSetUsername(
         const char *name  
       );













VR_INTERFACE(REGERR) NR_RegGetUniqueName(
         HREG hReg,     
         char* outbuf,  
         uint32 buflen  
       );






VR_INTERFACE(REGERR) NR_RegGetUsername(
         char **name        
       );

























VR_INTERFACE(REGERR) NR_RegAddKey(
         HREG hReg,        
         RKEY key,         
         char *path,       
         RKEY *newKey      
       );

















VR_INTERFACE(REGERR) NR_RegAddKeyRaw(
         HREG hReg,        
         RKEY key,         
         char *keyname,    
         RKEY *newKey      
       );
















VR_INTERFACE(REGERR) NR_RegDeleteKey(
         HREG hReg,        
         RKEY key,         
         char *path        
       );














VR_INTERFACE(REGERR) NR_RegDeleteKeyRaw(
         HREG hReg,        
         RKEY key,         
         char *keyname     
       );













VR_INTERFACE(REGERR) NR_RegGetKey(
         HREG hReg,        
         RKEY key,         
         const char *path, 
         RKEY *result      
       );













VR_INTERFACE(REGERR) NR_RegGetKeyRaw(
         HREG hReg,        
         RKEY key,         
         char *keyname,       
         RKEY *result      
       );



















VR_INTERFACE(REGERR) NR_RegEnumSubkeys(
         HREG    hReg,        
         RKEY    key,         
         REGENUM *state,      
         char    *buffer,     
         uint32  bufsize,     
         uint32  style        
       );



















VR_INTERFACE(REGERR) NR_RegGetEntryInfo(
         HREG    hReg,     
         RKEY    key,      
         char    *name,    
         REGINFO *info     
       );

       












VR_INTERFACE(REGERR) NR_RegGetEntryString(
         HREG   hReg,      
         RKEY   key,       
         char   *name,     
         char   *buffer,   
         uint32 bufsize    
       );














VR_INTERFACE(REGERR) NR_RegGetEntry(
         HREG   hReg,      
         RKEY   key,       
         char   *name,     
         void   *buffer,   
         uint32 *size      
       );                  














VR_INTERFACE(REGERR) NR_RegSetEntryString(
         HREG hReg,        
         RKEY key,         
         char *name,       
         char *buffer      
       );















VR_INTERFACE(REGERR) NR_RegSetEntry(
         HREG   hReg,        
         RKEY   key,         
         char   *name,       
         uint16 type,        
         void   *buffer,     
         uint32 size         
       );











VR_INTERFACE(REGERR) NR_RegDeleteEntry(
         HREG hReg,        
         RKEY key,         
         char *name        
       );















VR_INTERFACE(REGERR) NR_RegEnumEntries(
         HREG    hReg,        
         RKEY    key,         
         REGENUM *state,      
         char    *buffer,     
         uint32  bufsize,     
         REGINFO *info        
       );


VR_INTERFACE(void)      NR_ShutdownRegistry(void);
VR_INTERFACE(REGERR)    NR_StartupRegistry(void);


PR_END_EXTERN_C

#endif   



