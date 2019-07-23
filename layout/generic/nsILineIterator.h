



































#ifndef nsILineIterator_h___
#define nsILineIterator_h___

#include "nsISupports.h"


#define NS_ILINE_ITERATOR_IID \
 { 0xa6cf90ff, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}


#define NS_ILINE_ITERATOR_NAV_IID \
 { 0x80aa3d7a, 0xe0bf, 0x4e18,{0x8a, 0x82, 0x21, 0x10, 0x39, 0x7d, 0x7b, 0xc4}}
















#define NS_LINE_FLAG_IS_BLOCK           0x1


#define NS_LINE_FLAG_ENDS_IN_BREAK      0x4

class nsILineIterator : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINE_ITERATOR_IID)

  
  NS_IMETHOD GetNumLines(PRInt32* aResult) = 0;

  
  
  
  NS_IMETHOD GetDirection(PRBool* aIsRightToLeft) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_IMETHOD GetLine(PRInt32 aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     PRInt32* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     PRUint32* aLineFlags) = 0;

  
  
  
  NS_IMETHOD FindLineContaining(nsIFrame* aFrame,
                                PRInt32* aLineNumberResult) = 0;

  
  
  
  
  
  NS_IMETHOD FindLineAt(nscoord aY,
                        PRInt32* aLineNumberResult) = 0;

  
  
  
  
  NS_IMETHOD FindFrameAt(PRInt32 aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         PRBool* aXIsBeforeFirstFrame,
                         PRBool* aXIsAfterLastFrame) = 0;

  
  
  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, PRInt32 aLineNumber) = 0;

#ifdef IBMBIDI
  
  
  NS_IMETHOD CheckLineOrder(PRInt32                  aLine,
                            PRBool                   *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual) = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILineIterator, NS_ILINE_ITERATOR_IID)


class nsILineIteratorNavigator : public nsILineIterator {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILINE_ITERATOR_NAV_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILineIteratorNavigator,
                              NS_ILINE_ITERATOR_NAV_IID)

#endif
