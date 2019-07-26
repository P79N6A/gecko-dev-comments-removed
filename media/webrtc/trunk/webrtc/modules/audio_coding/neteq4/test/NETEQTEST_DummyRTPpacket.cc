









#include "NETEQTEST_DummyRTPpacket.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h> 
#endif

int NETEQTEST_DummyRTPpacket::readFromFile(FILE *fp)
{
    if (!fp)
    {
        return -1;
    }

    uint16_t length, plen;
    uint32_t offset;

    if (fread(&length, 2, 1, fp) == 0)
    {
        reset();
        return -2;
    }
    length = ntohs(length);

    if (fread(&plen, 2, 1, fp) == 0)
    {
        reset();
        return -1;
    }
    int packetLen = ntohs(plen);

    if (fread(&offset, 4, 1, fp) == 0)
    {
        reset();
        return -1;
    }
    
    uint32_t receiveTime = ntohl(offset);

    
    length = (uint16_t) (length - _kRDHeaderLen);

    
    if (_datagram && _memSize < length)
    {
        reset();
    }

    if (!_datagram)
    {
        _datagram = new uint8_t[length];
        _memSize = length;
    }
    memset(_datagram, 0, length);

    if (length == 0)
    {
        _datagramLen = 0;
        _rtpParsed = false;
        return packetLen;
    }

    
    if (fread((unsigned short *) _datagram, 1, _kBasicHeaderLen, fp)
        != (size_t)_kBasicHeaderLen)
    {
        reset();
        return -1;
    }
    _receiveTime = receiveTime;
    _datagramLen = _kBasicHeaderLen;

    
    webrtc::WebRtcRTPHeader tempRTPinfo;
    int P, X, CC;
    parseBasicHeader(&tempRTPinfo, &P, &X, &CC);

    
    if (X != 0 || CC != 0)
    {
        int newLen = _kBasicHeaderLen + CC * 4 + X * 4;
        assert(_memSize >= newLen);

        
        size_t readLen = newLen - _kBasicHeaderLen;
        if (fread((unsigned short *) _datagram + _kBasicHeaderLen, 1, readLen,
            fp) != readLen)
        {
            reset();
            return -1;
        }
        _datagramLen = newLen;

        if (X != 0)
        {
            int totHdrLen = calcHeaderLength(X, CC);
            assert(_memSize >= totHdrLen);

            
            size_t readLen = totHdrLen - newLen;
            if (fread((unsigned short *) _datagram + newLen, 1, readLen, fp)
                != readLen)
            {
                reset();
                return -1;
            }
            _datagramLen = totHdrLen;
        }
    }
    _datagramLen = length;

    if (!_blockList.empty() && _blockList.count(payloadType()) > 0)
    {
        
        return readFromFile(fp);
    }

    _rtpParsed = false;
    return packetLen;

}

int NETEQTEST_DummyRTPpacket::writeToFile(FILE *fp)
{
    if (!fp)
    {
        return -1;
    }

    uint16_t length, plen;
    uint32_t offset;

    
    length = htons(_datagramLen + _kRDHeaderLen);
    if (fwrite(&length, 2, 1, fp) != 1)
    {
        return -1;
    }

    
    plen = htons(_datagramLen);
    if (fwrite(&plen, 2, 1, fp) != 1)
    {
        return -1;
    }

    
    offset = htonl(_receiveTime);
    if (fwrite(&offset, 4, 1, fp) != 1)
    {
        return -1;
    }

    
    int headerLen;
    if (_datagramLen == 0)
    {
        
        headerLen = 0;
    }
    else
    {
        parseHeader();
        headerLen = _payloadPtr - _datagram;
        assert(headerLen >= 0);
    }

    
    if (fwrite((unsigned short *) _datagram, 1, headerLen, fp) !=
        static_cast<size_t>(headerLen))
    {
        return -1;
    }

    return (headerLen + _kRDHeaderLen); 

}

