





































 
#ifndef SU_PAS_H
#define SU_PAS_H


#include <Errors.h>
#include <Types.h>
#include <Files.h>
#include <Script.h>
#include <Resources.h>

typedef struct PASHeader 
{
       UInt32 magicNum; 	
       UInt32 versionNum; 	
       UInt8 filler[16]; 	
       UInt16 numEntries; 	
} PASHeader ; 


typedef struct PASEntry 	
{
        UInt32 entryID; 	
        UInt32 entryOffset; 
                            
        UInt32 entryLength; 
        
} PASEntry; 


typedef struct PASMiscInfo
{
	short	fileHasResFork;
	short	fileResAttrs;
	OSType 	fileType;
	OSType	fileCreator;
	UInt32	fileFlags;

} PASMiscInfo; 


typedef struct PASResFork
{
	short	NumberOfTypes;
	
} PASResFork; 


typedef struct PASResource
{
	short			attr;
	short			attrID;
	OSType			attrType;
	Str255			attrName;
	unsigned long	length;
		
} PASResource; 



#if PRAGMA_STRUCT_ALIGN
#pragma options align=reset
#endif


#define kCreator	'MOSS'
#define kType		'PASf'
#define PAS_BUFFER_SIZE (1024*512)

#define PAS_MAGIC_NUM	(0x00244200)
#define PAS_VERSION		(0x00010000)

enum
{
	ePas_Data	=	1,
	ePas_Misc,
	ePas_Resource
};

#ifdef __cplusplus
extern "C" {
#endif


OSErr PAS_EncodeFile(FSSpec *inSpec, FSSpec *outSpec);
OSErr PAS_DecodeFile(FSSpec *inSpec, FSSpec *outSpec);



#ifdef __cplusplus
}
#endif

#endif
