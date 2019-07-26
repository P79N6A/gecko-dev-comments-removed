






































#pragma once
































#include <map>
#include "SharedPtr.h"
#include "base/lock.h"

template <class T>
class Wrapper
{
private:
    typedef std::map<typename T::Handle, typename T::Ptr>      	HandleMapType;
	HandleMapType 	handleMap;
	Lock 		handleMapMutex;

public:
	typename T::Ptr wrap(typename T::Handle handle)
	{
		AutoLock lock(handleMapMutex);
		typename HandleMapType::iterator it = handleMap.find(handle);
		if(it != handleMap.end())
		{
			return it->second;
		}
		else
		{
			typename T::Ptr p(new T(handle));
			handleMap[handle] = p;
			return p;
		}
	}

	bool changeHandle(typename T::Handle oldHandle, typename T::Handle newHandle)
	{
		AutoLock lock(handleMapMutex);
		typename HandleMapType::iterator it = handleMap.find(oldHandle);
		if(it != handleMap.end())
		{
			typename T::Ptr p = it->second;
			handleMap.erase(it);
			handleMap[newHandle] = p;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool release(typename T::Handle handle)
	{
		AutoLock lock(handleMapMutex);
		typename HandleMapType::iterator it = handleMap.find(handle);
		if(it != handleMap.end())
		{
			handleMap.erase(it);
			return true;
		}
		else
		{
			return false;
		}
	}

	void reset()
	{
		AutoLock lock(handleMapMutex);
		handleMap.clear();
	}
};

#define CSF_DECLARE_WRAP(classname, handletype) \
	public: \
		static classname ## Ptr wrap(handletype handle); \
		static void reset(); \
	private: \
		friend class Wrapper<classname>; \
		typedef classname ## Ptr Ptr; \
		typedef handletype Handle; \
		static Wrapper<classname> wrapper;

#define CSF_IMPLEMENT_WRAP(classname, handletype) \
	Wrapper<classname> classname::wrapper; \
	classname ## Ptr classname::wrap(handletype handle) \
	{ \
		return wrapper.wrap(handle); \
	} \
	void classname::reset() \
	{ \
		wrapper.reset(); \
	}

