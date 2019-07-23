





































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
  



  virtual PRBool Equal(nsIDOMElement *aElm) = 0;
};





class nsLangTextAttr : public nsTextAttr
{
public:
  nsLangTextAttr(nsAString& aLang, nsIContent *aRootContent) :
    mLang(aLang), mRootContent(aRootContent) { }

  virtual PRBool Equal(nsIDOMElement *aElm);

private:
  nsString mLang;
  nsCOMPtr<nsIContent> mRootContent;
};





class nsCSSTextAttr : public nsTextAttr
{
public:
  nsCSSTextAttr(PRBool aIncludeDefAttrValue, nsIDOMElement *aElm,
                nsIDOMElement *aRootElm);

  
  virtual PRBool Equal(nsIDOMElement *aElm);

  
  


  PRBool Iterate();

  


  PRBool Get(nsACString& aName, nsAString& aValue);

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
  
  
  virtual PRBool Equal(nsIDOMElement *aElm);

  

  






  virtual PRBool Get(nsAString& aValue);

private:
  










  nscolor GetColor(nsIFrame *aFrame);

  nsIFrame *mFrame;
  nsIFrame *mRootFrame;
};





class nsFontSizeTextAttr : public nsTextAttr
{
public:
  nsFontSizeTextAttr(nsIFrame *aFrame, nsIFrame *aRootFrame);
  
  
  virtual PRBool Equal(nsIDOMElement *aElm);

  

  






  virtual PRBool Get(nsAString& aValue);

private:
  





   nscoord GetFontSize(nsIFrame *aFrame);

  nsIFrame *mFrame;
  nsIFrame *mRootFrame;
};

#endif
