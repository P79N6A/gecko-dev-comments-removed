



#ifndef nsHtml5DependentUTF16Buffer_h_
#define nsHtml5DependentUTF16Buffer_h_

#include "nscore.h"
#include "nsHtml5OwningUTF16Buffer.h"

class MOZ_STACK_CLASS nsHtml5DependentUTF16Buffer : public nsHtml5UTF16Buffer
{
  public:
    



    nsHtml5DependentUTF16Buffer(const nsAString& aToWrap);

    ~nsHtml5DependentUTF16Buffer();

    





    already_AddRefed<nsHtml5OwningUTF16Buffer> FalliblyCopyAsOwningBuffer();
};

#endif 
