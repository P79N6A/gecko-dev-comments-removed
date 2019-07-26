






















#include "filestrm.h"

#include "cmemory.h"

#include <stdio.h>

U_CAPI FileStream* U_EXPORT2
T_FileStream_open(const char* filename, const char* mode)
{
    if(filename != NULL && *filename != 0 && mode != NULL && *mode != 0) {
        FILE *file = fopen(filename, mode);
        return (FileStream*)file;
    } else {
        return NULL;
    }
}
































U_CAPI void U_EXPORT2
T_FileStream_close(FileStream* fileStream)
{
    if (fileStream != 0)
        fclose((FILE*)fileStream);
}

U_CAPI UBool U_EXPORT2
T_FileStream_file_exists(const char* filename)
{
    FILE* temp = fopen(filename, "r");
    if (temp) {
        fclose(temp);
        return TRUE;
    } else
        return FALSE;
}













U_CAPI int32_t U_EXPORT2
T_FileStream_read(FileStream* fileStream, void* addr, int32_t len)
{
    return fread(addr, 1, len, (FILE*)fileStream);
}

U_CAPI int32_t U_EXPORT2
T_FileStream_write(FileStream* fileStream, const void* addr, int32_t len)
{

    return fwrite(addr, 1, len, (FILE*)fileStream);
}

U_CAPI void U_EXPORT2
T_FileStream_rewind(FileStream* fileStream)
{
    rewind((FILE*)fileStream);
}

U_CAPI int32_t U_EXPORT2
T_FileStream_putc(FileStream* fileStream, int32_t ch)
{
    int32_t c = fputc(ch, (FILE*)fileStream);
    return c;
}

U_CAPI int U_EXPORT2
T_FileStream_getc(FileStream* fileStream)
{
    int c = fgetc((FILE*)fileStream);
    return c;
}

U_CAPI int32_t U_EXPORT2
T_FileStream_ungetc(int32_t ch, FileStream* fileStream)
{

    int32_t c = ungetc(ch, (FILE*)fileStream);
    return c;
}

U_CAPI int32_t U_EXPORT2
T_FileStream_peek(FileStream* fileStream)
{
    int32_t c = fgetc((FILE*)fileStream);
    return ungetc(c, (FILE*)fileStream);
}

U_CAPI char* U_EXPORT2
T_FileStream_readLine(FileStream* fileStream, char* buffer, int32_t length)
{
    return fgets(buffer, length, (FILE*)fileStream);
}

U_CAPI int32_t U_EXPORT2
T_FileStream_writeLine(FileStream* fileStream, const char* buffer)
{
    return fputs(buffer, (FILE*)fileStream);
}

U_CAPI int32_t U_EXPORT2
T_FileStream_size(FileStream* fileStream)
{
    int32_t savedPos = ftell((FILE*)fileStream);
    int32_t size = 0;

    

    fseek((FILE*)fileStream, 0, SEEK_END);
    size = (int32_t)ftell((FILE*)fileStream);
    fseek((FILE*)fileStream, savedPos, SEEK_SET);
    return size;
}

U_CAPI int U_EXPORT2
T_FileStream_eof(FileStream* fileStream)
{
    return feof((FILE*)fileStream);
}







U_CAPI int U_EXPORT2
T_FileStream_error(FileStream* fileStream)
{
    return (fileStream == 0 || ferror((FILE*)fileStream));
}










U_CAPI FileStream* U_EXPORT2
T_FileStream_stdin(void)
{
    return (FileStream*)stdin;
}

U_CAPI FileStream* U_EXPORT2
T_FileStream_stdout(void)
{
    return (FileStream*)stdout;
}


U_CAPI FileStream* U_EXPORT2
T_FileStream_stderr(void)
{
    return (FileStream*)stderr;
}

U_CAPI UBool U_EXPORT2
T_FileStream_remove(const char* fileName){
    return (remove(fileName) == 0);
}
