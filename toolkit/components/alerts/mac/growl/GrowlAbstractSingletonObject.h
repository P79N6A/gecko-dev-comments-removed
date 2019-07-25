








#import <Foundation/Foundation.h>










@interface GrowlAbstractSingletonObject : NSObject {
	BOOL	_isInitialized;
}




+ (id) sharedInstance;












+ (void) destroyAllSingletons;

@end







@interface GrowlAbstractSingletonObject (GrowlAbstractSingletonObjectInit)








- (id) initSingleton;








- (void) destroy;

@end
