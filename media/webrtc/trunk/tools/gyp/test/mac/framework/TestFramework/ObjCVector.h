



#import <Cocoa/Cocoa.h>

#ifdef __cplusplus
struct ObjCVectorImp;
#else
typedef struct _ObjCVectorImpT ObjCVectorImp;
#endif

@interface ObjCVector : NSObject {
 @private
  ObjCVectorImp* imp_;
}

- (id)init;

- (void)addObject:(id)obj;
- (void)addObject:(id)obj atIndex:(NSUInteger)index;

- (void)removeObject:(id)obj;
- (void)removeObjectAtIndex:(NSUInteger)index;

- (id)objectAtIndex:(NSUInteger)index;

@end
