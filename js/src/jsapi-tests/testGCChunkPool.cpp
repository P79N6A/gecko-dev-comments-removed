






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
    for (js::gc::ChunkPool::Enum e(pool); !e.empty(); e.popFront(), ++i)
        CHECK(e.front());
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
    for (js::gc::ChunkPool::Enum e(pool); !e.empty(); e.popFront(), --offset) {
        if (offset == 0) {
            chunk = pool.remove(e.front());
            break;
        }
    }
    CHECK(chunk);
    MOZ_ASSERT(!pool.contains(chunk));
    MOZ_ASSERT(pool.verify());
    pool.push(chunk);

    
    js::AutoLockGC lock(rt);
    for (js::gc::ChunkPool::Enum e(pool); !e.empty();) {
        js::gc::Chunk *chunk = e.front();
        e.removeAndPopFront();
        js::gc::UnmapPages(chunk, js::gc::ChunkSize);
    }

    return true;
}
END_TEST(testGCChunkPool)
