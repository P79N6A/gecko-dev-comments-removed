










































#ifndef nsIMediaList_h_
#define nsIMediaList_h_

#include "nsIDOMMediaList.h"
#include "nsAString.h"
#include "nsCOMArray.h"
#include "nsIAtom.h"
class nsPresContext;
class nsICSSStyleSheet;
class nsCSSStyleSheet;

class nsMediaList : public nsIDOMMediaList {
public:
  nsMediaList();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIDOMMEDIALIST

  nsresult GetText(nsAString& aMediaText);
  nsresult SetText(const nsAString& aMediaText);
  PRBool Matches(nsPresContext* aPresContext);
  nsresult SetStyleSheet(nsICSSStyleSheet* aSheet);
  nsresult AppendAtom(nsIAtom* aMediumAtom) {
    return mArray.AppendObject(aMediumAtom) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult Clone(nsMediaList** aResult);

  PRInt32 Count() { return mArray.Count(); }
  nsIAtom* MediumAt(PRInt32 aIndex) { return mArray[aIndex]; }
  void Clear() { mArray.Clear(); }

protected:
  ~nsMediaList();

  nsresult Delete(const nsAString & aOldMedium);
  nsresult Append(const nsAString & aOldMedium);

  nsCOMArray<nsIAtom> mArray;
  
  
  
  nsCSSStyleSheet*         mStyleSheet;
};
#endif 
