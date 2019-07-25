










#include "RefCountObject.h"

RefCountObject::RefCountObject(GLuint id)
{
    mId = id;
    mRefCount = 0;
}

RefCountObject::~RefCountObject()
{
    ASSERT(mRefCount == 0);
}

void RefCountObject::addRef() const
{
    mRefCount++;
}

void RefCountObject::release() const
{
    ASSERT(mRefCount > 0);

    if (--mRefCount == 0)
    {
        delete this;
    }
}

void RefCountObjectBindingPointer::set(RefCountObject *newObject)
{
    
    if (newObject != NULL) newObject->addRef();
    if (mObject != NULL) mObject->release();

    mObject = newObject;
}
