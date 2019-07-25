









































#ifndef nsIStyleSheet_h___
#define nsIStyleSheet_h___

#include <stdio.h>
#include "nsISupports.h"

class nsString;
class nsIURI;
class nsIDocument;
template<class T> struct already_AddRefed;



#define NS_ISTYLE_SHEET_IID     \
{ 0x5de8de51, 0x1f82, 0x4e3d,   \
 { 0x95, 0x44, 0x9a, 0x5b, 0xb0, 0x7b, 0x44, 0x00 } }









class nsIStyleSheet : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_SHEET_IID)

  
  virtual already_AddRefed<nsIURI> GetSheetURI() const = 0;
  virtual already_AddRefed<nsIURI> GetBaseURI() const = 0;
  virtual void GetTitle(nsString& aTitle) const = 0;
  virtual void GetType(nsString& aType) const = 0;
  virtual PRBool HasRules() const = 0;

  






  virtual PRBool IsApplicable() const = 0;

  








  virtual void SetEnabled(PRBool aEnabled) = 0;

  


  virtual PRBool IsComplete() const = 0;
  virtual void SetComplete() = 0;

  
  virtual already_AddRefed<nsIStyleSheet> GetParentSheet() const = 0;  
  virtual already_AddRefed<nsIDocument> GetOwningDocument() const = 0; 
  virtual void SetOwningDocument(nsIDocument* aDocument) = 0;

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheet, NS_ISTYLE_SHEET_IID)

#endif 
