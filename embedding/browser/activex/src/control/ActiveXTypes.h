




































#ifndef ACTIVEXTYPES_H
#define ACTIVEXTYPES_H

#include <vector>
#include <map>


typedef std::basic_string<TCHAR> tstring;


typedef CComPtr<IUnknown> CIUnkPtr;


#define CIPtr(iface) CComQIPtr< iface, &IID_ ## iface >

typedef std::vector<CIUnkPtr> CObjectList;
typedef std::map<tstring, CIUnkPtr> CNamedObjectList;

#endif