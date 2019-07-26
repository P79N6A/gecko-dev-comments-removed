





#ifndef nsTArrayForwardDeclare_h__
#define nsTArrayForwardDeclare_h__













#include <stddef.h>

template<class E>
class nsTArray;

template<class E>
class FallibleTArray;

template<class E, size_t N>
class nsAutoTArray;

template<class E, size_t N>
class AutoFallibleTArray;

#define InfallibleTArray nsTArray
#define AutoInfallibleTArray nsAutoTArray

#endif
