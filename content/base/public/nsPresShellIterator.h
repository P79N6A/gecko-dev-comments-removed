




































#ifndef nsPresShellIterato_h___
#define nsPresShellIterato_h___

#include "nsIPresShell.h"
#include "nsIDocument.h"

class nsPresShellIterator :
  private nsTObserverArray<nsIPresShell>::ForwardIterator
{
public:
  nsPresShellIterator(nsIDocument* aDoc)
  : nsTObserverArray<nsIPresShell>::ForwardIterator(aDoc->mPresShells),
    mDoc(aDoc) {}

  already_AddRefed<nsIPresShell> GetNextShell()
  {
    nsIPresShell* shell = nsnull;
    if (!mDoc->ShellsAreHidden()) {
      shell = GetNext();
      NS_IF_ADDREF(shell);
    }
    return shell;
  }
private:
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}

  nsCOMPtr<nsIDocument> mDoc;
};

#endif
