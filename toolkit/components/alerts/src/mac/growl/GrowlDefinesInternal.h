







#ifndef _GROWL_GROWLDEFINESINTERNAL_H
#define _GROWL_GROWLDEFINESINTERNAL_H

#include <CoreFoundation/CoreFoundation.h>

#ifdef __OBJC__
#define XSTR(x) (@x)
#else 
#define XSTR CFSTR
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
		unsigned reserved: 12;
		signed   priority: 3;
		unsigned sticky:   1;
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

#endif 







#ifdef __OBJC__






#define SYNCHRONIZE_GROWL_PREFS() CFPreferencesAppSynchronize(CFSTR("com.Growl.GrowlHelperApp"))






#define UPDATE_GROWL_PREFS() do { \
	SYNCHRONIZE_GROWL_PREFS(); \
	NSNumber *pid = [[NSNumber alloc] initWithInt:[[NSProcessInfo processInfo] processIdentifier]];\
	NSDictionary *userInfo = [[NSDictionary alloc] initWithObjectsAndKeys:\
		pid,     @"pid",\
		nil];\
	[pid release];\
	[[NSDistributedNotificationCenter defaultCenter]\
		postNotificationName:@"GrowlPreferencesChanged" object:@"GrowlUserDefaults" userInfo:userInfo];\
	[userInfo release];\
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
	CFBooleanRef boolValue; \
	if (value) {\
		boolValue = kCFBooleanTrue; \
	} else {\
		boolValue = kCFBooleanFalse; \
	}\
	WRITE_GROWL_PREF_VALUE(key, boolValue, domain); } while(0)









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









#define READ_GROWL_PREF_FLOAT(key, domain, result) do {\
	CFNumberRef floatValue = NULL; \
	READ_GROWL_PREF_VALUE(key, domain, CFNumberRef, &floatValue); \
	if (floatValue) {\
		CFNumberGetValue(floatValue, kCFNumberFloatType, result); \
		CFRelease(floatValue); \
	} } while(0)









#define WRITE_GROWL_PREF_FLOAT(key, value, domain) do {\
	CFNumberRef floatValue = CFNumberCreate(NULL, kCFNumberFloatType, &value); \
	WRITE_GROWL_PREF_VALUE(key, floatValue, domain); \
	CFRelease(floatValue); } while(0)

#endif 
