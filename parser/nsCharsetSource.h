



#ifndef nsCharsetSource_h_
#define nsCharsetSource_h_


#define kCharsetUninitialized           0
#define kCharsetFromWeakDocTypeDefault  1
#define kCharsetFromDocTypeDefault      2 // This and up confident for XHR
#define kCharsetFromCache               3
#define kCharsetFromParentFrame         4
#define kCharsetFromAutoDetection       5
#define kCharsetFromHintPrevDoc         6
#define kCharsetFromMetaPrescan         7 // this one and smaller: HTML5 Tentative
#define kCharsetFromMetaTag             8 // this one and greater: HTML5 Confident
#define kCharsetFromIrreversibleAutoDetection 9
#define kCharsetFromChannel            10
#define kCharsetFromOtherComponent     11
#define kCharsetFromParentForced       12 // propagates to child frames
#define kCharsetFromUserForced         13 // propagates to child frames
#define kCharsetFromByteOrderMark      14

#endif 
