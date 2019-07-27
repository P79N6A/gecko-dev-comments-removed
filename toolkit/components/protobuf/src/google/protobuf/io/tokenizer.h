



































#ifndef GOOGLE_PROTOBUF_IO_TOKENIZER_H__
#define GOOGLE_PROTOBUF_IO_TOKENIZER_H__

#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
namespace io {

class ZeroCopyInputStream;     


class ErrorCollector;
class Tokenizer;




class LIBPROTOBUF_EXPORT ErrorCollector {
 public:
  inline ErrorCollector() {}
  virtual ~ErrorCollector();

  
  
  
  virtual void AddError(int line, int column, const string& message) = 0;

  
  
  
  virtual void AddWarning(int , int ,
                          const string& ) { }

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ErrorCollector);
};







class LIBPROTOBUF_EXPORT Tokenizer {
 public:
  
  
  
  Tokenizer(ZeroCopyInputStream* input, ErrorCollector* error_collector);
  ~Tokenizer();

  enum TokenType {
    TYPE_START,       
    TYPE_END,         

    TYPE_IDENTIFIER,  
                      
                      
                      
    TYPE_INTEGER,     
                      
                      
                      
                      
                      
    TYPE_FLOAT,       
                      
                      
    TYPE_STRING,      
                      
                      
    TYPE_SYMBOL,      
                      
                      
  };

  
  struct Token {
    TokenType type;
    string text;       
                       
                       

    
    
    int line;
    int column;
    int end_column;
  };

  
  
  const Token& current();

  
  
  const Token& previous();

  
  
  bool Next();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool NextWithComments(string* prev_trailing_comments,
                        vector<string>* detached_comments,
                        string* next_leading_comments);

  

  
  
  
  static double ParseFloat(const string& text);

  
  
  
  static void ParseString(const string& text, string* output);

  
  static void ParseStringAppend(const string& text, string* output);

  
  
  
  
  
  static bool ParseInteger(const string& text, uint64 max_value,
                           uint64* output);

  

  
  
  
  
  void set_allow_f_after_float(bool value) { allow_f_after_float_ = value; }

  
  enum CommentStyle {
    
    
    CPP_COMMENT_STYLE,
    
    SH_COMMENT_STYLE
  };

  
  void set_comment_style(CommentStyle style) { comment_style_ = style; }

  
  
  void set_require_space_after_number(bool require) {
    require_space_after_number_ = require;
  }

  
  
  void set_allow_multiline_strings(bool allow) {
    allow_multiline_strings_ = allow;
  }

  
  static bool IsIdentifier(const string& text);

  
 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(Tokenizer);

  Token current_;           
  Token previous_;          

  ZeroCopyInputStream* input_;
  ErrorCollector* error_collector_;

  char current_char_;       
  const char* buffer_;      
  int buffer_size_;         
  int buffer_pos_;          
  bool read_error_;         

  
  int line_;
  int column_;

  
  
  
  
  string* record_target_;
  int record_start_;

  
  bool allow_f_after_float_;
  CommentStyle comment_style_;
  bool require_space_after_number_;
  bool allow_multiline_strings_;

  
  
  static const int kTabWidth = 8;

  
  

  
  void NextChar();

  
  void Refresh();

  inline void RecordTo(string* target);
  inline void StopRecording();

  
  
  inline void StartToken();
  
  
  
  inline void EndToken();

  
  void AddError(const string& message) {
    error_collector_->AddError(line_, column_, message);
  }

  
  
  
  
  

  
  
  void ConsumeString(char delimiter);

  
  
  
  
  
  
  TokenType ConsumeNumber(bool started_with_zero, bool started_with_dot);

  
  void ConsumeLineComment(string* content);
  
  void ConsumeBlockComment(string* content);

  enum NextCommentStatus {
    
    LINE_COMMENT,

    
    BLOCK_COMMENT,

    
    
    SLASH_NOT_COMMENT,

    
    NO_COMMENT
  };

  
  
  NextCommentStatus TryConsumeCommentStart();

  
  
  
  
  
  
  

  
  
  template<typename CharacterClass>
  inline bool LookingAt();

  
  
  
  template<typename CharacterClass>
  inline bool TryConsumeOne();

  
  inline bool TryConsume(char c);

  
  template<typename CharacterClass>
  inline void ConsumeZeroOrMore();

  
  
  
  template<typename CharacterClass>
  inline void ConsumeOneOrMore(const char* error);
};


inline const Tokenizer::Token& Tokenizer::current() {
  return current_;
}

inline const Tokenizer::Token& Tokenizer::previous() {
  return previous_;
}

inline void Tokenizer::ParseString(const string& text, string* output) {
  output->clear();
  ParseStringAppend(text, output);
}

}  
}  

}  
#endif  
