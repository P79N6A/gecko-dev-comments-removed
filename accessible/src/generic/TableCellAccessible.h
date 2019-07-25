





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

  


  virtual TableAccessible* Table() const = 0;

  


  virtual uint32_t ColIdx() const = 0;

  


  virtual uint32_t RowIdx() const = 0;

  


  virtual uint32_t ColExtent() const { return 1; }

  


  virtual uint32_t RowExtent() const { return 1; }

  


  virtual void ColHeaderCells(nsTArray<Accessible*>* aCells);

  


  virtual void RowHeaderCells(nsTArray<Accessible*>* aCells);

  


  virtual bool Selected() = 0;
};

} 
} 

#endif 
