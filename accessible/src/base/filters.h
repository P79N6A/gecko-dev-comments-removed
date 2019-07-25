




































#ifndef a11yFilters_h_
#define a11yFilters_h_

class nsAccessible;




namespace filters {

  


  typedef bool (*FilterFuncPtr) (nsAccessible*);

  bool GetSelected(nsAccessible* aAccessible);
  bool GetSelectable(nsAccessible* aAccessible);
  bool GetRow(nsAccessible* aAccessible);
  bool GetCell(nsAccessible* aAccessible);
  bool GetEmbeddedObject(nsAccessible* aAccessible);
}

#endif
