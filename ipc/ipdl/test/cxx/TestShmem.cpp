#include "TestShmem.h"

#include "IPDLUnitTests.h"      


namespace mozilla {
namespace _ipdltest {




void
TestShmemParent::Main()
{
    Shmem mem;
    size_t size = 12345;
    if (!AllocShmem(size, &mem))
        fail("can't alloc shmem");

    if (mem.Size<char>() != size)
        fail("shmem is wrong size: expected %lu, got %lu",
             size, mem.Size<char>());

    char* ptr = mem.get<char>();
    memcpy(ptr, "Hello!", sizeof("Hello!"));
    if (!SendGive(mem, size))
        fail("can't send Give()");

    
    

    
    
}


bool
TestShmemParent::RecvTake(Shmem& mem, const size_t& expectedSize)
{
    if (mem.Size<char>() != expectedSize)
        fail("expected shmem size %lu, but it has size %lu",
             expectedSize, mem.Size<char>());

    if (strcmp(mem.get<char>(), "And yourself!"))
        fail("expected message was not written");

    Close();

    return true;
}




bool
TestShmemChild::RecvGive(Shmem& mem, const size_t& expectedSize)
{
    if (mem.Size<char>() != expectedSize)
        fail("expected shmem size %lu, but it has size %lu",
             expectedSize, mem.Size<char>());

    if (strcmp(mem.get<char>(), "Hello!"))
        fail("expected message was not written");

    memcpy(mem.get<char>(), "And yourself!", sizeof("And yourself!"));

    if (!SendTake(mem, expectedSize))
        fail("can't send Take()");

    return true;
}


} 
} 
