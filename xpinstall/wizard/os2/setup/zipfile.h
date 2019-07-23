






































#ifndef _zipfile_h
#define _zipfile_h










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






PR_EXTERN(int) ZIP_OpenArchive( const char * zipname, void** hZip );
PR_EXTERN(int) ZIP_CloseArchive( void** hZip );






PR_EXTERN(int) ZIP_TestArchive( void* hZip );





PR_EXTERN(int) ZIP_ExtractFile( void* hZip, const char * filename, const char * outname );
















PR_EXTERN(void*) ZIP_FindInit( void* hZip, const char * pattern );
PR_EXTERN(int) ZIP_FindNext( void* hFind, char * outbuf, int bufsize );
PR_EXTERN(int) ZIP_FindFree( void* hFind );


PR_END_EXTERN_C

#endif 
