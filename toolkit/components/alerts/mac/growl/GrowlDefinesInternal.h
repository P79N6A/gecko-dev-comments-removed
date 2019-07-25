







#ifndef _GROWL_GROWLDEFINESINTERNAL_H
#define _GROWL_GROWLDEFINESINTERNAL_H

#include <CoreFoundation/CoreFoundation.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __OBJC__
#define XSTR(x) (@x)
#else 
#define XSTR CFSTR
#endif 















#ifndef NSINTEGER_DEFINED
typedef int NSInteger;
typedef unsigned int NSUInteger;
#define NSINTEGER_DEFINED
#endif





#ifndef CGFLOAT_DEFINED
typedef float CGFloat;
#define CGFLOAT_IS_DOUBLE 0
#define CGFLOAT_DEFINED
#endif













#if CGFLOAT_IS_DOUBLE
#define GrowlCGFloatCeiling(x) ceil(x)
#define GrowlCGFloatAbsoluteValue(x) fabs(x)
#define GrowlCGFloatFloor(x) floor(x)
#else
#define GrowlCGFloatCeiling(x) ceilf(x)
#define GrowlCGFloatAbsoluteValue(x) fabsf(x)
#define GrowlCGFloatFloor(x) floorf(x)
#endif




#define GROWL_TCP_PORT	23052




#define GROWL_UDP_PORT	9887




#define GROWL_PROTOCOL_VERSION	1




#define GROWL_PROTOCOL_VERSION_AES128	2




#define GROWL_TYPE_REGISTRATION			0



#define GROWL_TYPE_NOTIFICATION			1



#define GROWL_TYPE_REGISTRATION_SHA256	2



#define GROWL_TYPE_NOTIFICATION_SHA256	3



#define GROWL_TYPE_REGISTRATION_NOAUTH	4



#define GROWL_TYPE_NOTIFICATION_NOAUTH	5

#define ATTRIBUTE_PACKED __attribute((packed))





struct GrowlNetworkPacket {
	unsigned char version;
	unsigned char type;
} ATTRIBUTE_PACKED;












struct GrowlNetworkRegistration {
	struct GrowlNetworkPacket common;
	










	unsigned short appNameLen;
	


	unsigned char numAllNotifications;

	unsigned char numDefaultNotifications;
	










	unsigned char data[];
} ATTRIBUTE_PACKED;














struct GrowlNetworkNotification {
	struct GrowlNetworkPacket common;
	









	struct GrowlNetworkNotificationFlags {
#ifdef __BIG_ENDIAN__
		unsigned reserved: 12;
		signed   priority: 3;
		unsigned sticky:   1;
#else
		unsigned sticky:   1;
		signed   priority: 3;
		unsigned reserved: 12;
#endif
	} ATTRIBUTE_PACKED flags; 

	


	unsigned short nameLen;
	


	unsigned short titleLen;
	


	unsigned short descriptionLen;
	


	unsigned short appNameLen;
	






	unsigned char data[];
} ATTRIBUTE_PACKED;







#define GrowlEnabledKey					XSTR("GrowlEnabled")











#define GROWL_SCREENSHOT_MODE			XSTR("ScreenshotMode")






#define GROWL_APP_LOCATION				XSTR("AppLocation")






#define GROWL_REMOTE_ADDRESS			XSTR("RemoteAddress")





#define GROWL_PREFPANE_BUNDLE_IDENTIFIER		XSTR("com.growl.prefpanel")




#define GROWL_HELPERAPP_BUNDLE_IDENTIFIER	XSTR("com.Growl.GrowlHelperApp")





#define GROWL_PREFPANE_NAME						XSTR("Growl.prefPane")
#define PREFERENCE_PANES_SUBFOLDER_OF_LIBRARY	XSTR("PreferencePanes")
#define PREFERENCE_PANE_EXTENSION				XSTR("prefPane")


#define GROWL_PLUGIN_EXTENSION                  XSTR("growlPlugin")
#define GROWL_PATHWAY_EXTENSION                 XSTR("growlPathway")
#define GROWL_VIEW_EXTENSION					XSTR("growlView")
#define GROWL_STYLE_EXTENSION					XSTR("growlStyle")








#define SYNCHRONIZE_GROWL_PREFS() CFPreferencesAppSynchronize(CFSTR("com.Growl.GrowlHelperApp"))






#define UPDATE_GROWL_PREFS() do { \
	SYNCHRONIZE_GROWL_PREFS(); \
	CFStringRef _key = CFSTR("pid"); \
	int pid = getpid(); \
	CFNumberRef _value = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &pid); \
	CFDictionaryRef userInfo = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&_key, (const void **)&_value, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks); \
	CFRelease(_value); \
	CFNotificationCenterPostNotification(CFNotificationCenterGetDistributedCenter(), \
										 CFSTR("GrowlPreferencesChanged"), \
										 CFSTR("GrowlUserDefaults"), \
										 userInfo, false); \
	CFRelease(userInfo); \
	} while(0)













