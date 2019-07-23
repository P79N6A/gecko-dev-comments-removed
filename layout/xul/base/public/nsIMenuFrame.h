




































#ifndef nsIMenuFrame_h___
#define nsIMenuFrame_h___

#include "nsISupports.h"





#define NS_IMENUFRAME_IID \
{ 0x212521C8, 0x1509, 0x4F41, { 0xAD, 0xDB, 0x6A, 0x0B, 0x93, 0x56, 0x77, 0x0F } }

class nsIMenuFrame : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMENUFRAME_IID)

  virtual PRBool IsOpen() = 0;
  virtual PRBool IsMenu() = 0;
  virtual PRBool IsOnMenuBar() = 0;
  virtual PRBool IsOnActiveMenuBar() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIMenuFrame, NS_IMENUFRAME_IID)

#endif

