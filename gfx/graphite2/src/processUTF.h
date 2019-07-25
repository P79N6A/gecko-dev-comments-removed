

























#pragma once 

#include "Main.h"
#include "graphite2/Segment.h"

namespace graphite2 {

class NoLimit		
{
public:
    NoLimit(gr_encform enc2, const void* pStart2) : m_enc(enc2), m_pStart(pStart2) {}
    gr_encform enc() const { return m_enc; }
    const void* pStart() const { return m_pStart; }

    bool inBuffer(const void* , uint32 ) const { return true; }
    bool needMoreChars(const void* , size_t ) const { return true; }
    
private:
    gr_encform m_enc;
    const void* m_pStart;
};


class CharacterCountLimit
{
public:
    CharacterCountLimit(gr_encform enc2, const void* pStart2, size_t numchars) : m_numchars(numchars), m_enc(enc2), m_pStart(pStart2) {}
    gr_encform enc() const { return m_enc; }
    const void* pStart() const { return m_pStart; }

    bool inBuffer (const void* , uint32 val) const { return (val != 0); }
    bool needMoreChars (const void* , size_t nProcessed) const { return nProcessed<m_numchars; }
    
private:
    size_t m_numchars;
    gr_encform m_enc;
    const void* m_pStart;
};


class BufferLimit
{
public:
    BufferLimit(gr_encform enc2, const void* pStart2, const void* pEnd) : m_enc(enc2), m_pStart(pStart2) {
	size_t nFullTokens = (static_cast<const char*>(pEnd)-static_cast<const char *>(m_pStart))/int(m_enc); 
	m_pEnd = static_cast<const char *>(m_pStart) + (nFullTokens*int(m_enc));
    }
    gr_encform enc() const { return m_enc; }
    const void* pStart() const { return m_pStart; }
  
    bool inBuffer (const void* pCharLastSurrogatePart, uint32 ) const { return pCharLastSurrogatePart<m_pEnd; }	

    bool needMoreChars (const void* pCharStart, size_t ) const { return inBuffer(pCharStart, 1); }
     
private:
    const void* m_pEnd;
    gr_encform m_enc;
    const void* m_pStart;
};


class IgnoreErrors
{
public:
    
    static bool ignoreUnicodeOutOfRangeErrors(bool ) { return true; }
    static bool ignoreBadSurrogatesErrors(bool ) { return true; }

    static bool handleError(const void* ) { return true;}
};


class BreakOnError
{
public:
    BreakOnError() : m_pErrorPos(NULL) {}
    
    
    static bool ignoreUnicodeOutOfRangeErrors(bool isBad) { return !isBad; }
    static bool ignoreBadSurrogatesErrors(bool isBad) { return !isBad; }

    bool handleError(const void* pPositionOfError) { m_pErrorPos=pPositionOfError; return false;}

public:
    const void* m_pErrorPos;
};














inline unsigned int utf8_extrabytes(const unsigned int topNibble) { return (0xE5FF0000>>(2*topNibble))&0x3; }

inline unsigned int utf8_mask(const unsigned int seq_extra) { return ((0xFEC0>>(4*seq_extra))&0xF)<<4; }

class Utf8Consumer
{
public:
    Utf8Consumer(const uint8* pCharStart2) : m_pCharStart(pCharStart2) {}
    
    const uint8* pCharStart() const { return m_pCharStart; }

private:
    template <class ERRORHANDLER>
    bool respondToError(uint32* pRes, ERRORHANDLER* pErrHandler) {       
        *pRes = 0xFFFD;
        if (!pErrHandler->handleError(m_pCharStart)) {
            return false;
        }                          
        ++m_pCharStart; 
        return true;
    }
    
public:
    template <class LIMIT, class ERRORHANDLER>
    inline bool consumeChar(const LIMIT& limit, uint32* pRes, ERRORHANDLER* pErrHandler) {			
        const unsigned int seq_extra = utf8_extrabytes(*m_pCharStart >> 4);        
        if (!limit.inBuffer(m_pCharStart+(seq_extra), *m_pCharStart)) {
            return false;
        }
    
        *pRes = *m_pCharStart ^ utf8_mask(seq_extra);
        
        if (seq_extra) {
            switch(seq_extra) {    
                case 3: {	
                    if (pErrHandler->ignoreUnicodeOutOfRangeErrors(*m_pCharStart>=0xF8)) {		
                        ++m_pCharStart;
                        if (!pErrHandler->ignoreBadSurrogatesErrors((*m_pCharStart&0xC0)!=0x80)) {
                            return respondToError(pRes, pErrHandler);
                        }           
                        
                        *pRes <<= 6; *pRes |= *m_pCharStart & 0x3F;		
                    }
                    else {
                        return respondToError(pRes, pErrHandler);
                    }		    
                }
                case 2: {
                    ++m_pCharStart;
                    if (!pErrHandler->ignoreBadSurrogatesErrors((*m_pCharStart&0xC0)!=0x80)) {
                        return respondToError(pRes, pErrHandler);
                    }
                }           
                *pRes <<= 6; *pRes |= *m_pCharStart & 0x3F;       
                case 1: {
                    ++m_pCharStart;
                    if (!pErrHandler->ignoreBadSurrogatesErrors((*m_pCharStart&0xC0)!=0x80)) {
                        return respondToError(pRes, pErrHandler);
                    }
                }           
                *pRes <<= 6; *pRes |= *m_pCharStart & 0x3F;
             }
        }
        ++m_pCharStart; 
        return true;
    }	
  
private:
    const uint8 *m_pCharStart;
};



class Utf16Consumer
{
private:
    static const unsigned int SURROGATE_OFFSET = 0x10000 - 0xDC00;

public:
      Utf16Consumer(const uint16* pCharStart2) : m_pCharStart(pCharStart2) {}
      
