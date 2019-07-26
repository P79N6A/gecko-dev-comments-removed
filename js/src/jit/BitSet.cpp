





#include "jit/BitSet.h"

using namespace js;
using namespace js::jit;

BitSet *
BitSet::New(TempAllocator &alloc, unsigned int numBits)
{
    BitSet *result = new(alloc) BitSet(numBits);
    if (!result->init(alloc))
        return nullptr;
    return result;
}

bool
BitSet::init(TempAllocator &alloc)
{
    size_t sizeRequired = numWords() * sizeof(*bits_);

    bits_ = (uint32_t *)alloc.allocate(sizeRequired);
    if (!bits_)
        return false;

    memset(bits_, 0, sizeRequired);

    return true;
}

bool
BitSet::empty() const
{
    JS_ASSERT(bits_);
    const uint32_t *bits = bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++) {
        if (bits[i])
            return false;
    }
    return true;
}

void
BitSet::insertAll(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->numBits_ == numBits_);
    JS_ASSERT(other->bits_);

    uint32_t *bits = bits_;
    const uint32_t *otherBits = other->bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++)
        bits[i] |= otherBits[i];
}

void
BitSet::removeAll(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->numBits_ == numBits_);
    JS_ASSERT(other->bits_);

    uint32_t *bits = bits_;
    const uint32_t *otherBits = other->bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++)
        bits[i] &= ~otherBits[i];
}

void
BitSet::intersect(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->numBits_ == numBits_);
    JS_ASSERT(other->bits_);

    uint32_t *bits = bits_;
    const uint32_t *otherBits = other->bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++)
        bits[i] &= otherBits[i];
}


bool
BitSet::fixedPointIntersect(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->numBits_ == numBits_);
    JS_ASSERT(other->bits_);

    bool changed = false;

    uint32_t *bits = bits_;
    const uint32_t *otherBits = other->bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++) {
        uint32_t old = bits[i];
        bits[i] &= otherBits[i];

        if (!changed && old != bits[i])
            changed = true;
    }
    return changed;
}

void
BitSet::complement()
{
    JS_ASSERT(bits_);
    uint32_t *bits = bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++)
        bits[i] = ~bits[i];
}

void
BitSet::clear()
{
    JS_ASSERT(bits_);
    uint32_t *bits = bits_;
    for (unsigned int i = 0, e = numWords(); i < e; i++)
        bits[i] = 0;
}
