




































#ifndef nsHtml5OwningUTF16Buffer_h_
#define nsHtml5OwningUTF16Buffer_h_

#include "nsHtml5UTF16Buffer.h"

class nsHtml5OwningUTF16Buffer : public nsHtml5UTF16Buffer
{
  private:

    


    nsHtml5OwningUTF16Buffer(PRUnichar* aBuffer);

  public:

    



    nsHtml5OwningUTF16Buffer(void* aKey);

    


    ~nsHtml5OwningUTF16Buffer();

    


    nsRefPtr<nsHtml5OwningUTF16Buffer> next;

    


    void* key;

    static already_AddRefed<nsHtml5OwningUTF16Buffer>
    FalliblyCreate(PRInt32 aLength);

    


    void Swap(nsHtml5OwningUTF16Buffer* aOther);

    nsrefcnt AddRef();
    nsrefcnt Release();
  private:
    nsAutoRefCnt mRefCnt;
};

#endif 
