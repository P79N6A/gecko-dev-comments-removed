



#ifndef nsHtml5OwningUTF16Buffer_h
#define nsHtml5OwningUTF16Buffer_h

#include "nsHtml5UTF16Buffer.h"

class nsHtml5OwningUTF16Buffer : public nsHtml5UTF16Buffer
{
  private:

    


    explicit nsHtml5OwningUTF16Buffer(char16_t* aBuffer);

  public:

    



    explicit nsHtml5OwningUTF16Buffer(void* aKey);

protected:
    


    ~nsHtml5OwningUTF16Buffer();

public:
    


    nsRefPtr<nsHtml5OwningUTF16Buffer> next;

    


    void* key;

    static already_AddRefed<nsHtml5OwningUTF16Buffer>
    FalliblyCreate(int32_t aLength);

    


    void Swap(nsHtml5OwningUTF16Buffer* aOther);

    nsrefcnt AddRef();
    nsrefcnt Release();
  private:
    nsAutoRefCnt mRefCnt;
};

#endif 
