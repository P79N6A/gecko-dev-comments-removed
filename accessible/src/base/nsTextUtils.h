





































#ifndef nsTextUtils_h_
#define nsTextUtils_h_

#include "nsIDOMElement.h"
#include "nsIDOMCSSStyleDeclaration.h"

#include "nsIContent.h"
#include "nsIFrame.h"

#include "nsCOMPtr.h"
#include "nsString.h"




class nsTextAttr
{
public:
  



  virtual PRBool equal(nsIDOMElement *aElm) = 0;
};





class nsLangTextAttr : public nsTextAttr
{
public:
  nsLangTextAttr(nsAString& aLang, nsIContent *aRootContent) :
    mLang(aLang), mRootContent(aRootContent) { }

  virtual PRBool equal(nsIDOMElement *aElm);

private:
  nsString mLang;
  nsCOMPtr<nsIContent> mRootContent;
};





class nsCSSTextAttr : public nsTextAttr
{
public:
  nsCSSTextAttr(PRBool aIncludeDefAttrValue, nsIDOMElement *aElm,
                nsIDOMElement *aRootElm);

  
  virtual PRBool equal(nsIDOMElement *aElm);

  
  


  PRBool iterate();

  


  PRBool get(nsACString& aName, nsAString& aValue);

private:
  PRInt32 mIndex;
  PRBool mIncludeDefAttrValue;

  nsCOMPtr<nsIDOMCSSStyleDeclaration> mStyleDecl;
  nsCOMPtr<nsIDOMCSSStyleDeclaration> mDefStyleDecl;
};





class nsBackgroundTextAttr : public nsTextAttr
{
public:
  nsBackgroundTextAttr(nsIFrame *aFrame, nsIFrame *aRootFrame);
  
  
  virtual PRBool equal(nsIDOMElement *aElm);

  



  virtual PRBool get(nsAString& aValue);

private:
  










  nscolor getColor(nsIFrame *aFrame);

  nsIFrame *mFrame;
  nsIFrame *mRootFrame;
};

#endif
