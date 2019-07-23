












































#pragma options align=mac68k

#ifndef _NS_APPLESINGLEDECODER_H_
#define _NS_APPLESINGLEDECODER_H_

#include <stdlib.h>
#include <string.h>
#include <Carbon/Carbon.h>





#define APPLESINGLE_MAGIC   0x00051600L
#define APPLESINGLE_VERSION 0x00020000L

typedef struct ASHeader  
{
  
  UInt32 magicNum;       
  UInt32 versionNum;     
  UInt8 filler[16];      
  UInt16 numEntries;    
} ASHeader ;  

typedef struct ASEntry  
{
  UInt32 entryID;     
  UInt32 entryOffset; 
                      
  UInt32 entryLength; 
} ASEntry; 

typedef struct ASFinderInfo
{
  FInfo ioFlFndrInfo;     
  FXInfo ioFlXFndrInfo;   
} ASFinderInfo; 

typedef struct ASMacInfo  
{
       UInt8 filler[3];    
       UInt8 ioFlAttrib;  
} ASMacInfo;

typedef struct ASFileDates  
{
  SInt32 create; 
  SInt32 modify; 
  SInt32 backup; 
  SInt32 access; 
} ASFileDates; 


#define AS_DATA         1 /* data fork */
#define AS_RESOURCE     2 /* resource fork */
#define AS_REALNAME     3 /* File's name on home file system */
#define AS_COMMENT      4 /* standard Mac comment */
#define AS_ICONBW       5 /* Mac black & white icon */
#define AS_ICONCOLOR    6 /* Mac color icon */

#define AS_FILEDATES    8 /* file dates; create, modify, etc */
#define AS_FINDERINFO   9 /* Mac Finder info & extended info */
#define AS_MACINFO      10 /* Mac file info, attributes, etc */
#define AS_PRODOSINFO   11 /* Pro-DOS file info, attrib., etc */
#define AS_MSDOSINFO    12 /* MS-DOS file info, attributes, etc */
#define AS_AFPNAME      13 /* Short name on AFP server */
#define AS_AFPINFO      14 /* AFP file info, attrib., etc */

#define AS_AFPDIRID     15 /* AFP directory ID */





#define MAC_ERR_CHECK(_funcCall)   \
  err = _funcCall;         \
  if (err!=noErr)         \
      return err;
  
  

class nsAppleSingleDecoder 
{

public:
  nsAppleSingleDecoder(const FSRef *inRef, FSRef *outRef);
  nsAppleSingleDecoder();
  ~nsAppleSingleDecoder();
    
  















  OSErr Decode(const FSRef *inRef, FSRef *outRef);
    
  








  OSErr Decode();
  
  









  OSErr DecodeFolder(const FSRef *aFolder);
     
  









  static Boolean IsAppleSingleFile(const FSRef *inRef);
  
  







  static Boolean IsDirectory(const FSRef *inRef);

  


  static Boolean UCstrcmp(const HFSUniStr255 *str1, const HFSUniStr255 *str2);
  
private:
  const FSRef *mInRef;
  FSRef       *mOutRef;
  
  SInt16      mInRefNum;  
  Boolean     mRenameReqd;
  
  OSErr  ProcessASEntry(ASEntry inEntry);  
  OSErr  ProcessDataFork(ASEntry inEntry);
  OSErr  ProcessResourceFork(ASEntry inEntry);
  OSErr  ProcessRealName(ASEntry inEntry);
  OSErr  ProcessFileDates(ASEntry inEntry);
  OSErr  ProcessFinderInfo(ASEntry inEntry);
  OSErr  EntryToMacFile(ASEntry inEntry, UInt16 inTargetSpecRefNum);

  OSErr  FSMakeUnique(const FSRef *inParentRef, FSRef *outRef);
};

#ifdef __cplusplus
extern "C" {
#endif

Boolean 
DecodeDirIterateFilter(Boolean containerChanged, ItemCount currentLevel,
  const FSCatalogInfo *catalogInfo, const FSRef *ref, 
  const FSSpec *spec, const HFSUniStr255 *name, void *yourDataPtr);

#ifdef __cplusplus
}
#endif

#pragma options align=reset

#endif
