









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_MAP_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_MAP_WRAPPER_H_

#include <map>

#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

class MapItem {
 public:
  MapItem(int id, void* ptr);
  virtual ~MapItem();
  void* GetItem();
  int GetId();
  unsigned int GetUnsignedId();
  void SetItem(void* ptr);

 private:
  friend class MapWrapper;

  int   item_id_;
  void* item_pointer_;
};

class MapWrapper {
 public:
  MapWrapper();
  ~MapWrapper();

  
  
  int Insert(int id, void* ptr);

  
  int Erase(MapItem* item);

  
  int Erase(int id);

  
  int Size() const;

  
  MapItem* First() const;

  
  MapItem* Last() const;

  
  MapItem* Next(MapItem* item) const;

  
  MapItem* Previous(MapItem* item) const;

  
  MapItem* Find(int id) const;

 private:
  std::map<int, MapItem*>    map_;
};

} 

#endif  
