





























#ifndef AssemblerBufferWithConstantPool_h
#define AssemblerBufferWithConstantPool_h

#include "assembler/wtf/Platform.h"

#if ENABLE_ASSEMBLER

#include "AssemblerBuffer.h"
#include "assembler/wtf/SegmentedVector.h"
#include "assembler/wtf/Assertions.h"

#include "methodjit/Logging.h"

#define ASSEMBLER_HAS_CONSTANT_POOL 1

namespace JSC {















































template <int maxPoolSize, int barrierSize, int maxInstructionSize, class AssemblerType>
class AssemblerBufferWithConstantPool: public AssemblerBuffer {
    typedef SegmentedVector<uint32_t, 512> LoadOffsets;
public:
    enum {
        UniqueConst,
        ReusableConst,
        UnusedEntry
    };

    AssemblerBufferWithConstantPool()
        : AssemblerBuffer()
        , m_numConsts(0)
        , m_maxDistance(maxPoolSize)
        , m_lastConstDelta(0)
        , m_flushCount(0)
    {
        m_pool = static_cast<uint32_t*>(malloc(maxPoolSize));
        m_mask = static_cast<char*>(malloc(maxPoolSize / sizeof(uint32_t)));
    }

    ~AssemblerBufferWithConstantPool()
    {
        free(m_mask);
        free(m_pool);
    }

    void ensureSpace(int space)
    {
        flushIfNoSpaceFor(space);
        AssemblerBuffer::ensureSpace(space);
    }

    void ensureSpace(int insnSpace, int constSpace)
    {
        flushIfNoSpaceFor(insnSpace, constSpace);
        AssemblerBuffer::ensureSpace(insnSpace);
    }

    bool isAligned(int alignment)
    {
        flushIfNoSpaceFor(alignment);
        return AssemblerBuffer::isAligned(alignment);
    }

    void putByteUnchecked(int value)
    {
        AssemblerBuffer::putByteUnchecked(value);
        correctDeltas(1);
    }

    void putByte(int value)
    {
        flushIfNoSpaceFor(1);
        AssemblerBuffer::putByte(value);
        correctDeltas(1);
    }

    void putShortUnchecked(int value)
    {
        AssemblerBuffer::putShortUnchecked(value);
        correctDeltas(2);
    }

    void putShort(int value)
    {
        flushIfNoSpaceFor(2);
        AssemblerBuffer::putShort(value);
        correctDeltas(2);
    }

    void putIntUnchecked(int value)
    {
        AssemblerBuffer::putIntUnchecked(value);
        correctDeltas(4);
    }

    void putInt(int value)
    {
        flushIfNoSpaceFor(4);
        AssemblerBuffer::putInt(value);
        correctDeltas(4);
    }

    void putInt64Unchecked(int64_t value)
    {
        AssemblerBuffer::putInt64Unchecked(value);
        correctDeltas(8);
    }

    int size()
    {
        flushIfNoSpaceFor(maxInstructionSize, sizeof(uint64_t));
        return AssemblerBuffer::size();
    }

    int uncheckedSize()
    {
        return AssemblerBuffer::size();
    }

    void* executableAllocAndCopy(ExecutableAllocator* allocator, ExecutablePool** poolp, CodeKind kind)
    {
        flushConstantPool(false);
        return AssemblerBuffer::executableAllocAndCopy(allocator, poolp, kind);
    }

    void putIntWithConstantInt(uint32_t insn, uint32_t constant, bool isReusable = false)
    {
        flushIfNoSpaceFor(4, 4);

        m_loadOffsets.append(AssemblerBuffer::size());
        if (isReusable)
            for (int i = 0; i < m_numConsts; ++i) {
                if (m_mask[i] == ReusableConst && m_pool[i] == constant) {
                    AssemblerBuffer::putInt(AssemblerType::patchConstantPoolLoad(insn, i));
                    correctDeltas(4);
                    return;
                }
            }

        m_pool[m_numConsts] = constant;
        m_mask[m_numConsts] = static_cast<char>(isReusable ? ReusableConst : UniqueConst);

        AssemblerBuffer::putInt(AssemblerType::patchConstantPoolLoad(insn, m_numConsts));
        ++m_numConsts;

        correctDeltas(4, 4);
    }

    
    void flushWithoutBarrier(bool isForced = false)
    {
        
        if (isForced || (5 * m_numConsts * sizeof(uint32_t)) > (3 * maxPoolSize))
            flushConstantPool(false);
    }

