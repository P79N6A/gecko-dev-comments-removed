




#include "nsComposeTxtSrvFilter.h"
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsIDOMNode.h"                 
#include "nsNameSpaceManager.h"        
#include "nsLiteralString.h"            
#include "nscore.h"                     

nsComposeTxtSrvFilter::nsComposeTxtSrvFilter() :
  mIsForMail(false)
{
}

NS_IMPL_ISUPPORTS(nsComposeTxtSrvFilter, nsITextServicesFilter)

NS_IMETHODIMP
nsComposeTxtSrvFilter::Skip(nsIDOMNode* aNode, bool *_retval)
{
  *_retval = false;

  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (content) {
    if (content->IsHTMLElement(nsGkAtoms::blockquote)) {
      if (mIsForMail) {
        *_retval = content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                        nsGkAtoms::cite, eIgnoreCase);
      }
    } else if (content->IsHTMLElement(nsGkAtoms::span)) {
      if (mIsForMail) {
        *_retval = content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozquote,
                                        nsGkAtoms::_true, eIgnoreCase);
        if (!*_retval) {
          *_retval = content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::_class,
                                          nsGkAtoms::mozsignature, eCaseMatters);
        }
      }
    } else if (content->IsAnyOfHTMLElements(nsGkAtoms::script,
                                            nsGkAtoms::textarea,
                                            nsGkAtoms::select,
                                            nsGkAtoms::map)) {
      *_retval = true;
    } else if (content->IsHTMLElement(nsGkAtoms::table)) {
      if (mIsForMail) {
        *_retval =
          content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::_class,
                               NS_LITERAL_STRING("moz-email-headers-table"),
                               eCaseMatters);
      }
    }
  }

  return NS_OK;
}