      const uint16* pCharStart() const { return m_pCharStart; }
  
private:
    template <class ERRORHANDLER>
    bool respondToError(uint32* pRes, ERRORHANDLER* pErrHandler) {       
        *pRes = 0xFFFD;
        if (!pErrHandler->handleError(m_pCharStart)) {
            return false;
        }                          
        ++m_pCharStart; 
        return true;
    }
    
public:
      template <class LIMIT, class ERRORHANDLER>
      inline bool consumeChar(const LIMIT& limit, uint32* pRes, ERRORHANDLER* pErrHandler)			
      {
	  *pRes = *m_pCharStart;
      if (0xD800 > *pRes || pErrHandler->ignoreUnicodeOutOfRangeErrors(*pRes >= 0xE000)) {
          ++m_pCharStart;
          return true;
      }
      
      if (!pErrHandler->ignoreBadSurrogatesErrors(*pRes >= 0xDC00)) {        
          return respondToError(pRes, pErrHandler);
      }

      ++m_pCharStart;
	  if (!limit.inBuffer(m_pCharStart, *pRes)) {
	      return false;
	  }

	  uint32 ul = *(m_pCharStart);
	  if (!pErrHandler->ignoreBadSurrogatesErrors(0xDC00 > ul || ul > 0xDFFF)) {
          return respondToError(pRes, pErrHandler);
	  }
	  ++m_pCharStart;
	  *pRes =  (*pRes<<10) + ul + SURROGATE_OFFSET;
	  return true;
      }

private:
      const uint16 *m_pCharStart;
};


class Utf32Consumer
{
public:
      Utf32Consumer(const uint32* pCharStart2) : m_pCharStart(pCharStart2) {}
      
      const uint32* pCharStart() const { return m_pCharStart; }
  
private:
    template <class ERRORHANDLER>
    bool respondToError(uint32* pRes, ERRORHANDLER* pErrHandler) {       
        *pRes = 0xFFFD;
        if (!pErrHandler->handleError(m_pCharStart)) {
            return false;
        }                          
        ++m_pCharStart; 
        return true;
    }

public:
      template <class LIMIT, class ERRORHANDLER>
      inline bool consumeChar(const LIMIT& limit, uint32* pRes, ERRORHANDLER* pErrHandler)			
      {
	  *pRes = *m_pCharStart;
      if (pErrHandler->ignoreUnicodeOutOfRangeErrors(!(*pRes<0xD800 || (*pRes>=0xE000 && *pRes<0x110000)))) {
          if (!limit.inBuffer(++m_pCharStart, *pRes))
            return false;
          else
            return true;
      }
      
      return respondToError(pRes, pErrHandler);
      }

private:
      const uint32 *m_pCharStart;
};














































template <class LIMIT, class CHARPROCESSOR, class ERRORHANDLER>
void processUTF(const LIMIT& limit, CHARPROCESSOR* pProcessor, ERRORHANDLER* pErrHandler)
{
     uint32             cid;
     switch (limit.enc()) {
       case gr_utf8 : {
        const uint8 *pInit = static_cast<const uint8 *>(limit.pStart());
	    Utf8Consumer consumer(pInit);
        for (;limit.needMoreChars(consumer.pCharStart(), pProcessor->charsProcessed());) {
            const uint8 *pCur = consumer.pCharStart();
		    if (!consumer.consumeChar(limit, &cid, pErrHandler))
		        break;
		    if (!pProcessor->processChar(cid, pCur - pInit))
		        break;
        }
        break; }
       case gr_utf16: {
        const uint16* pInit = static_cast<const uint16 *>(limit.pStart());
        Utf16Consumer consumer(pInit);
        for (;limit.needMoreChars(consumer.pCharStart(), pProcessor->charsProcessed());) {
            const uint16 *pCur = consumer.pCharStart();
    		if (!consumer.consumeChar(limit, &cid, pErrHandler))
	    	    break;
		    if (!pProcessor->processChar(cid, pCur - pInit))
		        break;
            }
	    break;
        }
       case gr_utf32 : default: {
        const uint32 *pInit = static_cast<const uint32 *>(limit.pStart());
	    Utf32Consumer consumer(pInit);
        for (;limit.needMoreChars(consumer.pCharStart(), pProcessor->charsProcessed());) {
            const uint32 *pCur = consumer.pCharStart();
		    if (!consumer.consumeChar(limit, &cid, pErrHandler))
		        break;
		    if (!pProcessor->processChar(cid, pCur - pInit))
		        break;
            }
        break;
        }
    }
}

