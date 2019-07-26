









#ifndef WEBRTC_SYSTEM_WRAPPERS_SOURCE_LIST_NO_STL_H_
#define WEBRTC_SYSTEM_WRAPPERS_SOURCE_LIST_NO_STL_H_

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

class CriticalSectionWrapper;

class ListNoStlItem {
 public:
  ListNoStlItem(const void* ptr);
  ListNoStlItem(const unsigned int item);
  virtual ~ListNoStlItem();
  void* GetItem() const;
  unsigned int GetUnsignedItem() const;

 protected:
  ListNoStlItem* next_;
  ListNoStlItem* prev_;

 private:
  friend class ListNoStl;

  const void*         item_ptr_;
  const unsigned int  item_;
  DISALLOW_COPY_AND_ASSIGN(ListNoStlItem);
};

class ListNoStl {
 public:
  ListNoStl();
  virtual ~ListNoStl();

  
  unsigned int GetSize() const;
  int PushBack(const void* ptr);
  int PushBack(const unsigned int item_id);
  int PushFront(const void* ptr);
  int PushFront(const unsigned int item_id);
  int PopFront();
  int PopBack();
  bool Empty() const;
  ListNoStlItem* First() const;
  ListNoStlItem* Last() const;
  ListNoStlItem* Next(ListNoStlItem* item) const;
  ListNoStlItem* Previous(ListNoStlItem* item) const;
  int Erase(ListNoStlItem* item);
  int Insert(ListNoStlItem* existing_previous_item,
             ListNoStlItem* new_item);

  int InsertBefore(ListNoStlItem* existing_next_item,
                   ListNoStlItem* new_item);

 private:
  void PushBack(ListNoStlItem* item);
  void PushFront(ListNoStlItem* item);

  CriticalSectionWrapper* critical_section_;
  ListNoStlItem* first_;
  ListNoStlItem* last_;
  unsigned int size_;
  DISALLOW_COPY_AND_ASSIGN(ListNoStl);
};

}  

#endif  
