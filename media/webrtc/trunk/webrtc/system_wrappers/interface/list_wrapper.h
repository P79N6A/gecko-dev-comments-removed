









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_LIST_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_LIST_WRAPPER_H_

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

class CriticalSectionWrapper;

class ListItem {
  friend class ListWrapper;

 public:
  ListItem(const void* ptr);
  ListItem(const unsigned int item);
  virtual ~ListItem();
  void* GetItem() const;
  unsigned int GetUnsignedItem() const;

 protected:
  ListItem* next_;
  ListItem* prev_;

 private:
  const void*         item_ptr_;
  const unsigned int  item_;
};

class ListWrapper {
 public:
  ListWrapper();
  virtual ~ListWrapper();

  
  unsigned int GetSize() const;

  
  int PushBack(const void* ptr);
  
  int PushFront(const void* ptr);

  
  int PushBack(const unsigned int item_id);
  
  int PushFront(const unsigned int item_id);

  
  int PopFront();

  
  int PopBack();

  
  bool Empty() const;

  
  ListItem* First() const;

  
  ListItem* Last() const;

  
  ListItem* Next(ListItem* item) const;

  
  ListItem* Previous(ListItem* item) const;

  
  int Erase(ListItem* item);

  
  
  
  
  int Insert(ListItem* existing_previous_item,
             ListItem* new_item);

  
  
  
  
  int InsertBefore(ListItem* existing_next_item,
                   ListItem* new_item);

 private:
  void PushBackImpl(ListItem* item);
  void PushFrontImpl(ListItem* item);

  CriticalSectionWrapper* critical_section_;
  ListItem* first_;
  ListItem* last_;
  unsigned int size_;
};

}  

#endif  
