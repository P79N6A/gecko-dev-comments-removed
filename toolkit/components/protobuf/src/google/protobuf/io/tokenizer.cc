

























































































#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/stringprintf.h>
#include <google/protobuf/io/strtod.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/stl_util.h>

namespace google {
namespace protobuf {
namespace io {
namespace {












#define CHARACTER_CLASS(NAME, EXPRESSION)      \
  class NAME {                                 \
   public:                                     \
    static inline bool InClass(char c) {       \
      return EXPRESSION;                       \
    }                                          \
  }

CHARACTER_CLASS(Whitespace, c == ' ' || c == '\n' || c == '\t' ||
                            c == '\r' || c == '\v' || c == '\f');
CHARACTER_CLASS(WhitespaceNoNewline, c == ' ' || c == '\t' ||
                                     c == '\r' || c == '\v' || c == '\f');

CHARACTER_CLASS(Unprintable, c < ' ' && c > '\0');

CHARACTER_CLASS(Digit, '0' <= c && c <= '9');
CHARACTER_CLASS(OctalDigit, '0' <= c && c <= '7');
CHARACTER_CLASS(HexDigit, ('0' <= c && c <= '9') ||
                          ('a' <= c && c <= 'f') ||
                          ('A' <= c && c <= 'F'));

CHARACTER_CLASS(Letter, ('a' <= c && c <= 'z') ||
                        ('A' <= c && c <= 'Z') ||
                        (c == '_'));

CHARACTER_CLASS(Alphanumeric, ('a' <= c && c <= 'z') ||
                              ('A' <= c && c <= 'Z') ||
                              ('0' <= c && c <= '9') ||
                              (c == '_'));

CHARACTER_CLASS(Escape, c == 'a' || c == 'b' || c == 'f' || c == 'n' ||
                        c == 'r' || c == 't' || c == 'v' || c == '\\' ||
                        c == '?' || c == '\'' || c == '\"');

#undef CHARACTER_CLASS



inline int DigitValue(char digit) {
  if ('0' <= digit && digit <= '9') return digit - '0';
  if ('a' <= digit && digit <= 'z') return digit - 'a' + 10;
  if ('A' <= digit && digit <= 'Z') return digit - 'A' + 10;
  return -1;
}


inline char TranslateEscape(char c) {
  switch (c) {
    case 'a':  return '\a';
    case 'b':  return '\b';
    case 'f':  return '\f';
    case 'n':  return '\n';
    case 'r':  return '\r';
    case 't':  return '\t';
    case 'v':  return '\v';
    case '\\': return '\\';
    case '?':  return '\?';    
    case '\'': return '\'';
    case '"':  return '\"';

    
    default:   return '?';
  }
}

}  

ErrorCollector::~ErrorCollector() {}



Tokenizer::Tokenizer(ZeroCopyInputStream* input,
                     ErrorCollector* error_collector)
  : input_(input),
    error_collector_(error_collector),
    buffer_(NULL),
    buffer_size_(0),
    buffer_pos_(0),
    read_error_(false),
    line_(0),
    column_(0),
    record_target_(NULL),
    record_start_(-1),
    allow_f_after_float_(false),
    comment_style_(CPP_COMMENT_STYLE),
    require_space_after_number_(true),
    allow_multiline_strings_(false) {

  current_.line = 0;
  current_.column = 0;
  current_.end_column = 0;
  current_.type = TYPE_START;

  Refresh();
}

Tokenizer::~Tokenizer() {
  
  
  if (buffer_size_ > buffer_pos_) {
    input_->BackUp(buffer_size_ - buffer_pos_);
  }
}




void Tokenizer::NextChar() {
  
  
  if (current_char_ == '\n') {
    ++line_;
    column_ = 0;
  } else if (current_char_ == '\t') {
    column_ += kTabWidth - column_ % kTabWidth;
  } else {
    ++column_;
  }

  
  ++buffer_pos_;
  if (buffer_pos_ < buffer_size_) {
    current_char_ = buffer_[buffer_pos_];
  } else {
    Refresh();
  }
}

void Tokenizer::Refresh() {
  if (read_error_) {
    current_char_ = '\0';
    return;
  }

  
  if (record_target_ != NULL && record_start_ < buffer_size_) {
    record_target_->append(buffer_ + record_start_, buffer_size_ - record_start_);
    record_start_ = 0;
  }

  const void* data = NULL;
  buffer_ = NULL;
  buffer_pos_ = 0;
  do {
    if (!input_->Next(&data, &buffer_size_)) {
      
      buffer_size_ = 0;
      read_error_ = true;
      current_char_ = '\0';
      return;
    }
  } while (buffer_size_ == 0);

  buffer_ = static_cast<const char*>(data);

  current_char_ = buffer_[0];
}

inline void Tokenizer::RecordTo(string* target) {
  record_target_ = target;
  record_start_ = buffer_pos_;
}

inline void Tokenizer::StopRecording() {
  
  
  
  
  if (buffer_pos_ != record_start_) {
    record_target_->append(buffer_ + record_start_, buffer_pos_ - record_start_);
  }
  record_target_ = NULL;
  record_start_ = -1;
}

inline void Tokenizer::StartToken() {
  current_.type = TYPE_START;    
  current_.text.clear();
  current_.line = line_;
  current_.column = column_;
  RecordTo(&current_.text);
}

inline void Tokenizer::EndToken() {
  StopRecording();
  current_.end_column = column_;
}




template<typename CharacterClass>
inline bool Tokenizer::LookingAt() {
  return CharacterClass::InClass(current_char_);
}

template<typename CharacterClass>
inline bool Tokenizer::TryConsumeOne() {
  if (CharacterClass::InClass(current_char_)) {
    NextChar();
    return true;
  } else {
    return false;
  }
}

inline bool Tokenizer::TryConsume(char c) {
  if (current_char_ == c) {
    NextChar();
    return true;
  } else {
    return false;
  }
}

template<typename CharacterClass>
inline void Tokenizer::ConsumeZeroOrMore() {
  while (CharacterClass::InClass(current_char_)) {
    NextChar();
  }
}

template<typename CharacterClass>
inline void Tokenizer::ConsumeOneOrMore(const char* error) {
  if (!CharacterClass::InClass(current_char_)) {
    AddError(error);
  } else {
    do {
      NextChar();
    } while (CharacterClass::InClass(current_char_));
  }
}





void Tokenizer::ConsumeString(char delimiter) {
  while (true) {
    switch (current_char_) {
      case '\0':
        AddError("Unexpected end of string.");
        return;

      case '\n': {
        if (!allow_multiline_strings_) {
          AddError("String literals cannot cross line boundaries.");
          return;
        }
        NextChar();
        break;
      }

      case '\\': {
        
        NextChar();
        if (TryConsumeOne<Escape>()) {
          
        } else if (TryConsumeOne<OctalDigit>()) {
          
          
          
        } else if (TryConsume('x') || TryConsume('X')) {
          if (!TryConsumeOne<HexDigit>()) {
            AddError("Expected hex digits for escape sequence.");
          }
          
        } else if (TryConsume('u')) {
          if (!TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>()) {
            AddError("Expected four hex digits for \\u escape sequence.");
          }
        } else if (TryConsume('U')) {
          
          
          if (!TryConsume('0') ||
              !TryConsume('0') ||
              !(TryConsume('0') || TryConsume('1')) ||
              !TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>() ||
              !TryConsumeOne<HexDigit>()) {
            AddError("Expected eight hex digits up to 10ffff for \\U escape "
                     "sequence");
          }
        } else {
          AddError("Invalid escape sequence in string literal.");
        }
        break;
      }

      default: {
        if (current_char_ == delimiter) {
          NextChar();
          return;
        }
        NextChar();
        break;
      }
    }
  }
}

Tokenizer::TokenType Tokenizer::ConsumeNumber(bool started_with_zero,
                                              bool started_with_dot) {
  bool is_float = false;

  if (started_with_zero && (TryConsume('x') || TryConsume('X'))) {
    
    ConsumeOneOrMore<HexDigit>("\"0x\" must be followed by hex digits.");

  } else if (started_with_zero && LookingAt<Digit>()) {
    
    ConsumeZeroOrMore<OctalDigit>();
    if (LookingAt<Digit>()) {
      AddError("Numbers starting with leading zero must be in octal.");
      ConsumeZeroOrMore<Digit>();
    }

  } else {
    
    if (started_with_dot) {
      is_float = true;
      ConsumeZeroOrMore<Digit>();
    } else {
      ConsumeZeroOrMore<Digit>();

      if (TryConsume('.')) {
        is_float = true;
        ConsumeZeroOrMore<Digit>();
      }
    }

    if (TryConsume('e') || TryConsume('E')) {
      is_float = true;
      TryConsume('-') || TryConsume('+');
      ConsumeOneOrMore<Digit>("\"e\" must be followed by exponent.");
    }

    if (allow_f_after_float_ && (TryConsume('f') || TryConsume('F'))) {
      is_float = true;
    }
  }

  if (LookingAt<Letter>() && require_space_after_number_) {
    AddError("Need space between number and identifier.");
  } else if (current_char_ == '.') {
    if (is_float) {
      AddError(
        "Already saw decimal point or exponent; can't have another one.");
    } else {
      AddError("Hex and octal numbers must be integers.");
    }
  }

  return is_float ? TYPE_FLOAT : TYPE_INTEGER;
}

void Tokenizer::ConsumeLineComment(string* content) {
  if (content != NULL) RecordTo(content);

  while (current_char_ != '\0' && current_char_ != '\n') {
    NextChar();
  }
  TryConsume('\n');

  if (content != NULL) StopRecording();
}

void Tokenizer::ConsumeBlockComment(string* content) {
  int start_line = line_;
  int start_column = column_ - 2;

  if (content != NULL) RecordTo(content);

  while (true) {
    while (current_char_ != '\0' &&
           current_char_ != '*' &&
           current_char_ != '/' &&
           current_char_ != '\n') {
      NextChar();
    }

    if (TryConsume('\n')) {
      if (content != NULL) StopRecording();

      
      ConsumeZeroOrMore<WhitespaceNoNewline>();
      if (TryConsume('*')) {
        if (TryConsume('/')) {
          
          break;
        }
      }

      if (content != NULL) RecordTo(content);
    } else if (TryConsume('*') && TryConsume('/')) {
      
      if (content != NULL) {
        StopRecording();
        
        content->erase(content->size() - 2);
      }
      break;
    } else if (TryConsume('/') && current_char_ == '*') {
      
      
      AddError(
        "\"/*\" inside block comment.  Block comments cannot be nested.");
    } else if (current_char_ == '\0') {
      AddError("End-of-file inside block comment.");
      error_collector_->AddError(
        start_line, start_column, "  Comment started here.");
      if (content != NULL) StopRecording();
      break;
    }
  }
}

Tokenizer::NextCommentStatus Tokenizer::TryConsumeCommentStart() {
  if (comment_style_ == CPP_COMMENT_STYLE && TryConsume('/')) {
    if (TryConsume('/')) {
      return LINE_COMMENT;
    } else if (TryConsume('*')) {
      return BLOCK_COMMENT;
    } else {
      
      current_.type = TYPE_SYMBOL;
      current_.text = "/";
      current_.line = line_;
      current_.column = column_ - 1;
      current_.end_column = column_;
      return SLASH_NOT_COMMENT;
    }
  } else if (comment_style_ == SH_COMMENT_STYLE && TryConsume('#')) {
    return LINE_COMMENT;
  } else {
    return NO_COMMENT;
  }
}



bool Tokenizer::Next() {
  previous_ = current_;

  while (!read_error_) {
    ConsumeZeroOrMore<Whitespace>();

    switch (TryConsumeCommentStart()) {
      case LINE_COMMENT:
        ConsumeLineComment(NULL);
        continue;
      case BLOCK_COMMENT:
        ConsumeBlockComment(NULL);
        continue;
      case SLASH_NOT_COMMENT:
        return true;
      case NO_COMMENT:
        break;
    }

    
    if (read_error_) break;

    if (LookingAt<Unprintable>() || current_char_ == '\0') {
      AddError("Invalid control characters encountered in text.");
      NextChar();
      
      
      
      
      
      while (TryConsumeOne<Unprintable>() ||
             (!read_error_ && TryConsume('\0'))) {
        
      }

    } else {
      
      StartToken();

      if (TryConsumeOne<Letter>()) {
        ConsumeZeroOrMore<Alphanumeric>();
        current_.type = TYPE_IDENTIFIER;
      } else if (TryConsume('0')) {
        current_.type = ConsumeNumber(true, false);
      } else if (TryConsume('.')) {
        
        

        if (TryConsumeOne<Digit>()) {
          
          if (previous_.type == TYPE_IDENTIFIER &&
              current_.line == previous_.line &&
              current_.column == previous_.end_column) {
            
            error_collector_->AddError(line_, column_ - 2,
              "Need space between identifier and decimal point.");
          }
          current_.type = ConsumeNumber(false, true);
        } else {
          current_.type = TYPE_SYMBOL;
        }
      } else if (TryConsumeOne<Digit>()) {
        current_.type = ConsumeNumber(false, false);
      } else if (TryConsume('\"')) {
        ConsumeString('\"');
        current_.type = TYPE_STRING;
      } else if (TryConsume('\'')) {
        ConsumeString('\'');
        current_.type = TYPE_STRING;
      } else {
        
        if (current_char_ & 0x80) {
          error_collector_->AddError(line_, column_,
              StringPrintf("Interpreting non ascii codepoint %d.",
                           static_cast<unsigned char>(current_char_)));
        }
        NextChar();
        current_.type = TYPE_SYMBOL;
      }

      EndToken();
      return true;
    }
  }

  
  current_.type = TYPE_END;
  current_.text.clear();
  current_.line = line_;
  current_.column = column_;
  current_.end_column = column_;
  return false;
}

namespace {








class CommentCollector {
 public:
  CommentCollector(string* prev_trailing_comments,
                   vector<string>* detached_comments,
                   string* next_leading_comments)
      : prev_trailing_comments_(prev_trailing_comments),
        detached_comments_(detached_comments),
        next_leading_comments_(next_leading_comments),
        has_comment_(false),
        is_line_comment_(false),
        can_attach_to_prev_(true) {
    if (prev_trailing_comments != NULL) prev_trailing_comments->clear();
    if (detached_comments != NULL) detached_comments->clear();
    if (next_leading_comments != NULL) next_leading_comments->clear();
  }

