






#ifndef MOZILLA_A11Y_ARIAGRIDACCESSIBLEWRAP_H
#define MOZILLA_A11Y_ARIAGRIDACCESSIBLEWRAP_H

#include "ARIAGridAccessible.h"
#include "ia2AccessibleTable.h"
#include "ia2AccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class ARIAGridAccessibleWrap : public ARIAGridAccessible,
                               public ia2AccessibleTable
{
public:
  ARIAGridAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    ARIAGridAccessible(aContent, aDoc), ia2AccessibleTable(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};





class ARIAGridCellAccessibleWrap : public ARIAGridCellAccessible,
                                   public ia2AccessibleTableCell
{
public:
  ARIAGridCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    ARIAGridCellAccessible(aContent, aDoc), ia2AccessibleTableCell(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};

} 
} 

#endif
