



#include "base/word_iterator.h"

#include "base/logging.h"
#include "unicode/ubrk.h"
#include "unicode/ustring.h"

const size_t npos = -1;

WordIterator::WordIterator(const std::wstring& str, BreakType break_type)
    : iter_(NULL),
      string_(str),
      break_type_(break_type),
      prev_(npos),
      pos_(0) {
}

WordIterator::~WordIterator() {
  if (iter_)
    ubrk_close(iter_);
}

bool WordIterator::Init() {
  UErrorCode status = U_ZERO_ERROR;
  UBreakIteratorType break_type;
  switch (break_type_) {
    case BREAK_WORD:
      break_type = UBRK_WORD;
      break;
    case BREAK_LINE:
      break_type = UBRK_LINE;
      break;
    default:
      NOTREACHED();
      break_type = UBRK_LINE;
  }
#if defined(WCHAR_T_IS_UTF16)
  iter_ = ubrk_open(break_type, NULL,
                    string_.data(), static_cast<int32_t>(string_.size()),
                    &status);
#else  
  
  
  
  
  
  
  chars_.resize(string_.length() * sizeof(UChar) + 1);

  UErrorCode error = U_ZERO_ERROR;
  int32_t destLength;
  u_strFromWCS(&chars_[0], chars_.size(), &destLength, string_.data(),
               string_.length(), &error);

  iter_ = ubrk_open(break_type, NULL, &chars_[0], destLength, &status);
#endif
  if (U_FAILURE(status)) {
    NOTREACHED() << "ubrk_open failed";
    return false;
  }
  ubrk_first(iter_);  
  return true;
}

bool WordIterator::Advance() {
  prev_ = pos_;
  const int32_t pos = ubrk_next(iter_);
  if (pos == UBRK_DONE) {
    pos_ = npos;
    return false;
  } else {
    pos_ = static_cast<size_t>(pos);
    return true;
  }
}

bool WordIterator::IsWord() const {
  return (ubrk_getRuleStatus(iter_) != UBRK_WORD_NONE);
}

std::wstring WordIterator::GetWord() const {
  DCHECK(prev_ != npos && pos_ != npos);
  return string_.substr(prev_, pos_ - prev_);
}
