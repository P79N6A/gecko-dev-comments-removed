





#ifndef ObservedDocShell_h_
#define ObservedDocShell_h_

#include "nsRefPtr.h"

class nsDocShell;

namespace mozilla {





class ObservedDocShell : public LinkedListElement<ObservedDocShell>
{
private:
  nsRefPtr<nsDocShell> mDocShell;

public:
  explicit ObservedDocShell(nsDocShell* aDocShell);
  nsDocShell* operator*() const { return mDocShell.get(); }
};

} 

#endif 
