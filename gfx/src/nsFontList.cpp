










































#include "nsCOMPtr.h"
#include "nsXPCOM.h"
#include "nsFontList.h"
#include "nsGfxCIID.h"
#include "nsDependentString.h"
#include "nsIFontEnumerator.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsPrimitives.h"
#include "nsMemory.h"
#include "nsReadableUtils.h"
#include "nsXPIDLString.h"

static NS_DEFINE_IID(kCFontEnumerator, NS_FONT_ENUMERATOR_CID);

nsFontList::nsFontList()
{
}

nsFontList::~nsFontList()
{
}

NS_IMPL_ISUPPORTS1(nsFontList,nsIFontList)


class
nsFontListEnumerator : public nsISimpleEnumerator
{
  public:
    nsFontListEnumerator();
    virtual ~nsFontListEnumerator();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    NS_IMETHOD Init(const PRUnichar *aLangGroup, const PRUnichar *aFontType);


  protected:
    PRUnichar **mFonts;
    PRUint32 mCount;
    PRUint32 mIndex;
};

nsFontListEnumerator::nsFontListEnumerator() :
  mFonts(nsnull), mCount(0), mIndex(0)
{
}

nsFontListEnumerator::~nsFontListEnumerator()
{
  if (mFonts) {
    PRUint32 i;
    for (i=0; i<mCount; i++) {
      nsMemory::Free(mFonts[i]);
    }
    nsMemory::Free(mFonts);
  }
}

NS_IMPL_ISUPPORTS1(nsFontListEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
nsFontListEnumerator::Init(const PRUnichar *aLangGroup, 
                           const PRUnichar *aFontType)
{
  nsresult rv;
  nsCOMPtr<nsIFontEnumerator> fontEnumerator;

  fontEnumerator = do_CreateInstance(kCFontEnumerator, &rv);
  if (NS_FAILED(rv))
    return rv;
 
  nsXPIDLCString langGroup;
  langGroup.Adopt(ToNewUTF8String(nsDependentString(aLangGroup)));
  nsXPIDLCString fontType;
  fontType.Adopt(ToNewUTF8String(nsDependentString(aFontType)));
  rv = fontEnumerator->EnumerateFonts(langGroup.get(), fontType.get(), 
                                        &mCount, &mFonts);
  return rv;
}

NS_IMETHODIMP
nsFontListEnumerator::HasMoreElements(PRBool *result)
{
  *result = (mIndex < mCount);
  return NS_OK;
}

NS_IMETHODIMP
nsFontListEnumerator::GetNext(nsISupports **aFont)
{
  NS_ENSURE_ARG_POINTER(aFont);
  *aFont = nsnull;
  if (mIndex >= mCount) {
    return NS_ERROR_UNEXPECTED;
  }
  PRUnichar *fontName = mFonts[mIndex++];
  nsCOMPtr<nsISupportsString> fontNameWrapper;
  nsresult rv;
  fontNameWrapper = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(fontNameWrapper, NS_ERROR_OUT_OF_MEMORY);
  fontNameWrapper->SetData(nsDependentString(fontName));
  *aFont = NS_STATIC_CAST(nsISupports*, fontNameWrapper);
  NS_ADDREF(*aFont);
  return NS_OK;
}

NS_IMETHODIMP
nsFontList::AvailableFonts(const PRUnichar *aLangGroup, 
                           const PRUnichar *aFontType, 
                           nsISimpleEnumerator **aFontEnumerator)
{
  NS_ENSURE_ARG(aLangGroup);
  NS_ENSURE_ARG(aFontType);
  NS_ENSURE_ARG_POINTER(aFontEnumerator);
  nsCOMPtr<nsFontListEnumerator> fontListEnum = new nsFontListEnumerator();
  NS_ENSURE_TRUE(fontListEnum.get(), NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = fontListEnum->Init(aLangGroup, aFontType);
  NS_ENSURE_SUCCESS(rv, rv);

  *aFontEnumerator = NS_STATIC_CAST(nsISimpleEnumerator*, fontListEnum);
  NS_ADDREF(*aFontEnumerator);
  return NS_OK;
}

