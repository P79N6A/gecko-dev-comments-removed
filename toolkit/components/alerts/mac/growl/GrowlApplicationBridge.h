














#ifndef __GrowlApplicationBridge_h__
#define __GrowlApplicationBridge_h__

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import "GrowlDefines.h"


@protocol GrowlApplicationBridgeDelegate;


#define GROWL_USER_CHOSE_NOT_TO_INSTALL_NOTIFICATION @"User chose not to install"


#pragma mark -









@interface GrowlApplicationBridge : NSObject {

}







+ (BOOL) isGrowlInstalled;







+ (BOOL) isGrowlRunning;

#pragma mark -





























+ (void) setGrowlDelegate:(NSObject<GrowlApplicationBridgeDelegate> *)inDelegate;







+ (NSObject<GrowlApplicationBridgeDelegate> *) growlDelegate;

#pragma mark -

























+ (void) notifyWithTitle:(NSString *)title
			 description:(NSString *)description
		notificationName:(NSString *)notifName
				iconData:(NSData *)iconData
				priority:(signed int)priority
				isSticky:(BOOL)isSticky
			clickContext:(id)clickContext;


























+ (void) notifyWithTitle:(NSString *)title
			 description:(NSString *)description
		notificationName:(NSString *)notifName
				iconData:(NSData *)iconData
				priority:(signed int)priority
				isSticky:(BOOL)isSticky
			clickContext:(id)clickContext
			  identifier:(NSString *)identifier;















+ (void) notifyWithDictionary:(NSDictionary *)userInfo;

#pragma mark -




















+ (BOOL) registerWithDictionary:(NSDictionary *)regDict;











+ (void) reregisterGrowlNotifications;

#pragma mark -

















+ (void) setWillRegisterWhenGrowlIsReady:(BOOL)flag;






+ (BOOL) willRegisterWhenGrowlIsReady;

#pragma mark -


















+ (NSDictionary *) registrationDictionaryFromDelegate;






















+ (NSDictionary *) registrationDictionaryFromBundle:(NSBundle *)bundle;

























+ (NSDictionary *) bestRegistrationDictionary;

#pragma mark -





















+ (NSDictionary *) registrationDictionaryByFillingInDictionary:(NSDictionary *)regDict;






















+ (NSDictionary *) registrationDictionaryByFillingInDictionary:(NSDictionary *)regDict restrictToKeys:(NSSet *)keys;













+ (NSDictionary *) notificationDictionaryByFillingInDictionary:(NSDictionary *)regDict;

+ (NSDictionary *) frameworkInfoDictionary;
@end


#pragma mark -










@protocol GrowlApplicationBridgeDelegate



@end


#pragma mark -






@interface NSObject (GrowlApplicationBridgeDelegate_InformalProtocol)





























- (NSDictionary *) registrationDictionaryForGrowl;


















- (NSString *) applicationNameForGrowl;











- (NSImage *) applicationIconForGrowl;










- (NSData *) applicationIconDataForGrowl;








- (void) growlIsReady;











- (void) growlNotificationWasClicked:(id)clickContext;











- (void) growlNotificationTimedOut:(id)clickContext;

@end

#pragma mark -








@interface NSObject (GrowlApplicationBridgeDelegate_Installation_InformalProtocol)







- (NSString *)growlInstallationWindowTitle;







- (NSString *)growlUpdateWindowTitle;












- (NSAttributedString *)growlInstallationInformation;












- (NSAttributedString *)growlUpdateInformation;

@end


@interface GrowlApplicationBridge (GrowlInstallationPrompt_private)
+ (void) _userChoseNotToInstallGrowl;
@end

#endif 
