















































#ifndef _NR_BUFIO_H_
#define _NR_BUFIO_H_

typedef struct BufioFileStruct BufioFile;

BufioFile*  bufio_Open(const char* name, const char* mode);
int         bufio_Close(BufioFile* file);
int         bufio_Seek(BufioFile* file, PRInt32 offset, int whence);
PRUint32    bufio_Read(BufioFile* file, char* dest, PRUint32 count);
PRUint32    bufio_Write(BufioFile* file, const char* src, PRUint32 count);
PRInt32     bufio_Tell(BufioFile* file);
int         bufio_Flush(BufioFile* file);
int         bufio_SetBufferSize(BufioFile* file, int bufsize);

#endif  

