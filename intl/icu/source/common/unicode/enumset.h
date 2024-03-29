













#ifndef ENUMSET_H
#define ENUMSET_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN







template<typename T, uint32_t minValue, uint32_t limitValue>
class EnumSet {
public:
    inline EnumSet() : fBools(0) {}
    inline EnumSet(const EnumSet<T,minValue,limitValue>& other) : fBools(other.fBools) {}
    inline ~EnumSet() {}
#ifndef U_HIDE_INTERNAL_API
    inline void clear() { fBools=0; }
    inline void add(T toAdd) { set(toAdd, 1); }
    inline void remove(T toRemove) { set(toRemove, 0); }
    inline int32_t contains(T toCheck) const { return get(toCheck); }
    inline void set(T toSet, int32_t v) { fBools=(fBools&(~flag(toSet)))|(v?(flag(toSet)):0); }
    inline int32_t get(T toCheck) const { return (fBools & flag(toCheck))?1:0; }
    inline UBool isValidEnum(T toCheck) const {  return (toCheck>=minValue&&toCheck<limitValue); }
    inline UBool isValidValue(int32_t v) const { return (v==0||v==1); }
    inline const EnumSet<T,minValue,limitValue>& operator=(const EnumSet<T,minValue,limitValue>& other) {
        fBools = other.fBools;
        return *this;
    }
  
    inline uint32_t getAll() const {
        return fBools; 
    }
#endif 

private:
    inline uint32_t flag(T toCheck) const { return (1<<(toCheck-minValue)); }
private:
    uint32_t fBools;
};

U_NAMESPACE_END

#endif 
#endif 
