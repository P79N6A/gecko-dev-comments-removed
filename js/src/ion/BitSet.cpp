








































#include "jsutil.h"
#include "BitSet.h"

using namespace js;
using namespace js::ion;

BitSet *
BitSet::New(unsigned int max)
{
    BitSet *result = new BitSet(max);
    if (!result->init())
        return NULL;
    return result;
}

bool
BitSet::init()
{
    size_t sizeRequired = numWords() * sizeof(*bits_);

    TempAllocator *alloc = GetIonContext()->temp;
    bits_ = (unsigned long *)alloc->allocate(sizeRequired);
    if (!bits_)
        return false;

    memset(bits_, 0, sizeRequired);

    return true;
}

bool
BitSet::contains(unsigned int value) const
{
    JS_ASSERT(bits_);
    JS_ASSERT(value <= max_);

    return bits_[wordForValue(value)] & bitForValue(value);
}

bool
BitSet::empty() const
{
    JS_ASSERT(bits_);
    for (unsigned int i = 0; i < numWords(); i++) {
        if (bits_[i])
            return false;
    }
    return true;
}

void
BitSet::insert(unsigned int value)
{
    JS_ASSERT(bits_);
    JS_ASSERT(value <= max_);

    bits_[wordForValue(value)] |= bitForValue(value);
}

void
BitSet::insertAll(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->max_ == max_);
    JS_ASSERT(other->bits_);

    for (unsigned int i = 0; i < numWords(); i++)
        bits_[i] |= other->bits_[i];
}

void
BitSet::remove(unsigned int value)
{
    JS_ASSERT(bits_);
    JS_ASSERT(value <= max_);

    bits_[wordForValue(value)] &= ~bitForValue(value);
}

void
BitSet::removeAll(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->max_ == max_);
    JS_ASSERT(other->bits_);

    for (unsigned int i = 0; i < numWords(); i++)
        bits_[i] &= ~other->bits_[i];
}

void
BitSet::intersect(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->max_ == max_);
    JS_ASSERT(other->bits_);

    for (unsigned int i = 0; i < numWords(); i++)
        bits_[i] &= other->bits_[i];
}


bool
BitSet::fixedPointIntersect(const BitSet *other)
{
    JS_ASSERT(bits_);
    JS_ASSERT(other->max_ == max_);
    JS_ASSERT(other->bits_);

    bool changed = false;

    for (unsigned int i = 0; i < numWords(); i++) {
        unsigned long old = bits_[i];
        bits_[i] &= other->bits_[i];

        if (!changed && old != bits_[i])
            changed = true;
    }
    return changed;
}

void
BitSet::complement()
{
    JS_ASSERT(bits_);
    for (unsigned int i = 0; i < numWords(); i++)
        bits_[i] = ~bits_[i];
}

BitSet::Iterator
BitSet::begin()
{
    return Iterator(*this, 0);
}

BitSet::Iterator
BitSet::end()
{
    return Iterator(*this, max_ + 1);
}
