



#include "CallControlManager.h"
#include "CallControlManagerImpl.h"

namespace CSF
{
CallControlManagerPtr CallControlManager::create()
{
	CallControlManagerPtr instance(new CallControlManagerImpl());
	return instance;
}

CallControlManager::~CallControlManager()
{
}

}
