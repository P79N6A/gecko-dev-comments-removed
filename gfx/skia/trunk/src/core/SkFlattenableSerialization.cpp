






#include "SkFlattenableSerialization.h"

#include "SkData.h"
#include "SkValidatingReadBuffer.h"
#include "SkWriteBuffer.h"

SkData* SkValidatingSerializeFlattenable(SkFlattenable* flattenable) {
    SkWriteBuffer writer(SkWriteBuffer::kValidation_Flag);
    writer.writeFlattenable(flattenable);
    size_t size = writer.bytesWritten();
    void* data = sk_malloc_throw(size);
    writer.writeToMemory(data);
    return SkData::NewFromMalloc(data, size);
}

SkFlattenable* SkValidatingDeserializeFlattenable(const void* data, size_t size,
                                                  SkFlattenable::Type type) {
    SkValidatingReadBuffer buffer(data, size);
    return buffer.readFlattenable(type);
}
