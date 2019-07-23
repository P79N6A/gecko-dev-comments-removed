








#ifndef HAVE_CFMUTABLEDICTIONARYADDITIONS_H
#define HAVE_CFMUTABLEDICTIONARYADDITIONS_H

#include "CFGrowlDefines.h"

void setObjectForKey(MUTABLE_DICTIONARY_TYPE dict, const void *key, OBJECT_TYPE value);
void setIntegerForKey(MUTABLE_DICTIONARY_TYPE dict, const void *key, int value);
void setBooleanForKey(MUTABLE_DICTIONARY_TYPE dict, const void *key, BOOL_TYPE value);

#endif
