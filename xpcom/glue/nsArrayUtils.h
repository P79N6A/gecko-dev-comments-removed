





































#ifndef nsArrayUtils_h__
#define nsArrayUtils_h__

#include "nsCOMPtr.h"
#include "nsIArray.h"


class NS_COM_GLUE nsQueryArrayElementAt : public nsCOMPtr_helper
  {
    public:
      nsQueryArrayElementAt(nsIArray* aArray, PRUint32 aIndex,
                            nsresult* aErrorPtr)
          : mArray(aArray),
            mIndex(aIndex),
            mErrorPtr(aErrorPtr)
        {
          
        }

      virtual nsresult NS_FASTCALL operator()(const nsIID& aIID, void**) const;

    private:
      nsIArray*  mArray;
      PRUint32   mIndex;
      nsresult*  mErrorPtr;
  };

inline
const nsQueryArrayElementAt
do_QueryElementAt(nsIArray* aArray, PRUint32 aIndex, nsresult* aErrorPtr = 0)
  {
    return nsQueryArrayElementAt(aArray, aIndex, aErrorPtr);
  }

#endif 
