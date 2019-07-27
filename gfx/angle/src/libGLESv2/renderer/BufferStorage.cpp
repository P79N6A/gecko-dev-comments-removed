#include "precompiled.h"








#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{

unsigned int BufferStorage::mNextSerial = 1;

BufferStorage::BufferStorage()
{
    updateSerial();
}

BufferStorage::~BufferStorage()
{
}

unsigned int BufferStorage::getSerial() const
{
    return mSerial;
}

void BufferStorage::updateSerial()
{
    mSerial = mNextSerial++;
}

}
