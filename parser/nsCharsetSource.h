



#ifndef nsCharsetSource_h_
#define nsCharsetSource_h_


#define kCharsetUninitialized           0
#define kCharsetFromWeakDocTypeDefault  1
#define kCharsetFromUserDefault         2
#define kCharsetFromDocTypeDefault      3 // This and up confident for XHR
#define kCharsetFromCache               4
#define kCharsetFromParentFrame         5
#define kCharsetFromAutoDetection       6
#define kCharsetFromHintPrevDoc         7
#define kCharsetFromMetaPrescan         8 // this one and smaller: HTML5 Tentative
#define kCharsetFromMetaTag             9 // this one and greater: HTML5 Confident
#define kCharsetFromIrreversibleAutoDetection 10
#define kCharsetFromChannel            11
#define kCharsetFromOtherComponent     12
#define kCharsetFromParentForced       13 // propagates to child frames
#define kCharsetFromUserForced         14 // propagates to child frames
#define kCharsetFromByteOrderMark      15

#endif 
