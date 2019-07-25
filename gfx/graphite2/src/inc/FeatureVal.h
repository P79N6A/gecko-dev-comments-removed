

























#pragma once
#include <cstring>
#include <cassert>
#include "inc/Main.h"
#include "inc/List.h"

namespace graphite2 {

class FeatureRef;
class FeatureMap;

class FeatureVal : public Vector<uint32>
{
public:
    uint32 maskedOr(const FeatureVal &other, const FeatureVal &mask) {
        uint32 len = size() ;
        if (other.size()<len) len = other.size();		
        if (mask.size()<len) len = mask.size();		
        for (uint32 i = 0; i < len; i++)
            if (mask[i]) operator[](i) = (operator[](i) & ~mask[i]) | (other[i] & mask[i]);
        return len;
    }

    FeatureVal() : m_pMap(0) { }
    FeatureVal(int num, const FeatureMap & pMap) : Vector<uint32>(num), m_pMap(&pMap) {}
    FeatureVal(const FeatureVal & rhs) : Vector<uint32>(rhs), m_pMap(rhs.m_pMap) {}

    bool operator ==(const FeatureVal & b) const
    {
        size_t n = size();
        if (n != b.size())      return false;
        
        for(const_iterator l = begin(), r = b.begin(); n && *l == *r; --n, ++l, ++r);
        
        return n == 0;
    }

    CLASS_NEW_DELETE
private:
    friend class FeatureRef;		
    const FeatureMap* m_pMap;
};

typedef FeatureVal Features;

} 


struct gr_feature_val : public graphite2::FeatureVal {};
