



































#ifndef _NSSASN_H_
#define _NSSASN_H_

#include "nscore.h"
#include "nsIX509Cert.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIASN1Sequence.h"
#include "nsIASN1PrintableItem.h"
#include "nsIMutableArray.h"






class nsNSSASN1Sequence : public nsIASN1Sequence
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIASN1SEQUENCE
  NS_DECL_NSIASN1OBJECT

  nsNSSASN1Sequence();
  virtual ~nsNSSASN1Sequence();
  
private:
  nsCOMPtr<nsIMutableArray> mASN1Objects;
  nsString mDisplayName;
  nsString mDisplayValue;
  PRUint32 mType;
  PRUint32 mTag;
  PRBool   mIsValidContainer;
  PRBool   mIsExpanded;
};

class nsNSSASN1PrintableItem : public nsIASN1PrintableItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIASN1PRINTABLEITEM
  NS_DECL_NSIASN1OBJECT

  nsNSSASN1PrintableItem();
  virtual ~nsNSSASN1PrintableItem();
  
private:
  nsString mDisplayName;
  nsString mValue;
  PRUint32 mType;
  PRUint32 mTag;
  unsigned char *mData;
  PRUint32       mLen;
};

nsresult CreateFromDER(unsigned char *data,
                       unsigned int   len,
                       nsIASN1Object **retval);
#endif 
