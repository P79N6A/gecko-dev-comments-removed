




































#include "nsComposeTxtSrvFilter.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsString.h"
#include "nsINameSpaceManager.h"

nsComposeTxtSrvFilter::nsComposeTxtSrvFilter() :
  mIsForMail(PR_FALSE)
{

  mBlockQuoteAtom  = do_GetAtom("blockquote");
  mPreAtom         = do_GetAtom("pre");
  mSpanAtom        = do_GetAtom("span");
  mTableAtom       = do_GetAtom("table");
  mMozQuoteAtom    = do_GetAtom("_moz_quote");
  mClassAtom       = do_GetAtom("class");
  mTypeAtom        = do_GetAtom("type");
  mScriptAtom      = do_GetAtom("script");
  mTextAreaAtom    = do_GetAtom("textarea");
  mSelectAreaAtom  = do_GetAtom("select");
  mMapAtom         = do_GetAtom("map");
  mCiteAtom        = do_GetAtom("cite");
  mTrueAtom        = do_GetAtom("true");
  mMozSignatureAtom= do_GetAtom("moz-signature");
}

NS_IMPL_ISUPPORTS1(nsComposeTxtSrvFilter, nsITextServicesFilter)

NS_IMETHODIMP 
nsComposeTxtSrvFilter::Skip(nsIDOMNode* aNode, PRBool *_retval)
{
  *_retval = PR_FALSE;

  
  
  
  nsCOMPtr<nsIContent> content(do_QueryInterface(aNode));
  if (content) {
    nsIAtom *tag = content->Tag();
    if (tag == mBlockQuoteAtom) {
      if (mIsForMail) {
        *_retval = content->AttrValueIs(kNameSpaceID_None, mTypeAtom,
                                        mCiteAtom, eIgnoreCase);
      }
    } else if (tag == mPreAtom || tag == mSpanAtom) {
      if (mIsForMail) {
        *_retval = content->AttrValueIs(kNameSpaceID_None, mMozQuoteAtom,
                                        mTrueAtom, eIgnoreCase);
        if (!*_retval) {
          *_retval = content->AttrValueIs(kNameSpaceID_None, mClassAtom,
                                          mMozSignatureAtom, eCaseMatters);
        }
      }         
    } else if (tag == mScriptAtom ||
               tag == mTextAreaAtom ||
               tag == mSelectAreaAtom ||
               tag == mMapAtom) {
      *_retval = PR_TRUE;
    } else if (tag == mTableAtom) {
      if (mIsForMail) {
        *_retval =
          content->AttrValueIs(kNameSpaceID_None, mClassAtom,
                               NS_LITERAL_STRING("moz-email-headers-table"),
                               eCaseMatters);
      } 
    }
  }

  return NS_OK;
}