#define READ_GROWL_PREF_VALUE(key, domain, type, result) do {\
	CFDictionaryRef prefs = (CFDictionaryRef)CFPreferencesCopyAppValue((CFStringRef)domain, \
																		CFSTR("com.Growl.GrowlHelperApp")); \
	if (prefs) {\
		if (CFDictionaryContainsKey(prefs, key)) {\
			*result = (type)CFDictionaryGetValue(prefs, key); \
			CFRetain(*result); \
		} \
		CFRelease(prefs); } \
	} while(0)










#define WRITE_GROWL_PREF_VALUE(key, value, domain) do {\
	CFDictionaryRef staticPrefs = (CFDictionaryRef)CFPreferencesCopyAppValue((CFStringRef)domain, \
																			 CFSTR("com.Growl.GrowlHelperApp")); \
	CFMutableDictionaryRef prefs; \
	if (staticPrefs == NULL) {\
		prefs = CFDictionaryCreateMutable(NULL, 0, NULL, NULL); \
	} else {\
		prefs = CFDictionaryCreateMutableCopy(NULL, 0, staticPrefs); \
		CFRelease(staticPrefs); \
	}\
	CFDictionarySetValue(prefs, key, value); \
	CFPreferencesSetAppValue((CFStringRef)domain, prefs, CFSTR("com.Growl.GrowlHelperApp")); \
	CFRelease(prefs); } while(0)









#define READ_GROWL_PREF_BOOL(key, domain, result) do {\
	CFBooleanRef boolValue = NULL; \
	READ_GROWL_PREF_VALUE(key, domain, CFBooleanRef, &boolValue); \
	if (boolValue) {\
		*result = CFBooleanGetValue(boolValue); \
		CFRelease(boolValue); \
	} } while(0)









#define WRITE_GROWL_PREF_BOOL(key, value, domain) do {\
	WRITE_GROWL_PREF_VALUE(key, value ? kCFBooleanTrue : kCFBooleanFalse, domain); } while(0)









#define READ_GROWL_PREF_INT(key, domain, result) do {\
	CFNumberRef intValue = NULL; \
	READ_GROWL_PREF_VALUE(key, domain, CFNumberRef, &intValue); \
	if (intValue) {\
		CFNumberGetValue(intValue, kCFNumberIntType, result); \
		CFRelease(intValue); \
	} } while(0)









#define WRITE_GROWL_PREF_INT(key, value, domain) do {\
	CFNumberRef intValue = CFNumberCreate(NULL, kCFNumberIntType, &value); \
	WRITE_GROWL_PREF_VALUE(key, intValue, domain); \
	CFRelease(intValue); } while(0)









#ifdef __LP64__
#define READ_GROWL_PREF_FLOAT(key, domain, result) do {\
	CFNumberRef floatValue = NULL; \
	READ_GROWL_PREF_VALUE(key, domain, CFNumberRef, &floatValue); \
	if (floatValue) {\
		CFNumberGetValue(floatValue, kCFNumberCGFloatType, result); \
		CFRelease(floatValue); \
	} } while(0)
#else
#define READ_GROWL_PREF_FLOAT(key, domain, result) do {\
	CFNumberRef floatValue = NULL; \
	READ_GROWL_PREF_VALUE(key, domain, CFNumberRef, &floatValue); \
	if (floatValue) {\
		CFNumberGetValue(floatValue, kCFNumberFloatType, result); \
		CFRelease(floatValue); \
	} } while(0)
#endif









#ifdef __LP64__
#define WRITE_GROWL_PREF_FLOAT(key, value, domain) do {\
	CFNumberRef floatValue = CFNumberCreate(NULL, kCFNumberCGFloatType, &value); \
	WRITE_GROWL_PREF_VALUE(key, floatValue, domain); \
	CFRelease(floatValue); } while(0)
#else
#define WRITE_GROWL_PREF_FLOAT(key, value, domain) do {\
	CFNumberRef floatValue = CFNumberCreate(NULL, kCFNumberFloatType, &value); \
	WRITE_GROWL_PREF_VALUE(key, floatValue, domain); \
	CFRelease(floatValue); } while(0)
#endif







#define GROWL_CLOSE_ALL_NOTIFICATIONS XSTR("GrowlCloseAllNotifications")

#pragma mark Small utilities





#define FLOAT_EQ(x,y) (((y - FLT_EPSILON) < x) && (x < (y + FLT_EPSILON)))

#endif 
