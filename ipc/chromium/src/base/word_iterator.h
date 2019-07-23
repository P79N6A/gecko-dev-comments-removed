



#ifndef BASE_WORD_ITERATOR_H__
#define BASE_WORD_ITERATOR_H__

#include <string>
#include <vector>

#include "unicode/uchar.h"

#include "base/basictypes.h"

















class WordIterator {
 public:
  enum BreakType {
    BREAK_WORD,
    BREAK_LINE
  };

  
  WordIterator(const std::wstring& str, BreakType break_type);
  ~WordIterator();

  
  
  bool Init();

  
  
  size_t pos() const { return pos_; }
  
  size_t prev() const { return prev_; }

  
  
  
  
  bool Advance();

  
  
  
  bool IsWord() const;

  
  
  
  std::wstring GetWord() const;

 private:
  
  void* iter_;
#if !defined(WCHAR_T_IS_UTF16)
  std::vector<UChar> chars_;
#endif

  
  const std::wstring& string_;

  
  BreakType break_type_;

  
  size_t prev_, pos_;

  DISALLOW_EVIL_CONSTRUCTORS(WordIterator);
};

#endif
