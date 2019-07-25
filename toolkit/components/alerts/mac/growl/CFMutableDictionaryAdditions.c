








#include "CFMutableDictionaryAdditions.h"

void setObjectForKey(CFMutableDictionaryRef dict, const void *key, CFTypeRef value) {
	CFDictionarySetValue(dict, key, value);
}

void setIntegerForKey(CFMutableDictionaryRef dict, const void *key, int value) {
	CFNumberRef num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &value);
	CFDictionarySetValue(dict, key, num);
	CFRelease(num);
}

void setBooleanForKey(CFMutableDictionaryRef dict, const void *key, Boolean value) {
	CFDictionarySetValue(dict, key, value ? kCFBooleanTrue : kCFBooleanFalse);
}
