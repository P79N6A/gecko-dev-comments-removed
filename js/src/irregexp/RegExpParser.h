





























#ifndef V8_PARSER_H_
#define V8_PARSER_H_

#include "irregexp/RegExpAST.h"

namespace js {

namespace frontend {
    class TokenStream;
}

namespace irregexp {

bool
ParsePattern(frontend::TokenStream& ts, LifoAlloc& alloc, JSAtom* str,
             bool multiline, bool match_only,
             RegExpCompileData* data);

bool
ParsePatternSyntax(frontend::TokenStream& ts, LifoAlloc& alloc, JSAtom* str);







template <typename T, int initial_size>
class BufferedVector
{
  public:
    typedef Vector<T*, 1, LifoAllocPolicy<Infallible> > VectorType;

    BufferedVector() : list_(nullptr), last_(nullptr) {}

    
    
    
    void Add(LifoAlloc* alloc, T* value) {
        if (last_ != nullptr) {
            if (list_ == nullptr) {
                list_ = alloc->newInfallible<VectorType>(*alloc);
                list_->reserve(initial_size);
            }
            list_->append(last_);
        }
        last_ = value;
    }

    T* last() {
        MOZ_ASSERT(last_ != nullptr);
        return last_;
    }

    T* RemoveLast() {
        MOZ_ASSERT(last_ != nullptr);
        T* result = last_;
        if ((list_ != nullptr) && (list_->length() > 0))
            last_ = list_->popCopy();
        else
            last_ = nullptr;
        return result;
    }

    T* Get(int i) {
        MOZ_ASSERT((0 <= i) && (i < length()));
        if (list_ == nullptr) {
            MOZ_ASSERT(0 == i);
            return last_;
        } else {
            if (size_t(i) == list_->length()) {
                MOZ_ASSERT(last_ != nullptr);
                return last_;
            } else {
                return (*list_)[i];
            }
        }
    }

    void Clear() {
        list_ = nullptr;
        last_ = nullptr;
    }

    int length() {
        int length = (list_ == nullptr) ? 0 : list_->length();
        return length + ((last_ == nullptr) ? 0 : 1);
    }

    VectorType* GetList(LifoAlloc* alloc) {
        if (list_ == nullptr)
            list_ = alloc->newInfallible<VectorType>(*alloc);
        if (last_ != nullptr) {
            list_->append(last_);
            last_ = nullptr;
        }
        return list_;
    }

  private:
    VectorType* list_;
    T* last_;
};



class RegExpBuilder
{
  public:
    explicit RegExpBuilder(LifoAlloc* alloc);
    void AddCharacter(char16_t character);
    
    
    void AddEmpty();
    void AddAtom(RegExpTree* tree);
    void AddAssertion(RegExpTree* tree);
    void NewAlternative();  
    void AddQuantifierToAtom(int min, int max, RegExpQuantifier::QuantifierType type);
    RegExpTree* ToRegExp();

  private:
    void FlushCharacters();
    void FlushText();
    void FlushTerms();

    LifoAlloc* alloc;
    bool pending_empty_;
    CharacterVector* characters_;
    BufferedVector<RegExpTree, 2> terms_;
    BufferedVector<RegExpTree, 2> text_;
    BufferedVector<RegExpTree, 2> alternatives_;

    enum LastAdded {
        ADD_NONE, ADD_CHAR, ADD_TERM, ADD_ASSERT, ADD_ATOM
    };
    mozilla::DebugOnly<LastAdded> last_added_;
};


typedef uint32_t widechar;

template <typename CharT>
class RegExpParser
{
  public:
    RegExpParser(frontend::TokenStream& ts, LifoAlloc* alloc,
                 const CharT* chars, const CharT* end, bool multiline_mode);

    RegExpTree* ParsePattern();
    RegExpTree* ParseDisjunction();
    RegExpTree* ParseGroup();
    RegExpTree* ParseCharacterClass();

    
    
    bool ParseIntervalQuantifier(int* min_out, int* max_out);

    
    
    widechar ParseClassCharacterEscape();

    
    
    bool ParseHexEscape(int length, size_t* value);

    size_t ParseOctalLiteral();

    
    
    
    
    bool ParseBackReferenceIndex(int* index_out);

    bool ParseClassAtom(char16_t* char_class, CharacterRange* char_range);
    RegExpTree* ReportError(unsigned errorNumber);
    void Advance();
    void Advance(int dist) {
        next_pos_ += dist - 1;
        Advance();
    }

    void Reset(const CharT* pos) {
        next_pos_ = pos;
        has_more_ = (pos < end_);
        Advance();
    }

    
    
    bool simple() { return simple_; }
    bool contains_anchor() { return contains_anchor_; }
    void set_contains_anchor() { contains_anchor_ = true; }
    int captures_started() { return captures_ == nullptr ? 0 : captures_->length(); }
    const CharT* position() { return next_pos_ - 1; }

    static const int kMaxCaptures = 1 << 16;
    static const widechar kEndMarker = (1 << 21);

  private:
    enum SubexpressionType {
        INITIAL,
        CAPTURE,  
        POSITIVE_LOOKAHEAD,
        NEGATIVE_LOOKAHEAD,
        GROUPING
    };

    class RegExpParserState {
      public:
        RegExpParserState(LifoAlloc* alloc,
                          RegExpParserState* previous_state,
                          SubexpressionType group_type,
                          int disjunction_capture_index)
            : previous_state_(previous_state),
              builder_(alloc->newInfallible<RegExpBuilder>(alloc)),
              group_type_(group_type),
              disjunction_capture_index_(disjunction_capture_index)
        {}
        
        RegExpParserState* previous_state() { return previous_state_; }
        bool IsSubexpression() { return previous_state_ != nullptr; }
        
        RegExpBuilder* builder() { return builder_; }
        
        SubexpressionType group_type() { return group_type_; }
        
        
        
        int capture_index() { return disjunction_capture_index_; }

      private:
        
        RegExpParserState* previous_state_;
        
        RegExpBuilder* builder_;
        
        SubexpressionType group_type_;
        
        int disjunction_capture_index_;
    };

    widechar current() { return current_; }
    bool has_more() { return has_more_; }
    bool has_next() { return next_pos_ < end_; }
    widechar Next() {
        if (has_next())
            return *next_pos_;
        return kEndMarker;
    }
    void ScanForCaptures();

    frontend::TokenStream& ts;
    LifoAlloc* alloc;
    RegExpCaptureVector* captures_;
    const CharT* next_pos_;
    const CharT* end_;
    widechar current_;
    
    int capture_count_;
    bool has_more_;
    bool multiline_;
    bool simple_;
    bool contains_anchor_;
    bool is_scanned_for_captures_;
};

} } 

#endif
