






#ifndef MOZILLA_A11Y_ARIAGRIDACCESSIBLEWRAP_H
#define MOZILLA_A11Y_ARIAGRIDACCESSIBLEWRAP_H

#include "ARIAGridAccessible.h"
#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class ARIAGridAccessibleWrap : public ARIAGridAccessible,
                               public CAccessibleTable
{
public:
  ARIAGridAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    ARIAGridAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class ARIAGridCellAccessibleWrap : public ARIAGridCellAccessible,
                                   public CAccessibleTableCell
{
public:
  ARIAGridCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    ARIAGridCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif
