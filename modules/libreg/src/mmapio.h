




































#include "prio.h"

typedef struct MmioFileStruct MmioFile;

PRStatus mmio_FileSeek(MmioFile *file, PRInt32 offset, PRSeekWhence whence);
PRInt32  mmio_FileRead(MmioFile *file, char *dest, PRInt32 count);
PRInt32  mmio_FileWrite(MmioFile *file, const char *src, PRInt32 count);
PRInt32  mmio_FileTell(MmioFile *file);
PRStatus mmio_FileClose(MmioFile *file);
MmioFile *mmio_FileOpen(char *path, PRIntn flags, PRIntn mode);