    class ToUtf8Processor
    {
    public:
        
        
        ToUtf8Processor(uint8 * buffer, size_t maxLength) :
            m_count(0), m_byteLength(0), m_maxLength(maxLength), m_buffer(buffer)
        {}
        bool processChar(uint32 cid, size_t )
        {
            
            if (cid <= 0x7F)
                m_buffer[m_byteLength++] = cid;
            else if (cid <= 0x07FF)
            {
                if (m_byteLength + 2 >= m_maxLength)
                    return false;
                m_buffer[m_byteLength++] = 0xC0 + (cid >> 6);
                m_buffer[m_byteLength++] = 0x80 + (cid & 0x3F);
            }
            else if (cid <= 0xFFFF)
            {
                if (m_byteLength + 3 >= m_maxLength)
                    return false;
                m_buffer[m_byteLength++] = 0xE0 + (cid >> 12);
                m_buffer[m_byteLength++] = 0x80 + ((cid & 0x0FC0) >> 6);
                m_buffer[m_byteLength++] = 0x80 +  (cid & 0x003F);
            }
            else if (cid <= 0x10FFFF)
            {
                if (m_byteLength + 4 >= m_maxLength)
                    return false;
                m_buffer[m_byteLength++] = 0xF0 + (cid >> 18);
                m_buffer[m_byteLength++] = 0x80 + ((cid & 0x3F000) >> 12);
                m_buffer[m_byteLength++] = 0x80 + ((cid & 0x00FC0) >> 6);
                m_buffer[m_byteLength++] = 0x80 +  (cid & 0x0003F);
            }
            else
            {
                
            }
            m_count++;
            if (m_byteLength >= m_maxLength)
                return false;
            return true;
        }
        size_t charsProcessed() const { return m_count; }
        size_t bytesProcessed() const { return m_byteLength; }
    private:
        size_t m_count;
        size_t m_byteLength;
        size_t m_maxLength;
        uint8 * m_buffer;
    };

    class ToUtf16Processor
    {
    public:
        
        
        ToUtf16Processor(uint16 * buffer, size_t maxLength) :
            m_count(0), m_uint16Length(0), m_maxLength(maxLength), m_buffer(buffer)
        {}
        bool processChar(uint32 cid, size_t )
        {
            
            if (cid <= 0xD800)
                m_buffer[m_uint16Length++] = cid;
            else if (cid < 0xE000)
            {
                
            }
            else if (cid >= 0xE000 && cid <= 0xFFFF)
                m_buffer[m_uint16Length++] = cid;
            else if (cid <= 0x10FFFF)
            {
                if (m_uint16Length + 2 >= m_maxLength)
                    return false;
                m_buffer[m_uint16Length++] = 0xD800 + ((cid & 0xFC00) >> 10) + ((cid >> 16) - 1);
                m_buffer[m_uint16Length++] = 0xDC00 + ((cid & 0x03FF) >> 12);
            }
            else
            {
                
            }
            m_count++;
            if (m_uint16Length == m_maxLength)
                return false;
            return true;
        }
        size_t charsProcessed() const { return m_count; }
        size_t uint16Processed() const { return m_uint16Length; }
    private:
        size_t m_count;
        size_t m_uint16Length;
        size_t m_maxLength;
        uint16 * m_buffer;
    };

    class ToUtf32Processor
    {
    public:
        ToUtf32Processor(uint32 * buffer, size_t maxLength) :
            m_count(0), m_maxLength(maxLength), m_buffer(buffer) {}
        bool processChar(uint32 cid, size_t )
        {
            m_buffer[m_count++] = cid;
            if (m_count == m_maxLength)
                return false;
            return true;
        }
        size_t charsProcessed() const { return m_count; }
    private:
        size_t m_count;
        size_t m_maxLength;
        uint32 * m_buffer;
    };

} 
