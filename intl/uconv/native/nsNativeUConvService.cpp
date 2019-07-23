





































#ifdef MOZ_USE_NATIVE_UCONV
#include "nsString.h"
#include "nsIGenericFactory.h"

#include "nsINativeUConvService.h"

#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharRepresentable.h"

#include "nsNativeUConvService.h"

#include <nl_types.h> 
#include <langinfo.h> 
#include <iconv.h>    
#include <errno.h>


class IConvAdaptor : public nsIUnicodeDecoder, 
                     public nsIUnicodeEncoder, 
                     public nsICharRepresentable
{
public:
    IConvAdaptor();
    virtual ~IConvAdaptor();
    
    nsresult Init(const char* from, const char* to);
    
    NS_DECL_ISUPPORTS
    
    
    
    NS_IMETHOD Convert(const char * aSrc, 
                       PRInt32 * aSrcLength, 
                       PRUnichar * aDest, 
                       PRInt32 * aDestLength);
    
    NS_IMETHOD GetMaxLength(const char * aSrc, 
                            PRInt32 aSrcLength, 
                            PRInt32 * aDestLength);
    NS_IMETHOD Reset();
    
    
    
    NS_IMETHOD Convert(const PRUnichar * aSrc, 
                       PRInt32 * aSrcLength, 
                       char * aDest, 
                       PRInt32 * aDestLength);
    
    
    NS_IMETHOD Finish(char * aDest, PRInt32 * aDestLength);
    
    NS_IMETHOD GetMaxLength(const PRUnichar * aSrc, 
                            PRInt32 aSrcLength, 
                            PRInt32 * aDestLength);
    
    
    
    NS_IMETHOD SetOutputErrorBehavior(PRInt32 aBehavior, 
                                      nsIUnicharEncoder * aEncoder, 
                                      PRUnichar aChar);
    
    NS_IMETHOD FillInfo(PRUint32* aInfo);
    
    
private:
    nsresult ConvertInternal(void * aSrc, 
                             PRInt32 * aSrcLength, 
                             PRInt32 aSrcCharSize,
                             void * aDest, 
                             PRInt32 * aDestLength,
                             PRInt32 aDestCharSize);
    
    
    iconv_t mConverter;
    PRBool    mReplaceOnError;
    PRUnichar mReplaceChar;

#ifdef DEBUG
    nsCString mFrom, mTo;
#endif
};

NS_IMPL_ISUPPORTS3(IConvAdaptor, 
                   nsIUnicodeEncoder, 
                   nsIUnicodeDecoder,
                   nsICharRepresentable)

IConvAdaptor::IConvAdaptor()
{
    mConverter = 0;
    mReplaceOnError = PR_FALSE;
}

IConvAdaptor::~IConvAdaptor()
{
    if (mConverter)
        iconv_close(mConverter);
}

nsresult 
IConvAdaptor::Init(const char* from, const char* to)
{
#ifdef DEBUG
    mFrom = from;
    mTo = to;
#endif

    mConverter = iconv_open(to, from);
    if (mConverter == (iconv_t) -1 )    
    {
#ifdef DEBUG
        printf(" * IConvAdaptor - FAILED Initing: %s ==> %s\n", from, to);
#endif
        mConverter = nsnull;
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}


nsresult 
IConvAdaptor::Convert(const char * aSrc, 
                     PRInt32 * aSrcLength, 
                     PRUnichar * aDest, 
                     PRInt32 * aDestLength)
{
    return ConvertInternal( (void*) aSrc, 
                            aSrcLength, 
                            1,
                            (void*) aDest, 
                            aDestLength,
                            2);
}

nsresult
IConvAdaptor::GetMaxLength(const char * aSrc, 
                          PRInt32 aSrcLength, 
                          PRInt32 * aDestLength)
{
    if (!mConverter)
        return NS_ERROR_UENC_NOMAPPING;

    *aDestLength = aSrcLength*4; 
#ifdef DEBUG
    printf(" * IConvAdaptor - - GetMaxLength %d ( %s -> %s )\n", *aDestLength, mFrom.get(), mTo.get());
#endif
    return NS_OK;
}


nsresult 
IConvAdaptor::Reset()
{
    const char *zero_char_in_ptr  = NULL;
    char       *zero_char_out_ptr = NULL;
    size_t      zero_size_in      = 0,
                zero_size_out     = 0;

    iconv(mConverter, 
          (char **)&zero_char_in_ptr,
          &zero_size_in,
          &zero_char_out_ptr,
          &zero_size_out);

#ifdef DEBUG
    printf(" * IConvAdaptor - - Reset\n");
#endif
    return NS_OK;
}



nsresult 
IConvAdaptor::Convert(const PRUnichar * aSrc, 
                     PRInt32 * aSrcLength, 
                     char * aDest, 
                     PRInt32 * aDestLength)
{
    return ConvertInternal( (void*) aSrc, 
                            aSrcLength, 
                            2,
                            (void*) aDest, 
                            aDestLength,
                            1);
}


nsresult 
IConvAdaptor::Finish(char * aDest, PRInt32 * aDestLength)
{
    *aDestLength = 0;
    return NS_OK;
}

nsresult 
IConvAdaptor::GetMaxLength(const PRUnichar * aSrc, 
                          PRInt32 aSrcLength, 
                          PRInt32 * aDestLength)
{
    if (!mConverter)
        return NS_ERROR_UENC_NOMAPPING;

    *aDestLength = aSrcLength*4; 

    return NS_OK;
}


nsresult 
IConvAdaptor::SetOutputErrorBehavior(PRInt32 aBehavior, 
                                    nsIUnicharEncoder * aEncoder, 
                                    PRUnichar aChar)
{
    if (aBehavior == kOnError_Signal) {
        mReplaceOnError = PR_FALSE;
        return NS_OK;
    }
    else if (aBehavior != kOnError_Replace) {
        mReplaceOnError = PR_TRUE;
        mReplaceChar = aChar;
        return NS_OK;
    }

    NS_WARNING("Uconv Error Behavior not support");
    return NS_ERROR_FAILURE;
}

nsresult 
IConvAdaptor::FillInfo(PRUint32* aInfo)
{
#ifdef DEBUG
    printf(" * IConvAdaptor - FillInfo called\n");
#endif
    *aInfo = 0;
    return NS_OK;
}


nsresult 
IConvAdaptor::ConvertInternal(void * aSrc, 
                             PRInt32 * aSrcLength, 
                             PRInt32 aSrcCharSize,
                             void * aDest, 
                             PRInt32 * aDestLength,
                             PRInt32 aDestCharSize)
{
    if (!mConverter) {
        NS_WARNING("Converter Not Initialize");
        return NS_ERROR_NOT_INITIALIZED;
    }
    size_t res = 0;
    size_t inLeft = (size_t) *aSrcLength * aSrcCharSize;
    size_t outLeft = (size_t) *aDestLength * aDestCharSize;
    size_t outputAvail = outLeft;

    while (true){

        res = iconv(mConverter, 
                    (char**)&aSrc, 
                    &inLeft, 
                    (char**)&aDest, 
                    &outLeft);
        
        if (res == (size_t) -1) {
            
            
            
            
            
            if ((errno == E2BIG) && (outLeft < outputAvail)) {
                res = 0;
                break;
            }
            
            if (errno == EILSEQ) {

                if (mReplaceOnError) {
                    if (aDestCharSize == 1) {
                        (*(char*)aDest) = (char)mReplaceChar;
                        aDest = (char*)aDest + sizeof(char);
                    }
                    else
                    {
                        (*(PRUnichar*)aDest) = (PRUnichar)mReplaceChar;
                        aDest = (PRUnichar*)aDest + sizeof(PRUnichar);
                    
                    }
                    inLeft -= aSrcCharSize;
                    outLeft -= aDestCharSize;

#ifdef DEBUG
                    printf(" * IConvAdaptor - replacing char in output  ( %s -> %s )\n", 
                           mFrom.get(), mTo.get());

#endif
                    res = 0;
                }
            }

            if (res == -1) {
#ifdef DEBUG
                printf(" * IConvAdaptor - Bad input ( %s -> %s )\n", mFrom.get(), mTo.get());
#endif
                return NS_ERROR_UENC_NOMAPPING;
            }
        }

        if (inLeft <= 0 || outLeft <= 0 || res == -1)
            break;
    }


    if (res != (size_t) -1) {

        
        
        
        *aSrcLength  -= (inLeft  / aSrcCharSize);
        *aDestLength -= (outLeft / aDestCharSize);
        return NS_OK;
    }
    
#ifdef DEBUG
    printf(" * IConvAdaptor - - xp_iconv error( %s -> %s )\n", mFrom.get(), mTo.get());
#endif
    Reset();    
    return NS_ERROR_UENC_NOMAPPING;
}

NS_IMPL_ISUPPORTS1(NativeUConvService, nsINativeUConvService)

NS_IMETHODIMP 
NativeUConvService::GetNativeConverter(const char* from,
                                       const char* to,
                                       nsISupports** aResult) 
{
    *aResult = nsnull;

    nsRefPtr<IConvAdaptor> ucl = new IConvAdaptor();
    if (!ucl)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = ucl->Init(from, to);
    if (NS_SUCCEEDED(rv))
        NS_ADDREF(*aResult = ucl);

    return rv;
}
#endif
