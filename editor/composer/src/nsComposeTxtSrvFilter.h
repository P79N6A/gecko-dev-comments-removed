




































#ifndef nsComposeTxtSrvFilter_h__
#define nsComposeTxtSrvFilter_h__

#include "nsITextServicesFilter.h"
#include "nsIAtom.h"








class nsComposeTxtSrvFilter : public nsITextServicesFilter
{
public:
  nsComposeTxtSrvFilter();
  virtual ~nsComposeTxtSrvFilter() {}

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSITEXTSERVICESFILTER

  
  void Init(PRBool aIsForMail) { mIsForMail = aIsForMail; }

protected:
  PRBool            mIsForMail;
  nsCOMPtr<nsIAtom> mBlockQuoteAtom;
  nsCOMPtr<nsIAtom> mPreAtom;          
  nsCOMPtr<nsIAtom> mSpanAtom;         
  nsCOMPtr<nsIAtom> mMozQuoteAtom;     
  nsCOMPtr<nsIAtom> mTableAtom;
  nsCOMPtr<nsIAtom> mClassAtom;
  nsCOMPtr<nsIAtom> mTypeAtom;
  nsCOMPtr<nsIAtom> mScriptAtom;
  nsCOMPtr<nsIAtom> mTextAreaAtom;
  nsCOMPtr<nsIAtom> mSelectAreaAtom;
  nsCOMPtr<nsIAtom> mMapAtom;
  nsCOMPtr<nsIAtom> mCiteAtom;
  nsCOMPtr<nsIAtom> mTrueAtom;
  nsCOMPtr<nsIAtom> mMozSignatureAtom;
};

#define NS_COMPOSERTXTSRVFILTER_CID \
{/* {171E72DB-0F8A-412a-8461-E4C927A3A2AC}*/ \
0x171e72db, 0xf8a, 0x412a, \
{ 0x84, 0x61, 0xe4, 0xc9, 0x27, 0xa3, 0xa2, 0xac} } 

#define NS_COMPOSERTXTSRVFILTERMAIL_CID \
{/* {7FBD2146-5FF4-4674-B069-A7BBCE66E773}*/ \
0x7fbd2146, 0x5ff4, 0x4674, \
{ 0xb0, 0x69, 0xa7, 0xbb, 0xce, 0x66, 0xe7, 0x73} } 


#define COMPOSER_TXTSRVFILTER_CONTRACTID     "@mozilla.org/editor/txtsrvfilter;1"


#define COMPOSER_TXTSRVFILTERMAIL_CONTRACTID "@mozilla.org/editor/txtsrvfiltermail;1"

#endif
