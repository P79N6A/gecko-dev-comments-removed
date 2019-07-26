









#include <assert.h>
#include <stdlib.h>
#include "webrtc/modules/video_coding/main/source/timestamp_map.h"

namespace webrtc {



VCMTimestampMap::VCMTimestampMap(int32_t length):
    _nextAddIx(0),
    _nextPopIx(0)
{
    if (length <= 0)
    {
        
        length = 10;
    }

    _map = new VCMTimestampDataTuple[length];
    _length = length;
}


VCMTimestampMap::~VCMTimestampMap()
{
    delete [] _map;
}


void
VCMTimestampMap::Reset()
{
    _nextAddIx = 0;
    _nextPopIx = 0;
}

int32_t
VCMTimestampMap::Add(uint32_t timestamp, void* data)
{
    _map[_nextAddIx].timestamp = timestamp;
    _map[_nextAddIx].data = data;
    _nextAddIx = (_nextAddIx + 1) % _length;

    if (_nextAddIx == _nextPopIx)
    {
        
        _nextPopIx = (_nextPopIx + 1) % _length;
        return -1;
    }
    return 0;
}

void*
VCMTimestampMap::Pop(uint32_t timestamp)
{
    while (!IsEmpty())
    {
        if (_map[_nextPopIx].timestamp == timestamp)
        {
            
            void* data = _map[_nextPopIx].data;
            _map[_nextPopIx].data = NULL;
            _nextPopIx = (_nextPopIx + 1) % _length;
            return data;
        }
        else if (_map[_nextPopIx].timestamp > timestamp)
        {
            
            assert(_nextPopIx < _length && _nextPopIx >= 0);
            return NULL;
        }

        
        _nextPopIx = (_nextPopIx + 1) % _length;
    }

    
    assert(_nextPopIx < _length && _nextPopIx >= 0);
    return NULL;
}


bool
VCMTimestampMap::IsEmpty() const
{
    return (_nextAddIx == _nextPopIx);
}

}