  ~CommentCollector() {
    
    if (next_leading_comments_ != NULL && has_comment_) {
      comment_buffer_.swap(*next_leading_comments_);
    }
  }

  
  
  string* GetBufferForLineComment() {
    
    if (has_comment_ && !is_line_comment_) {
      Flush();
    }
    has_comment_ = true;
    is_line_comment_ = true;
    return &comment_buffer_;
  }

  
  
  string* GetBufferForBlockComment() {
    if (has_comment_) {
      Flush();
    }
    has_comment_ = true;
    is_line_comment_ = false;
    return &comment_buffer_;
  }

  void ClearBuffer() {
    comment_buffer_.clear();
    has_comment_ = false;
  }

  
  
  void Flush() {
    if (has_comment_) {
      if (can_attach_to_prev_) {
        if (prev_trailing_comments_ != NULL) {
          prev_trailing_comments_->append(comment_buffer_);
        }
        can_attach_to_prev_ = false;
      } else {
        if (detached_comments_ != NULL) {
          detached_comments_->push_back(comment_buffer_);
        }
      }
      ClearBuffer();
    }
  }

  void DetachFromPrev() {
    can_attach_to_prev_ = false;
  }

 private:
  string* prev_trailing_comments_;
  vector<string>* detached_comments_;
  string* next_leading_comments_;

