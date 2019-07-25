




































#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsImageMapUtils.h"


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

  nsCOMPtr<nsIHTMLDocument> htmlDoc(do_QueryInterface(aDocument));
  if (htmlDoc) {
    nsIDOMHTMLMapElement* map = htmlDoc->GetImageMap(usemap);
    NS_IF_ADDREF(map);
    return map;
  } else {
    
    
    
    
    
    
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(aDocument));
    if (domDoc) {
      nsCOMPtr<nsIDOMElement> element;
      domDoc->GetElementById(usemap, getter_AddRefs(element));

      if (element) {
        nsIDOMHTMLMapElement* map;
        CallQueryInterface(element, &map);
        return map;
      }
    }
  }
  
  return nsnull;
}
