






#include "mozilla/Move.h"

#include "gc/GCRuntime.h"
#include "gc/Heap.h"

#include "jsapi-tests/tests.h"

BEGIN_TEST(testGCChunkPool)
{
    const int N = 10;
    js::gc::ChunkPool pool;

    
    for (int i = 0; i < N; ++i) {
        js::gc::Chunk *chunk = js::gc::Chunk::allocate(rt);
        CHECK(chunk);
        pool.push(chunk);
    }
    MOZ_ASSERT(pool.verify());

    
    uint32_t i = 0;
    for (js::gc::ChunkPool::Iter iter(pool); !iter.done(); iter.next(), ++i)
        CHECK(iter.get());
    CHECK(i == pool.count());
    MOZ_ASSERT(pool.verify());

    
    for (int i = 0; i < N; ++i) {
        js::gc::Chunk *chunkA = pool.pop();
        js::gc::Chunk *chunkB = pool.pop();
        js::gc::Chunk *chunkC = pool.pop();
        pool.push(chunkA);
        pool.push(chunkB);
        pool.push(chunkC);
    }
    MOZ_ASSERT(pool.verify());

    
    js::gc::Chunk *chunk = nullptr;
    int offset = N / 2;
    for (js::gc::ChunkPool::Iter iter(pool); !iter.done(); iter.next(), --offset) {
        if (offset == 0) {
            chunk = pool.remove(iter.get());
            break;
        }
    }
    CHECK(chunk);
    MOZ_ASSERT(!pool.contains(chunk));
    MOZ_ASSERT(pool.verify());
    pool.push(chunk);

    
    js::AutoLockGC lock(rt);
    for (js::gc::ChunkPool::Iter iter(pool); !iter.done();) {
        js::gc::Chunk *chunk = iter.get();
        iter.next();
        pool.remove(chunk);
        js::gc::UnmapPages(chunk, js::gc::ChunkSize);
    }

    return true;
}
END_TEST(testGCChunkPool)
