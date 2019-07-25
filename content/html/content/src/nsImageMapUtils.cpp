




































#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "mozilla/dom/Element.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsImageMapUtils.h"

using namespace mozilla::dom;


already_AddRefed<nsIDOMHTMLMapElement>
nsImageMapUtils::FindImageMap(nsIDocument *aDocument, 
                              const nsAString &aUsemap)
{
  if (!aDocument)
    return nsnull;

  
  
  

  if (aUsemap.IsEmpty())
    return nsnull;

  nsAString::const_iterator start, end;
  aUsemap.BeginReading(start);
  aUsemap.EndReading(end);

  PRInt32 hash = aUsemap.FindChar('#');
  if (hash > -1) {
    
    start.advance(hash + 1);

    if (start == end) {
      return nsnull; 
    }
  } else {
    return nsnull;
  }

  const nsAString& usemap = Substring(start, end);

  nsCOMPtr<nsIDOMHTMLMapElement> map =
    do_QueryInterface(aDocument->FindImageMap(usemap));
  return map.forget();
}