    uint32_t* poolAddress()
    {
        return m_pool;
    }

    int sizeOfConstantPool()
    {
        return m_numConsts;
    }

    int flushCount()
    {
        return m_flushCount;
    }

private:
    void correctDeltas(int insnSize)
    {
        m_maxDistance -= insnSize;
        ASSERT(m_maxDistance >= 0);
        m_lastConstDelta -= insnSize;
        if (m_lastConstDelta < 0)
            m_lastConstDelta = 0;
    }

    void correctDeltas(int insnSize, int constSize)
    {
        correctDeltas(insnSize);

        m_maxDistance -= m_lastConstDelta;
        ASSERT(m_maxDistance >= 0);
        m_lastConstDelta = constSize;
    }

    void flushConstantPool(bool useBarrier = true)
    {
        js::JaegerSpew(js::JSpew_Insns, " -- FLUSHING CONSTANT POOL WITH %d CONSTANTS --\n",
                       m_numConsts);
        if (m_numConsts == 0)
            return;
        m_flushCount++;
        int alignPool = (AssemblerBuffer::size() + (useBarrier ? barrierSize : 0)) & (sizeof(uint64_t) - 1);

        if (alignPool)
            alignPool = sizeof(uint64_t) - alignPool;

        
        if (useBarrier)
            AssemblerBuffer::putInt(AssemblerType::placeConstantPoolBarrier(m_numConsts * sizeof(uint32_t) + alignPool));

        if (alignPool) {
            if (alignPool & 1)
                AssemblerBuffer::putByte(AssemblerType::padForAlign8);
            if (alignPool & 2)
                AssemblerBuffer::putShort(AssemblerType::padForAlign16);
            if (alignPool & 4)
                AssemblerBuffer::putInt(AssemblerType::padForAlign32);
        }

        int constPoolOffset = AssemblerBuffer::size();
        append(reinterpret_cast<char*>(m_pool), m_numConsts * sizeof(uint32_t));

        
        for (LoadOffsets::Iterator iter = m_loadOffsets.begin(); iter != m_loadOffsets.end(); ++iter) {
            void* loadAddr = reinterpret_cast<void*>(m_buffer + *iter);
            AssemblerType::patchConstantPoolLoad(loadAddr, reinterpret_cast<void*>(m_buffer + constPoolOffset));
        }

        m_loadOffsets.clear();
        m_numConsts = 0;
        m_maxDistance = maxPoolSize;
        ASSERT(m_maxDistance >= 0);

    }

    void flushIfNoSpaceFor(int nextInsnSize)
    {
        if (m_numConsts == 0) {
            m_maxDistance = maxPoolSize;
            return;
        }
        int lastConstDelta = m_lastConstDelta > nextInsnSize ? m_lastConstDelta - nextInsnSize : 0;
        if ((m_maxDistance < nextInsnSize + lastConstDelta + barrierSize + (int)sizeof(uint32_t)))
            flushConstantPool();
    }

    void flushIfNoSpaceFor(int nextInsnSize, int nextConstSize)
    {
        if (m_numConsts == 0) {
            m_maxDistance = maxPoolSize;
            return;
        }
        if ((m_maxDistance < nextInsnSize + m_lastConstDelta + nextConstSize + barrierSize + (int)sizeof(uint32_t)) ||
            (m_numConsts * sizeof(uint32_t) + nextConstSize >= maxPoolSize))
            flushConstantPool();
    }

    uint32_t* m_pool;
    char* m_mask;
    LoadOffsets m_loadOffsets;

    int m_numConsts;
    int m_maxDistance;
    int m_lastConstDelta;
    int m_flushCount;
};

} 

#endif 

#endif 
