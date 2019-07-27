



#pragma once




#include <windows.h>   
#include <wlanapi.h>   


class WinWLANLibrary {
 public:
  static WinWLANLibrary* Load();
  ~WinWLANLibrary();

  HANDLE GetWLANHandle() const;
  decltype(::WlanEnumInterfaces)* GetWlanEnumInterfacesPtr() const;
  decltype(::WlanGetNetworkBssList)* GetWlanGetNetworkBssListPtr() const;
  decltype(::WlanFreeMemory)* GetWlanFreeMemoryPtr() const;
  decltype(::WlanCloseHandle)* GetWlanCloseHandlePtr() const;
  decltype(::WlanOpenHandle)* GetWlanOpenHandlePtr() const;
  decltype(::WlanRegisterNotification)* GetWlanRegisterNotificationPtr() const;
  decltype(::WlanScan)* GetWlanScanPtr() const;

 private:
  WinWLANLibrary();
  bool Initialize();

  HMODULE mWlanLibrary;
  HANDLE mWlanHandle;
  decltype(::WlanEnumInterfaces)* mWlanEnumInterfacesPtr;
  decltype(::WlanGetNetworkBssList)* mWlanGetNetworkBssListPtr;
  decltype(::WlanFreeMemory)* mWlanFreeMemoryPtr;
  decltype(::WlanCloseHandle)* mWlanCloseHandlePtr;
  decltype(::WlanOpenHandle)* mWlanOpenHandlePtr;
  decltype(::WlanRegisterNotification)* mWlanRegisterNotificationPtr;
  decltype(::WlanScan)* mWlanScanPtr;
};

class ScopedWLANObject {
public:
 ScopedWLANObject(WinWLANLibrary* library, void* object)
   : mObject(object),
    mLibrary(library)
  {
  }

  ~ScopedWLANObject()
  {
    (*(mLibrary->GetWlanFreeMemoryPtr()))(mObject);
  }

 private:
  WinWLANLibrary *mLibrary;
  void *mObject;
};
