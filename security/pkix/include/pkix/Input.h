























#ifndef mozilla_pkix_Input_h
#define mozilla_pkix_Input_h

#include <cstring>

#include "pkix/Result.h"
#include "stdint.h"

namespace mozilla { namespace pkix {

class Reader;
















class Input final
{
public:
  typedef uint16_t size_type;

  
  
  
  
  
  
  
  
  
  
  
  template <size_type N>
  explicit Input(const uint8_t (&data)[N])
    : data(data)
    , len(N)
  {
  }

  
  Input()
    : data(nullptr)
    , len(0u)
  {
  }

  
  Input(const Input&) = default;

  
  
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

  
  
  
  
  
  
  Result Init(Input other)
  {
    return Init(other.data, other.len);
  }

  
  
  
  
  size_type GetLength() const { return static_cast<size_type>(len); }

  
  
  const uint8_t* UnsafeGetData() const { return data; }

private:
  const uint8_t* data;
  size_t len;

  void operator=(const Input&) = delete; 
};

inline bool
InputsAreEqual(const Input& a, const Input& b)
{
  return a.GetLength() == b.GetLength() &&
         !std::memcmp(a.UnsafeGetData(), b.UnsafeGetData(), a.GetLength());
}









class Reader final
{
public:
  Reader()
    : input(nullptr)
    , end(nullptr)
  {
  }

  explicit Reader(Input input)
    : input(input.UnsafeGetData())
    , end(input.UnsafeGetData() + input.GetLength())
  {
  }

  Result Init(Input input)
  {
    if (this->input) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    this->input = input.UnsafeGetData();
    this->end = input.UnsafeGetData() + input.GetLength();
    return Success;
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

  template <Input::size_type N>
  bool MatchRest(const uint8_t (&toMatch)[N])
  {
    
    
    
    if (static_cast<size_t>(end - input) != N) {
      return false;
    }
    if (memcmp(input, toMatch, N)) {
      return false;
    }
    input = end;
    return true;
  }

  bool MatchRest(Input toMatch)
  {
    
    
    
    size_t remaining = static_cast<size_t>(end - input);
    if (toMatch.GetLength() != remaining) {
      return false;
    }
    if (std::memcmp(input, toMatch.UnsafeGetData(), remaining)) {
      return false;
    }
    input = end;
    return true;
  }

  Result Skip(Input::size_type len)
  {
    Result rv = EnsureLength(len);
    if (rv != Success) {
      return rv;
    }
    input += len;
    return Success;
  }

  Result Skip(Input::size_type len, Reader& skipped)
  {
    Result rv = EnsureLength(len);
    if (rv != Success) {
      return rv;
    }
    rv = skipped.Init(input, len);
    if (rv != Success) {
      return rv;
    }
    input += len;
    return Success;
  }

  Result Skip(Input::size_type len,  Input& skipped)
  {
    Result rv = EnsureLength(len);
    if (rv != Success) {
      return rv;
    }
    rv = skipped.Init(input, len);
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

  Result SkipToEnd( Input& skipped)
  {
    return Skip(static_cast<Input::size_type>(end - input), skipped);
  }

  Result EnsureLength(Input::size_type len)
  {
    if (static_cast<size_t>(end - input) < len) {
      return Result::ERROR_BAD_DER;
    }
    return Success;
  }

  bool AtEnd() const { return input == end; }

  class Mark final
  {
  public:
    Mark(const Mark&) = default; 
  private:
    friend class Reader;
    Mark(const Reader& input, const uint8_t* mark) : input(input), mark(mark) { }
    const Reader& input;
    const uint8_t* const mark;
    void operator=(const Mark&) = delete;
  };

  Mark GetMark() const { return Mark(*this, input); }

  Result GetInput(const Mark& mark,  Input& item)
  {
    if (&mark.input != this || mark.mark > input) {
      return NotReached("invalid mark", Result::FATAL_ERROR_INVALID_ARGS);
    }
    return item.Init(mark.mark,
                     static_cast<Input::size_type>(input - mark.mark));
  }

private:
  Result Init(const uint8_t* data, Input::size_type len)
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

  Reader(const Reader&) = delete;
  void operator=(const Reader&) = delete;
};

inline bool
InputContains(const Input& input, uint8_t toFind)
{
  Reader reader(input);
  for (;;) {
    uint8_t b;
    if (reader.Read(b) != Success) {
      return false;
    }
    if (b == toFind) {
      return true;
    }
  }
}

} } 

#endif 
