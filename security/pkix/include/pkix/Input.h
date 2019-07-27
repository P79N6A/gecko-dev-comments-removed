























#ifndef mozilla_pkix__Input_h
#define mozilla_pkix__Input_h

#include <cstring>

#include "pkix/nullptr.h"
#include "pkix/Result.h"
#include "prlog.h"
#include "stdint.h"

namespace mozilla { namespace pkix {

class Input;
















class InputBuffer
{
public:
  
  
  
  
  
  
  
  
  
  
  
  template <uint16_t N>
  explicit InputBuffer(const uint8_t (&data)[N])
    : data(data)
    , len(N)
  {
  }

  
  InputBuffer()
    : data(nullptr)
    , len(0u)
  {
  }

  
  
  Result Init(const uint8_t* data, size_t len)
  {
    if (this->data) {
      
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    if (!data || len > 0xffffu) {
      
      return Result::ERROR_BAD_DER;
    }

    this->data = data;
    this->len = len;

    return Success;
  }

  
  
  
  
  
  
  Result Init(InputBuffer other)
  {
    return Init(other.data, other.len);
  }

  
  
  
  
  uint16_t GetLength() const { return static_cast<uint16_t>(len); }

  
  
  const uint8_t* UnsafeGetData() const { return data; }

private:
  const uint8_t* data;
  size_t len;

  void operator=(const InputBuffer&) ; 
};

inline bool
InputBuffersAreEqual(const InputBuffer& a, const InputBuffer& b)
{
  return a.GetLength() == b.GetLength() &&
         !std::memcmp(a.UnsafeGetData(), b.UnsafeGetData(), a.GetLength());
}









class Input
{
public:
  Input()
    : input(nullptr)
    , end(nullptr)
  {
  }

  explicit Input(InputBuffer buffer)
    : input(buffer.UnsafeGetData())
    , end(buffer.UnsafeGetData() + buffer.GetLength())
  {
  }

  bool Peek(uint8_t expectedByte) const
  {
    return input < end && *input == expectedByte;
  }

  Result Read(uint8_t& out)
  {
    Result rv = EnsureLength(1);
    if (rv != Success) {
      return rv;
    }
    out = *input++;
    return Success;
  }

  Result Read(uint16_t& out)
  {
    Result rv = EnsureLength(2);
    if (rv != Success) {
      return rv;
    }
    out = *input++;
    out <<= 8u;
    out |= *input++;
    return Success;
  }

  template <uint16_t N>
  bool MatchRest(const uint8_t (&toMatch)[N])
  {
    
    
    
    if (static_cast<size_t>(end - input) != N) {
      return false;
    }
    if (memcmp(input, toMatch, N)) {
      return false;
    }
    input += N;
    return true;
  }

  template <uint16_t N>
  bool MatchTLV(uint8_t tag, uint16_t len, const uint8_t (&value)[N])
  {
    static_assert(N <= 127, "buffer larger than largest length supported");
    if (len > N) {
      PR_NOT_REACHED("overflow prevented dynamically instead of statically");
      return false;
    }
    uint16_t totalLen = 2u + len;
    if (EnsureLength(totalLen) != Success) {
      return false;
    }
    if (*input != tag) {
      return false;
    }
    if (*(input + 1) != len) {
      return false;
    }
    if (memcmp(input + 2, value, len)) {
      return false;
    }
    input += totalLen;
    return true;
  }

  Result Skip(uint16_t len)
  {
    Result rv = EnsureLength(len);
    if (rv != Success) {
      return rv;
    }
    input += len;
    return Success;
  }

  Result Skip(uint16_t len, Input& skippedInput)
  {
    Result rv = EnsureLength(len);
    if (rv != Success) {
      return rv;
    }
    rv = skippedInput.Init(input, len);
    if (rv != Success) {
      return rv;
    }
    input += len;
    return Success;
  }

  Result Skip(uint16_t len, InputBuffer& skippedItem)
  {
    Result rv = EnsureLength(len);
    if (rv != Success) {
      return rv;
    }
    rv = skippedItem.Init(input, len);
    if (rv != Success) {
      return rv;
    }
    input += len;
    return Success;
  }

  void SkipToEnd()
  {
    input = end;
  }

  Result EnsureLength(uint16_t len)
  {
    if (static_cast<size_t>(end - input) < len) {
      return Result::ERROR_BAD_DER;
    }
    return Success;
  }

  bool AtEnd() const { return input == end; }

  class Mark
  {
  private:
    friend class Input;
    Mark(const Input& input, const uint8_t* mark) : input(input), mark(mark) { }
    const Input& input;
    const uint8_t* const mark;
    void operator=(const Mark&) ;
  };

  Mark GetMark() const { return Mark(*this, input); }

  Result GetInputBuffer(const Mark& mark,  InputBuffer& item)
  {
    if (&mark.input != this || mark.mark > input) {
      PR_NOT_REACHED("invalid mark");
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    return item.Init(mark.mark, static_cast<uint16_t>(input - mark.mark));
  }

private:
  Result Init(const uint8_t* data, uint16_t len)
  {
    if (input) {
      
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    input = data;
    end = data + len;
    return Success;
  }

  const uint8_t* input;
  const uint8_t* end;

  Input(const Input&) ;
  void operator=(const Input&) ;
};

} } 

#endif 
