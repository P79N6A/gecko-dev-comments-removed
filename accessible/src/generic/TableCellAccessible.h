





#ifndef mozilla_a11y_TableCellAccessible_h__
#define mozilla_a11y_TableCellAccessible_h__

#include "nsTArray.h"
#include "mozilla/StandardInteger.h"

class Accessible;

namespace mozilla {
namespace a11y {

  class TableAccessible;




class TableCellAccessible
{
public:

  


  virtual TableAccessible* Table() { return nullptr; }

  


  virtual uint32_t ColIdx() { return 0; }

  


  virtual uint32_t RowIdx() { return 0; }

  


  virtual uint32_t ColExtent() { return 0; }

  


  virtual uint32_t RowExtent() { return 0; }

  


  virtual void ColHeaderCells(nsTArray<Accessible*>* aCells) { }

  


  virtual void RowHeaderCells(nsTArray<Accessible*>* aCells) { }

  


  virtual bool Selected() { return false; }
};

}
}

#endif 
