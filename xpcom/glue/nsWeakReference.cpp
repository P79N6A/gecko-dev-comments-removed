









































#include "nsWeakReference.h"
#include "nsCOMPtr.h"

class nsWeakReference : public nsIWeakReference
  {
    public:
    
      NS_DECL_ISUPPORTS

    
      NS_DECL_NSIWEAKREFERENCE

    private:
      friend class nsSupportsWeakReference;

      nsWeakReference( nsSupportsWeakReference* referent )
          : mReferent(referent)
          
        {
          
        }

      ~nsWeakReference()
           
        {
          if ( mReferent )
            mReferent->NoticeProxyDestruction();
        }

      void
      NoticeReferentDestruction()
          
        {
          mReferent = 0;
        }

      nsSupportsWeakReference*  mReferent;
  };

nsresult
nsQueryReferent::operator()( const nsIID& aIID, void** answer ) const
  {
    nsresult status;
    if ( mWeakPtr )
      {
        if ( NS_FAILED(status = mWeakPtr->QueryReferent(aIID, answer)) )
          *answer = 0;
      }
    else
      status = NS_ERROR_NULL_POINTER;

    if ( mErrorPtr )
      *mErrorPtr = status;
    return status;
  }

NS_COM_GLUE nsIWeakReference*  
NS_GetWeakReference( nsISupports* aInstancePtr, nsresult* aErrorPtr )
  {
    nsresult status;

    nsIWeakReference* result = nsnull;

    if ( aInstancePtr )
      {
        nsCOMPtr<nsISupportsWeakReference> factoryPtr = do_QueryInterface(aInstancePtr, &status);
        NS_ASSERTION(factoryPtr, "Oops!  You're asking for a weak reference to an object that doesn't support that.");
        if ( factoryPtr )
          {
            status = factoryPtr->GetWeakReference(&result);
          }
        
      }
    else
      status = NS_ERROR_NULL_POINTER;

    if ( aErrorPtr )
      *aErrorPtr = status;
    return result;
  }

NS_COM_GLUE nsresult
nsSupportsWeakReference::GetWeakReference( nsIWeakReference** aInstancePtr )
  {
    if ( !aInstancePtr )
      return NS_ERROR_NULL_POINTER;

    if ( !mProxy )
      mProxy = new nsWeakReference(this);
    *aInstancePtr = mProxy;

    nsresult status;
    if ( !*aInstancePtr )
      status = NS_ERROR_OUT_OF_MEMORY;
    else
      {
        NS_ADDREF(*aInstancePtr);
        status = NS_OK;
      }

    return status;
  }

NS_IMPL_ISUPPORTS1(nsWeakReference, nsIWeakReference)

NS_IMETHODIMP
nsWeakReference::QueryReferent( const nsIID& aIID, void** aInstancePtr )
  {
    return mReferent ? mReferent->QueryInterface(aIID, aInstancePtr) : NS_ERROR_NULL_POINTER;
  }

void
nsSupportsWeakReference::ClearWeakReferences()
	{
		if ( mProxy )
			{
				mProxy->NoticeReferentDestruction();
				mProxy = 0;
			}
	}

