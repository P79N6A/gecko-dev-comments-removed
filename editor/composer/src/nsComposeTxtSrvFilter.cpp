




#include "nsComposeTxtSrvFilter.h"
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsIDOMNode.h"                 
#include "nsINameSpaceManager.h"        
#include "nsLiteralString.h"            
#include "nscore.h"                     

nsComposeTxtSrvFilter::nsComposeTxtSrvFilter() :
  mIsForMail(false)
{
}

NS_IMPL_ISUPPORTS1(nsComposeTxtSrvFilter, nsITextServicesFilter)

NS_IMETHODIMP
nsComposeTxtSrvFilter::Skip(nsIDOMNode* aNode, bool *_retval)
{
  *_retval = false;

  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (content) {
    nsIAtom *tag = content->Tag();
    if (tag == nsGkAtoms::blockquote) {
      if (mIsForMail) {
        *_retval = content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                                        nsGkAtoms::cite, eIgnoreCase);
      }
    } else if (tag == nsGkAtoms::span) {
      if (mIsForMail) {
        *_retval = content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::mozquote,
                                        nsGkAtoms::_true, eIgnoreCase);
        if (!*_retval) {
          *_retval = content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::_class,
                                          nsGkAtoms::mozsignature, eCaseMatters);
        }
      }
    } else if (tag == nsGkAtoms::script ||
               tag == nsGkAtoms::textarea ||
               tag == nsGkAtoms::select ||
               tag == nsGkAtoms::map) {
      *_retval = true;
    } else if (tag == nsGkAtoms::table) {
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
