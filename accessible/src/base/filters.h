



#ifndef a11yFilters_h_
#define a11yFilters_h_

class Accessible;




namespace filters {

  


  typedef bool (*FilterFuncPtr) (Accessible*);

  bool GetSelected(Accessible* aAccessible);
  bool GetSelectable(Accessible* aAccessible);
  bool GetRow(Accessible* aAccessible);
  bool GetCell(Accessible* aAccessible);
  bool GetEmbeddedObject(Accessible* aAccessible);
}

#endif