  string comment_buffer_;

  
  
  bool has_comment_;

  
  bool is_line_comment_;

  
  
  bool can_attach_to_prev_;
};

} 

bool Tokenizer::NextWithComments(string* prev_trailing_comments,
                                 vector<string>* detached_comments,
                                 string* next_leading_comments) {
  CommentCollector collector(prev_trailing_comments, detached_comments,
                             next_leading_comments);

  if (current_.type == TYPE_START) {
    collector.DetachFromPrev();
  } else {
    
    
    ConsumeZeroOrMore<WhitespaceNoNewline>();
    switch (TryConsumeCommentStart()) {
      case LINE_COMMENT:
        ConsumeLineComment(collector.GetBufferForLineComment());

        
        
        collector.Flush();
        break;
      case BLOCK_COMMENT:
        ConsumeBlockComment(collector.GetBufferForBlockComment());

        ConsumeZeroOrMore<WhitespaceNoNewline>();
        if (!TryConsume('\n')) {
          
          
          collector.ClearBuffer();
          return Next();
        }

        
        
        collector.Flush();
        break;
      case SLASH_NOT_COMMENT:
        return true;
      case NO_COMMENT:
        if (!TryConsume('\n')) {
          
          return Next();
        }
        break;
    }
  }

  
  while (true) {
    ConsumeZeroOrMore<WhitespaceNoNewline>();

    switch (TryConsumeCommentStart()) {
      case LINE_COMMENT:
        ConsumeLineComment(collector.GetBufferForLineComment());
        break;
      case BLOCK_COMMENT:
        ConsumeBlockComment(collector.GetBufferForBlockComment());

        
        
        ConsumeZeroOrMore<WhitespaceNoNewline>();
        TryConsume('\n');
        break;
      case SLASH_NOT_COMMENT:
        return true;
      case NO_COMMENT:
        if (TryConsume('\n')) {
          
          collector.Flush();
          collector.DetachFromPrev();
        } else {
          bool result = Next();
          if (!result ||
              current_.text == "}" ||
              current_.text == "]" ||
              current_.text == ")") {
            
            
            collector.Flush();
          }
          return result;
        }
        break;
    }
  }
}








bool Tokenizer::ParseInteger(const string& text, uint64 max_value,
                             uint64* output) {
  
  



  const char* ptr = text.c_str();
  int base = 10;
  if (ptr[0] == '0') {
    if (ptr[1] == 'x' || ptr[1] == 'X') {
      
      base = 16;
      ptr += 2;
    } else {
      
      base = 8;
    }
  }

  uint64 result = 0;
  for (; *ptr != '\0'; ptr++) {
    int digit = DigitValue(*ptr);
    GOOGLE_LOG_IF(DFATAL, digit < 0 || digit >= base)
      << " Tokenizer::ParseInteger() passed text that could not have been"
         " tokenized as an integer: " << CEscape(text);
    if (digit > max_value || result > (max_value - digit) / base) {
      
      return false;
    }
    result = result * base + digit;
  }

  *output = result;
  return true;
}

double Tokenizer::ParseFloat(const string& text) {
  const char* start = text.c_str();
  char* end;
  double result = NoLocaleStrtod(start, &end);

  
  
  
  if (*end == 'e' || *end == 'E') {
    ++end;
    if (*end == '-' || *end == '+') ++end;
  }

  
  
  if (*end == 'f' || *end == 'F') {
    ++end;
  }

  GOOGLE_LOG_IF(DFATAL, end - start != text.size() || *start == '-')
    << " Tokenizer::ParseFloat() passed text that could not have been"
       " tokenized as a float: " << CEscape(text);
  return result;
}



static void AppendUTF8(uint32 code_point, string* output) {
  uint32 tmp = 0;
  int len = 0;
  if (code_point <= 0x7f) {
    tmp = code_point;
    len = 1;
  } else if (code_point <= 0x07ff) {
    tmp = 0x0000c080 |
        ((code_point & 0x07c0) << 2) |
        (code_point & 0x003f);
    len = 2;
  } else if (code_point <= 0xffff) {
    tmp = 0x00e08080 |
        ((code_point & 0xf000) << 4) |
        ((code_point & 0x0fc0) << 2) |
        (code_point & 0x003f);
    len = 3;
  } else if (code_point <= 0x1fffff) {
    tmp = 0xf0808080 |
        ((code_point & 0x1c0000) << 6) |
        ((code_point & 0x03f000) << 4) |
        ((code_point & 0x000fc0) << 2) |
        (code_point & 0x003f);
    len = 4;
  } else {
    
    
    StringAppendF(output, "\\U%08x", code_point);
    return;
  }
  tmp = ghtonl(tmp);
  output->append(reinterpret_cast<const char*>(&tmp) + sizeof(tmp) - len, len);
}



static bool ReadHexDigits(const char* ptr, int len, uint32* result) {
  *result = 0;
  if (len == 0) return false;
  for (const char* end = ptr + len; ptr < end; ++ptr) {
    if (*ptr == '\0') return false;
    *result = (*result << 4) + DigitValue(*ptr);
  }
  return true;
}






static const uint32 kMinHeadSurrogate = 0xd800;
static const uint32 kMaxHeadSurrogate = 0xdc00;
static const uint32 kMinTrailSurrogate = 0xdc00;
static const uint32 kMaxTrailSurrogate = 0xe000;

static inline bool IsHeadSurrogate(uint32 code_point) {
  return (code_point >= kMinHeadSurrogate) && (code_point < kMaxHeadSurrogate);
}

static inline bool IsTrailSurrogate(uint32 code_point) {
  return (code_point >= kMinTrailSurrogate) &&
      (code_point < kMaxTrailSurrogate);
}


static uint32 AssembleUTF16(uint32 head_surrogate, uint32 trail_surrogate) {
  GOOGLE_DCHECK(IsHeadSurrogate(head_surrogate));
  GOOGLE_DCHECK(IsTrailSurrogate(trail_surrogate));
  return 0x10000 + (((head_surrogate - kMinHeadSurrogate) << 10) |
      (trail_surrogate - kMinTrailSurrogate));
}


static inline int UnicodeLength(char key) {
  if (key == 'u') return 4;
  if (key == 'U') return 8;
  return 0;
}





static const char* FetchUnicodePoint(const char* ptr, uint32* code_point) {
  const char* p = ptr;
  
  const int len = UnicodeLength(*p++);
  if (!ReadHexDigits(p, len, code_point))
    return ptr;
  p += len;

  
  
  
  
  if (IsHeadSurrogate(*code_point) && *p == '\\' && *(p + 1) == 'u') {
    uint32 trail_surrogate;
    if (ReadHexDigits(p + 2, 4, &trail_surrogate) &&
        IsTrailSurrogate(trail_surrogate)) {
      *code_point = AssembleUTF16(*code_point, trail_surrogate);
      p += 6;
    }
    
    
  }

  return p;
}



void Tokenizer::ParseStringAppend(const string& text, string* output) {
  
  
  const size_t text_size = text.size();
  if (text_size == 0) {
    GOOGLE_LOG(DFATAL)
      << " Tokenizer::ParseStringAppend() passed text that could not"
         " have been tokenized as a string: " << CEscape(text);
    return;
  }

  
  
  
  const size_t new_len = text_size + output->size();
  if (new_len > output->capacity()) {
    output->reserve(new_len);
  }

  
  
  
  
  for (const char* ptr = text.c_str() + 1; *ptr != '\0'; ptr++) {
    if (*ptr == '\\' && ptr[1] != '\0') {
      
      ++ptr;

      if (OctalDigit::InClass(*ptr)) {
        
        int code = DigitValue(*ptr);
        if (OctalDigit::InClass(ptr[1])) {
          ++ptr;
          code = code * 8 + DigitValue(*ptr);
        }
        if (OctalDigit::InClass(ptr[1])) {
          ++ptr;
          code = code * 8 + DigitValue(*ptr);
        }
        output->push_back(static_cast<char>(code));

      } else if (*ptr == 'x') {
        
        
        int code = 0;
        if (HexDigit::InClass(ptr[1])) {
          ++ptr;
          code = DigitValue(*ptr);
        }
        if (HexDigit::InClass(ptr[1])) {
          ++ptr;
          code = code * 16 + DigitValue(*ptr);
        }
        output->push_back(static_cast<char>(code));

      } else if (*ptr == 'u' || *ptr == 'U') {
        uint32 unicode;
        const char* end = FetchUnicodePoint(ptr, &unicode);
        if (end == ptr) {
          
          output->push_back(*ptr);
        } else {
          AppendUTF8(unicode, output);
          ptr = end - 1;  
        }
      } else {
        
        output->push_back(TranslateEscape(*ptr));
      }

    } else if (*ptr == text[0] && ptr[1] == '\0') {
      
    } else {
      output->push_back(*ptr);
    }
  }
}

template<typename CharacterClass>
static bool AllInClass(const string& s) {
  for (int i = 0; i < s.size(); ++i) {
    if (!CharacterClass::InClass(s[i]))
      return false;
  }
  return true;
}

bool Tokenizer::IsIdentifier(const string& text) {
  
  if (text.size() == 0)
    return false;
  if (!Letter::InClass(text.at(0)))
    return false;
  if (!AllInClass<Alphanumeric>(text.substr(1)))
    return false;
  return true;
}

}  
}  
}  
