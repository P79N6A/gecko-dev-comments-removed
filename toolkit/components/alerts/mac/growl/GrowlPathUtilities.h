













typedef int GrowlSearchPathDirectory;
enum {
	
	GrowlSupportDirectory = 0x10000,
	
	GrowlScreenshotsDirectory,
	GrowlTicketsDirectory,
	GrowlPluginsDirectory,
};
typedef NSSearchPathDomainMask GrowlSearchPathDomainMask; 

@interface GrowlPathUtilities : NSObject {

}

#pragma mark Bundles











+ (NSBundle *) growlPrefPaneBundle;










+ (NSBundle *) helperAppBundle;









+ (NSBundle *) runningHelperAppBundle;

#pragma mark Directories













+ (NSArray *) searchPathForDirectory:(GrowlSearchPathDirectory) directory inDomains:(GrowlSearchPathDomainMask) domainMask mustBeWritable:(BOOL)flag;










+ (NSArray *) searchPathForDirectory:(GrowlSearchPathDirectory) directory inDomains:(GrowlSearchPathDomainMask) domainMask;






+ (NSString *) growlSupportDirectory;








+ (NSString *) screenshotsDirectory;








+ (NSString *) ticketsDirectory;

#pragma mark Screenshot names

















+ (NSString *) nextScreenshotName;














+ (NSString *) nextScreenshotNameInDirectory:(NSString *) dirName;

#pragma mark Tickets
















+ (NSString *) defaultSavePathForTicketWithApplicationName:(NSString *) appName;

@end
