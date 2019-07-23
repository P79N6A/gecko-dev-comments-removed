



































 
#ifndef __nsDialogParamBlock_h
#define __nsDialogParamBlock_h

#include "nsIDialogParamBlock.h"
#include "nsIMutableArray.h"
#include "nsCOMPtr.h"


#define NS_DIALOGPARAMBLOCK_CID \
 {0x4e4aae11, 0x8901, 0x46cc, {0x82, 0x17, 0xda, 0xd7, 0xc5, 0x41, 0x58, 0x73}}

class nsString;

class nsDialogParamBlock: public nsIDialogParamBlock
{
public: 	
  nsDialogParamBlock();
  virtual ~nsDialogParamBlock();
   
  NS_DECL_NSIDIALOGPARAMBLOCK
  NS_DECL_ISUPPORTS	

private:

  enum {kNumInts = 8, kNumStrings = 16};

  nsresult InBounds(PRInt32 inIndex, PRInt32 inMax) {
    return inIndex >= 0 && inIndex < inMax ? NS_OK : NS_ERROR_ILLEGAL_VALUE;
  }
  
  PRInt32 mInt[kNumInts];
  PRInt32 mNumStrings;
  nsString* mString;
  nsCOMPtr<nsIMutableArray> mObjects;	
};

#endif

