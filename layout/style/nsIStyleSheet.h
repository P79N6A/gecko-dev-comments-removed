









































#ifndef nsIStyleSheet_h___
#define nsIStyleSheet_h___

#include <stdio.h>
#include "nsISupports.h"

class nsString;
class nsIURI;
class nsIDocument;



#define NS_ISTYLE_SHEET_IID     \
{ 0x3eb34a60, 0x04bd, 0x41d9,   \
 { 0x9f, 0x60, 0x88, 0x26, 0x94, 0xe6, 0x1c, 0x38 } }









class nsIStyleSheet : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_SHEET_IID)

  
  virtual nsIURI* GetSheetURI() const = 0;
  virtual nsIURI* GetBaseURI() const = 0;
  virtual void GetTitle(nsString& aTitle) const = 0;
  virtual void GetType(nsString& aType) const = 0;
  virtual bool HasRules() const = 0;

  






  virtual bool IsApplicable() const = 0;

  








  virtual void SetEnabled(bool aEnabled) = 0;

  


  virtual bool IsComplete() const = 0;
  virtual void SetComplete() = 0;

  
  virtual nsIStyleSheet* GetParentSheet() const = 0;  
  virtual nsIDocument* GetOwningDocument() const = 0; 
  virtual void SetOwningDocument(nsIDocument* aDocument) = 0;

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheet, NS_ISTYLE_SHEET_IID)

#endif 
