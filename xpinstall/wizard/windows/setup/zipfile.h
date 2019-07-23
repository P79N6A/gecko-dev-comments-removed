






































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






extern __declspec(dllexport)int ZIP_OpenArchive( const char * zipname, void** hZip );
extern __declspec(dllexport)int ZIP_CloseArchive( void** hZip );






extern __declspec(dllexport)int ZIP_TestArchive( void* hZip );





extern __declspec(dllexport)int ZIP_ExtractFile( void* hZip, const char * filename, const char * outname );
















extern __declspec(dllexport)void* ZIP_FindInit( void* hZip, const char * pattern );
extern __declspec(dllexport)int ZIP_FindNext( void* hFind, char * outbuf, int bufsize );
extern __declspec(dllexport)int ZIP_FindFree( void* hFind );


PR_END_EXTERN_C

#endif 
