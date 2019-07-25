









#include "timestamp_map.h"
#include <stdlib.h>
#include <assert.h>

namespace webrtc {



VCMTimestampMap::VCMTimestampMap(WebRtc_Word32 length):
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

WebRtc_Word32
VCMTimestampMap::Add(WebRtc_UWord32 timestamp, void* data)
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
VCMTimestampMap::Pop(WebRtc_UWord32 timestamp)
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
