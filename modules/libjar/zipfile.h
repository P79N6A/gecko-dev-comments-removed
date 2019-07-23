






































#ifndef _zipfile_h
#define _zipfile_h









#ifdef STANDALONE

#define ZIP_OK                 0
#define ZIP_ERR_GENERAL       -1
#define ZIP_ERR_MEMORY        -2
#define ZIP_ERR_DISK          -3
#define ZIP_ERR_CORRUPT       -4
#define ZIP_ERR_PARAM         -5
#define ZIP_ERR_FNF           -6
#define ZIP_ERR_UNSUPPORTED   -7
#define ZIP_ERR_SMALLBUF      -8

PR_BEGIN_EXTERN_C






PR_EXTERN(PRInt32) ZIP_OpenArchive( const char * zipname, void** hZip );
PR_EXTERN(PRInt32) ZIP_CloseArchive( void** hZip );






PR_EXTERN(PRInt32) ZIP_TestArchive( void* hZip );





PR_EXTERN(PRInt32) ZIP_ExtractFile( void* hZip, const char * filename, const char * outname );
















PR_EXTERN(void*) ZIP_FindInit( void* hZip, const char * pattern );
PR_EXTERN(PRInt32) ZIP_FindNext( void* hFind, char * outbuf, PRUint16 bufsize );
PR_EXTERN(PRInt32) ZIP_FindFree( void* hFind );

PR_END_EXTERN_C

#else

#define ZIP_OK                  NS_OK
#define ZIP_ERR_MEMORY          NS_ERROR_OUT_OF_MEMORY
#define ZIP_ERR_DISK            NS_ERROR_FILE_DISK_FULL
#define ZIP_ERR_CORRUPT         NS_ERROR_FILE_CORRUPTED
#define ZIP_ERR_PARAM           NS_ERROR_ILLEGAL_VALUE
#define ZIP_ERR_FNF             NS_ERROR_FILE_TARGET_DOES_NOT_EXIST
#define ZIP_ERR_UNSUPPORTED     NS_ERROR_NOT_IMPLEMENTED
#define ZIP_ERR_GENERAL         NS_ERROR_FAILURE

#endif 

#endif 
