






#include "SkMMapStream.h"

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

SkMMAPStream::SkMMAPStream(const char filename[])
{
    fAddr = NULL;   

    int fildes = open(filename, O_RDONLY);
    if (fildes < 0)
    {
        SkDEBUGF(("---- failed to open(%s) for mmap stream error=%d\n", filename, errno));
        return;
    }

    off_t offset = lseek(fildes, 0, SEEK_END);    
    if (offset == -1)
    {
        SkDEBUGF(("---- failed to lseek(%s) for mmap stream error=%d\n", filename, errno));
        close(fildes);
        return;
    }
    (void)lseek(fildes, 0, SEEK_SET);   

    
    size_t size = static_cast<size_t>(offset);

    void* addr = mmap(NULL, size, PROT_READ, MAP_SHARED, fildes, 0);

    
    
    
    
    close(fildes);

    if (MAP_FAILED == addr)
    {
        SkDEBUGF(("---- failed to mmap(%s) for mmap stream error=%d\n", filename, errno));
        return;
    }

    this->INHERITED::setMemory(addr, size);

    fAddr = addr;
    fSize = size;
}

SkMMAPStream::~SkMMAPStream()
{
    this->closeMMap();
}

void SkMMAPStream::setMemory(const void* data, size_t length, bool copyData)
{
    this->closeMMap();
    this->INHERITED::setMemory(data, length, copyData);
}

void SkMMAPStream::closeMMap()
{
    if (fAddr)
    {
        munmap(fAddr, fSize);
        fAddr = NULL;
    }
}

