









#ifndef mozilla_a11y_DocAccessibleWrap_h__
#define mozilla_a11y_DocAccessibleWrap_h__

#include "DocAccessible.h"

namespace mozilla {
namespace a11y {

class DocAccessibleWrap : public DocAccessible
{
public:
  DocAccessibleWrap(nsIDocument* aDocument, nsIContent* aRootContent,
                    nsIPresShell* aPresShell);
  virtual ~DocAccessibleWrap();

  bool mActivated;
};

} 
} 

#endif
