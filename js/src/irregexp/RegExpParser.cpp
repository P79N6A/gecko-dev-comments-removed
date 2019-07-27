





























#include "irregexp/RegExpParser.h"

#include "frontend/TokenStream.h"

using namespace js;
using namespace js::irregexp;




RegExpBuilder::RegExpBuilder(LifoAlloc *alloc)
  : alloc(alloc),
    pending_empty_(false),
    characters_(nullptr),
    last_added_(ADD_NONE)
{}

void
RegExpBuilder::FlushCharacters()
{
    pending_empty_ = false;
    if (characters_ != nullptr) {
        RegExpTree* atom = alloc->newInfallible<RegExpAtom>(characters_);
        characters_ = nullptr;
        text_.Add(alloc, atom);
        last_added_ = ADD_ATOM;
    }
}

void
RegExpBuilder::FlushText()
{
    FlushCharacters();
    int num_text = text_.length();
    if (num_text == 0)
        return;
    if (num_text == 1) {
        terms_.Add(alloc, text_.last());
    } else {
        RegExpText* text = alloc->newInfallible<RegExpText>(alloc);
        for (int i = 0; i < num_text; i++)
            text_.Get(i)->AppendToText(text);
        terms_.Add(alloc, text);
    }
    text_.Clear();
}

void
RegExpBuilder::AddCharacter(char16_t c)
{
    pending_empty_ = false;
    if (characters_ == nullptr)
        characters_ = alloc->newInfallible<CharacterVector>(*alloc);
    characters_->append(c);
    last_added_ = ADD_CHAR;
}

void
RegExpBuilder::AddEmpty()
{
    pending_empty_ = true;
}

void
RegExpBuilder::AddAtom(RegExpTree* term)
{
    if (term->IsEmpty()) {
        AddEmpty();
        return;
    }
    if (term->IsTextElement()) {
        FlushCharacters();
        text_.Add(alloc, term);
    } else {
        FlushText();
        terms_.Add(alloc, term);
    }
    last_added_ = ADD_ATOM;
}

void
RegExpBuilder::AddAssertion(RegExpTree *assert)
{
    FlushText();
    terms_.Add(alloc, assert);
    last_added_ = ADD_ASSERT;
}

void
RegExpBuilder::NewAlternative()
{
    FlushTerms();
}

void
RegExpBuilder::FlushTerms()
{
    FlushText();
    int num_terms = terms_.length();
    RegExpTree* alternative;
    if (num_terms == 0)
        alternative = RegExpEmpty::GetInstance();
    else if (num_terms == 1)
        alternative = terms_.last();
    else
        alternative = alloc->newInfallible<RegExpAlternative>(terms_.GetList(alloc));
    alternatives_.Add(alloc, alternative);
    terms_.Clear();
    last_added_ = ADD_NONE;
}

RegExpTree *
RegExpBuilder::ToRegExp()
{
    FlushTerms();
    int num_alternatives = alternatives_.length();
    if (num_alternatives == 0) {
        return RegExpEmpty::GetInstance();
    }
    if (num_alternatives == 1) {
        return alternatives_.last();
    }
    return alloc->newInfallible<RegExpDisjunction>(alternatives_.GetList(alloc));
}

void
RegExpBuilder::AddQuantifierToAtom(int min, int max,
                                   RegExpQuantifier::QuantifierType quantifier_type)
{
    if (pending_empty_) {
        pending_empty_ = false;
        return;
    }
    RegExpTree* atom;
    if (characters_ != nullptr) {
        JS_ASSERT(last_added_ == ADD_CHAR);
        
        CharacterVector *char_vector = characters_;
        int num_chars = char_vector->length();
        if (num_chars > 1) {
            CharacterVector *prefix = alloc->newInfallible<CharacterVector>(*alloc);
            prefix->append(char_vector->begin(), num_chars - 1);
            text_.Add(alloc, alloc->newInfallible<RegExpAtom>(prefix));
            char_vector = alloc->newInfallible<CharacterVector>(*alloc);
            char_vector->append((*characters_)[num_chars - 1]);
        }
        characters_ = nullptr;
        atom = alloc->newInfallible<RegExpAtom>(char_vector);
        FlushText();
    } else if (text_.length() > 0) {
        JS_ASSERT(last_added_ == ADD_ATOM);
        atom = text_.RemoveLast();
        FlushText();
    } else if (terms_.length() > 0) {
        JS_ASSERT(last_added_ == ADD_ATOM);
        atom = terms_.RemoveLast();
        if (atom->max_match() == 0) {
            
            last_added_ = ADD_TERM;
            if (min == 0)
                return;
            terms_.Add(alloc, atom);
            return;
        }
    } else {
        
        MOZ_CRASH("Bad call");
    }
    terms_.Add(alloc, alloc->newInfallible<RegExpQuantifier>(min, max, quantifier_type, atom));
    last_added_ = ADD_TERM;
}




template <typename CharT>
RegExpParser<CharT>::RegExpParser(frontend::TokenStream &ts, LifoAlloc *alloc,
                                  const CharT *chars, const CharT *end, bool multiline_mode)
  : ts(ts),
    alloc(alloc),
    captures_(nullptr),
    next_pos_(chars),
    end_(end),
    current_(kEndMarker),
    capture_count_(0),
    has_more_(true),
    multiline_(multiline_mode),
    simple_(false),
    contains_anchor_(false),
    is_scanned_for_captures_(false)
{
    Advance();
}

template <typename CharT>
RegExpTree *
RegExpParser<CharT>::ReportError(unsigned errorNumber)
{
    gc::AutoSuppressGC suppressGC(ts.context());
    ts.reportError(errorNumber);
    return nullptr;
}

template <typename CharT>
void
RegExpParser<CharT>::Advance()
{
    if (next_pos_ < end_) {
        current_ = *next_pos_;
        next_pos_++;
    } else {
        current_ = kEndMarker;
        has_more_ = false;
    }
}



inline int
HexValue(uint32_t c)
{
    c -= '0';
    if (static_cast<unsigned>(c) <= 9) return c;
    c = (c | 0x20) - ('a' - '0');  
    if (static_cast<unsigned>(c) <= 5) return c + 10;
    return -1;
}

template <typename CharT>
size_t
RegExpParser<CharT>::ParseOctalLiteral()
{
    JS_ASSERT('0' <= current() && current() <= '7');
    
    
    widechar value = current() - '0';
    Advance();
    if ('0' <= current() && current() <= '7') {
        value = value * 8 + current() - '0';
        Advance();
        if (value < 32 && '0' <= current() && current() <= '7') {
            value = value * 8 + current() - '0';
            Advance();
        }
    }
    return value;
}

template <typename CharT>
bool
RegExpParser<CharT>::ParseHexEscape(int length, size_t *value)
{
    const CharT *start = position();
    uint32_t val = 0;
    bool done = false;
    for (int i = 0; !done; i++) {
        widechar c = current();
        int d = HexValue(c);
        if (d < 0) {
            Reset(start);
            return false;
        }
        val = val * 16 + d;
        Advance();
        if (i == length - 1) {
            done = true;
        }
    }
    *value = val;
    return true;
}

#ifdef DEBUG

static bool
IsSpecialClassEscape(widechar c)
{
  switch (c) {
    case 'd': case 'D':
    case 's': case 'S':
    case 'w': case 'W':
      return true;
    default:
      return false;
  }
}
#endif

template <typename CharT>
widechar
RegExpParser<CharT>::ParseClassCharacterEscape()
{
    JS_ASSERT(current() == '\\');
    JS_ASSERT(has_next() && !IsSpecialClassEscape(Next()));
    Advance();
    switch (current()) {
      case 'b':
        Advance();
        return '\b';
      
      
      case 'f':
        Advance();
        return '\f';
      case 'n':
        Advance();
        return '\n';
      case 'r':
        Advance();
        return '\r';
      case 't':
        Advance();
        return '\t';
      case 'v':
        Advance();
        return '\v';
      case 'c': {
        widechar controlLetter = Next();
        widechar letter = controlLetter & ~('A' ^ 'a');
        
        
        if ((controlLetter >= '0' && controlLetter <= '9') ||
            controlLetter == '_' ||
            (letter >= 'A' && letter <= 'Z')) {
            Advance(2);
            
            
            return controlLetter & 0x1f;
        }
        
        
        return '\\';
      }
      case '0': case '1': case '2': case '3': case '4': case '5':
      case '6': case '7':
        
        
        
        return ParseOctalLiteral();
      case 'x': {
        Advance();
        size_t value;
        if (ParseHexEscape(2, &value))
            return value;
        
        
        return 'x';
      }
      case 'u': {
        Advance();
        size_t value;
        if (ParseHexEscape(4, &value))
            return value;
        
        
        return 'u';
      }
      default: {
        
        
        
        widechar result = current();
        Advance();
        return result;
      }
    }
    return 0;
}

static const char16_t kNoCharClass = 0;




static inline void
AddRangeOrEscape(LifoAlloc *alloc,
                 CharacterRangeVector *ranges,
                 char16_t char_class,
                 CharacterRange range)
{
    if (char_class != kNoCharClass)
        CharacterRange::AddClassEscape(alloc, char_class, ranges);
    else
        ranges->append(range);
}

template <typename CharT>
RegExpTree*
RegExpParser<CharT>::ParseCharacterClass()
{
    JS_ASSERT(current() == '[');
    Advance();
    bool is_negated = false;
    if (current() == '^') {
        is_negated = true;
        Advance();
    }
    CharacterRangeVector *ranges = alloc->newInfallible<CharacterRangeVector>(*alloc);
    while (has_more() && current() != ']') {
        char16_t char_class = kNoCharClass;
        CharacterRange first;
        if (!ParseClassAtom(&char_class, &first))
            return nullptr;
        if (current() == '-') {
            Advance();
            if (current() == kEndMarker) {
                
                
                break;
            } else if (current() == ']') {
                AddRangeOrEscape(alloc, ranges, char_class, first);
                ranges->append(CharacterRange::Singleton('-'));
                break;
            }
            char16_t char_class_2 = kNoCharClass;
            CharacterRange next;
            if (!ParseClassAtom(&char_class_2, &next))
                return nullptr;
            if (char_class != kNoCharClass || char_class_2 != kNoCharClass) {
                
                AddRangeOrEscape(alloc, ranges, char_class, first);
                ranges->append(CharacterRange::Singleton('-'));
                AddRangeOrEscape(alloc, ranges, char_class_2, next);
                continue;
            }
            if (first.from() > next.to())
                return ReportError(JSMSG_BAD_CLASS_RANGE);
            ranges->append(CharacterRange::Range(first.from(), next.to()));
        } else {
            AddRangeOrEscape(alloc, ranges, char_class, first);
        }
    }
    if (!has_more())
        return ReportError(JSMSG_UNTERM_CLASS);
    Advance();
    if (ranges->length() == 0) {
        ranges->append(CharacterRange::Everything());
        is_negated = !is_negated;
    }
    return alloc->newInfallible<RegExpCharacterClass>(ranges, is_negated);
}

template <typename CharT>
bool
RegExpParser<CharT>::ParseClassAtom(char16_t* char_class, CharacterRange *char_range)
{
    JS_ASSERT(*char_class == kNoCharClass);
    widechar first = current();
    if (first == '\\') {
        switch (Next()) {
          case 'w': case 'W': case 'd': case 'D': case 's': case 'S': {
            *char_class = Next();
            Advance(2);
            return true;
          }
          case kEndMarker:
            return ReportError(JSMSG_ESCAPE_AT_END_OF_REGEXP);
          default:
            widechar c = ParseClassCharacterEscape();
            *char_range = CharacterRange::Singleton(c);
            return true;
        }
    } else {
        Advance();
        *char_range = CharacterRange::Singleton(first);
        return true;
    }
}







template <typename CharT>
void
RegExpParser<CharT>::ScanForCaptures()
{
    
    int capture_count = captures_started();
    
    widechar n;
    while ((n = current()) != kEndMarker) {
        Advance();
        switch (n) {
          case '\\':
            Advance();
            break;
          case '[': {
            int c;
            while ((c = current()) != kEndMarker) {
                Advance();
                if (c == '\\') {
                    Advance();
                } else {
                    if (c == ']') break;
                }
            }
            break;
          }
          case '(':
            if (current() != '?') capture_count++;
            break;
        }
    }
    capture_count_ = capture_count;
    is_scanned_for_captures_ = true;
}

inline bool
IsInRange(int value, int lower_limit, int higher_limit)
{
    JS_ASSERT(lower_limit <= higher_limit);
    return static_cast<unsigned int>(value - lower_limit) <=
           static_cast<unsigned int>(higher_limit - lower_limit);
}

inline bool
IsDecimalDigit(widechar c)
{
    
    return IsInRange(c, '0', '9');
}

template <typename CharT>
bool
RegExpParser<CharT>::ParseBackReferenceIndex(int* index_out)
{
    JS_ASSERT('\\' == current());
    JS_ASSERT('1' <= Next() && Next() <= '9');

    
    
    const CharT *start = position();
    int value = Next() - '0';
    Advance(2);
    while (true) {
        widechar c = current();
        if (IsDecimalDigit(c)) {
            value = 10 * value + (c - '0');
            if (value > kMaxCaptures) {
                Reset(start);
                return false;
            }
            Advance();
        } else {
            break;
        }
    }
    if (value > captures_started()) {
        if (!is_scanned_for_captures_) {
            const CharT *saved_position = position();
            ScanForCaptures();
            Reset(saved_position);
        }
        if (value > capture_count_) {
            Reset(start);
            return false;
        }
    }
    *index_out = value;
    return true;
}








template <typename CharT>
bool
RegExpParser<CharT>::ParseIntervalQuantifier(int* min_out, int* max_out)
{
    JS_ASSERT(current() == '{');
    const CharT *start = position();
    Advance();
    int min = 0;
    if (!IsDecimalDigit(current())) {
        Reset(start);
        return false;
    }
    while (IsDecimalDigit(current())) {
        int next = current() - '0';
        if (min > (RegExpTree::kInfinity - next) / 10) {
            
            do {
                Advance();
            } while (IsDecimalDigit(current()));
            min = RegExpTree::kInfinity;
            break;
        }
        min = 10 * min + next;
        Advance();
    }
    int max = 0;
    if (current() == '}') {
        max = min;
        Advance();
    } else if (current() == ',') {
        Advance();
        if (current() == '}') {
            max = RegExpTree::kInfinity;
            Advance();
        } else {
            while (IsDecimalDigit(current())) {
                int next = current() - '0';
                if (max > (RegExpTree::kInfinity - next) / 10) {
                    do {
                        Advance();
                    } while (IsDecimalDigit(current()));
                    max = RegExpTree::kInfinity;
                    break;
                }
                max = 10 * max + next;
                Advance();
            }
            if (current() != '}') {
                Reset(start);
                return false;
            }
            Advance();
        }
    } else {
        Reset(start);
        return false;
    }
    *min_out = min;
    *max_out = max;
    return true;
}



template <typename CharT>
RegExpTree *
RegExpParser<CharT>::ParsePattern()
{
    RegExpTree* result = ParseDisjunction();
    JS_ASSERT_IF(result, !has_more());
    return result;
}











template <typename CharT>
RegExpTree*
RegExpParser<CharT>::ParseDisjunction()
{
    
    RegExpParserState initial_state(alloc, nullptr, INITIAL, 0);
    RegExpParserState* stored_state = &initial_state;
    
    RegExpBuilder* builder = initial_state.builder();
    while (true) {
        switch (current()) {
          case kEndMarker:
            if (stored_state->IsSubexpression()) {
                
                return ReportError(JSMSG_MISSING_PAREN);
            }
            JS_ASSERT(INITIAL == stored_state->group_type());
            
            return builder->ToRegExp();
          case ')': {
            if (!stored_state->IsSubexpression())
                return ReportError(JSMSG_UNMATCHED_RIGHT_PAREN);
            JS_ASSERT(INITIAL != stored_state->group_type());

            Advance();
            
            
            RegExpTree* body = builder->ToRegExp();

            int end_capture_index = captures_started();

            int capture_index = stored_state->capture_index();
            SubexpressionType group_type = stored_state->group_type();

            
            stored_state = stored_state->previous_state();
            builder = stored_state->builder();

            
            if (group_type == CAPTURE) {
                RegExpCapture* capture = alloc->newInfallible<RegExpCapture>(body, capture_index);
                (*captures_)[capture_index - 1] = capture;
                body = capture;
            } else if (group_type != GROUPING) {
                JS_ASSERT(group_type == POSITIVE_LOOKAHEAD ||
                          group_type == NEGATIVE_LOOKAHEAD);
                bool is_positive = (group_type == POSITIVE_LOOKAHEAD);
                body = alloc->newInfallible<RegExpLookahead>(body,
                                                   is_positive,
                                                   end_capture_index - capture_index,
                                                   capture_index);
            }
            builder->AddAtom(body);
            
            
            break;
          }
          case '|': {
            Advance();
            builder->NewAlternative();
            continue;
          }
          case '*':
          case '+':
          case '?':
            return ReportError(JSMSG_NOTHING_TO_REPEAT);
          case '^': {
            Advance();
            if (multiline_) {
                builder->AddAssertion(alloc->newInfallible<RegExpAssertion>(RegExpAssertion::START_OF_LINE));
            } else {
                builder->AddAssertion(alloc->newInfallible<RegExpAssertion>(RegExpAssertion::START_OF_INPUT));
                set_contains_anchor();
            }
            continue;
          }
          case '$': {
            Advance();
            RegExpAssertion::AssertionType assertion_type =
                multiline_ ? RegExpAssertion::END_OF_LINE :
                RegExpAssertion::END_OF_INPUT;
            builder->AddAssertion(alloc->newInfallible<RegExpAssertion>(assertion_type));
            continue;
          }
          case '.': {
            Advance();
            
            CharacterRangeVector *ranges = alloc->newInfallible<CharacterRangeVector>(*alloc);
            CharacterRange::AddClassEscape(alloc, '.', ranges);
            RegExpTree* atom = alloc->newInfallible<RegExpCharacterClass>(ranges, false);
            builder->AddAtom(atom);
            break;
          }
          case '(': {
            SubexpressionType subexpr_type = CAPTURE;
            Advance();
            if (current() == '?') {
                switch (Next()) {
                  case ':':
                    subexpr_type = GROUPING;
                    break;
                  case '=':
                    subexpr_type = POSITIVE_LOOKAHEAD;
                    break;
                  case '!':
                    subexpr_type = NEGATIVE_LOOKAHEAD;
                    break;
                  default:
                    return ReportError(JSMSG_INVALID_GROUP);
                }
                Advance(2);
            } else {
                if (captures_ == nullptr)
                    captures_ = alloc->newInfallible<RegExpCaptureVector>(*alloc);
                if (captures_started() >= kMaxCaptures)
                    return ReportError(JSMSG_TOO_MANY_PARENS);
                captures_->append((RegExpCapture *) nullptr);
            }
            
            stored_state = alloc->newInfallible<RegExpParserState>(alloc, stored_state, subexpr_type,
                                                                   captures_started());
            builder = stored_state->builder();
            continue;
          }
          case '[': {
            RegExpTree* atom = ParseCharacterClass();
            if (!atom)
                return nullptr;
            builder->AddAtom(atom);
            break;
          }
            
            
          case '\\':
            switch (Next()) {
              case kEndMarker:
                return ReportError(JSMSG_ESCAPE_AT_END_OF_REGEXP);
              case 'b':
                Advance(2);
                builder->AddAssertion(alloc->newInfallible<RegExpAssertion>(RegExpAssertion::BOUNDARY));
                continue;
              case 'B':
                Advance(2);
                builder->AddAssertion(alloc->newInfallible<RegExpAssertion>(RegExpAssertion::NON_BOUNDARY));
                continue;
                
                
                
                
                
              case 'd': case 'D': case 's': case 'S': case 'w': case 'W': {
                widechar c = Next();
                Advance(2);
                CharacterRangeVector *ranges =
                    alloc->newInfallible<CharacterRangeVector>(*alloc);
                CharacterRange::AddClassEscape(alloc, c, ranges);
                RegExpTree* atom = alloc->newInfallible<RegExpCharacterClass>(ranges, false);
                builder->AddAtom(atom);
                break;
              }
              case '1': case '2': case '3': case '4': case '5': case '6':
              case '7': case '8': case '9': {
                int index = 0;
                if (ParseBackReferenceIndex(&index)) {
                    RegExpCapture* capture = nullptr;
                    if (captures_ != nullptr && index <= (int) captures_->length()) {
                        capture = (*captures_)[index - 1];
                    }
                    if (capture == nullptr) {
                        builder->AddEmpty();
                        break;
                    }
                    RegExpTree *atom = alloc->newInfallible<RegExpBackReference>(capture);
                    builder->AddAtom(atom);
                    break;
                }
                widechar first_digit = Next();
                if (first_digit == '8' || first_digit == '9') {
                    
                    builder->AddCharacter(first_digit);
                    Advance(2);
                    break;
                }
              }
                
              case '0': {
                Advance();
                size_t octal = ParseOctalLiteral();
                builder->AddCharacter(octal);
                break;
              }
                
                
              case 'f':
                Advance(2);
                builder->AddCharacter('\f');
                break;
              case 'n':
                Advance(2);
                builder->AddCharacter('\n');
                break;
              case 'r':
                Advance(2);
                builder->AddCharacter('\r');
                break;
              case 't':
                Advance(2);
                builder->AddCharacter('\t');
                break;
              case 'v':
                Advance(2);
                builder->AddCharacter('\v');
                break;
              case 'c': {
                Advance();
                widechar controlLetter = Next();
                
                
                widechar letter = controlLetter & ~('a' ^ 'A');
                if (letter < 'A' || 'Z' < letter) {
                    
                    
                    
                    
                    builder->AddCharacter('\\');
                } else {
                    Advance(2);
                    builder->AddCharacter(controlLetter & 0x1f);
                }
                break;
              }
              case 'x': {
                Advance(2);
                size_t value;
                if (ParseHexEscape(2, &value)) {
                    builder->AddCharacter(value);
                } else {
                    builder->AddCharacter('x');
                }
                break;
              }
              case 'u': {
                Advance(2);
                size_t value;
                if (ParseHexEscape(4, &value)) {
                    builder->AddCharacter(value);
                } else {
                    builder->AddCharacter('u');
                }
                break;
              }
              default:
                
                builder->AddCharacter(Next());
                Advance(2);
                break;
            }
            break;
          case '{': {
            int dummy;
            if (ParseIntervalQuantifier(&dummy, &dummy))
                return ReportError(JSMSG_NOTHING_TO_REPEAT);
            
          }
          default:
            builder->AddCharacter(current());
            Advance();
            break;
        }  

        int min;
        int max;
        switch (current()) {
            
            
            
            
            
          case '*':
            min = 0;
            max = RegExpTree::kInfinity;
            Advance();
            break;
          case '+':
            min = 1;
            max = RegExpTree::kInfinity;
            Advance();
            break;
          case '?':
            min = 0;
            max = 1;
            Advance();
            break;
          case '{':
            if (ParseIntervalQuantifier(&min, &max)) {
                if (max < min)
                    return ReportError(JSMSG_NUMBERS_OUT_OF_ORDER);
                break;
            } else {
                continue;
            }
          default:
            continue;
        }
        RegExpQuantifier::QuantifierType quantifier_type = RegExpQuantifier::GREEDY;
        if (current() == '?') {
            quantifier_type = RegExpQuantifier::NON_GREEDY;
            Advance();
        }
        builder->AddQuantifierToAtom(min, max, quantifier_type);
    }
}

template class irregexp::RegExpParser<Latin1Char>;
template class irregexp::RegExpParser<char16_t>;

template <typename CharT>
static bool
ParsePattern(frontend::TokenStream &ts, LifoAlloc &alloc, const CharT *chars, size_t length,
             bool multiline, bool match_only, RegExpCompileData *data)
{
    if (match_only) {
        
        
        
        
        if (length >= 3 && chars[0] == '.' && chars[1] == '*' && chars[2] != '?') {
            chars += 2;
            length -= 2;
        }

        
        
        
        
        if (length >= 3 && !HasRegExpMetaChars(chars, length - 2) &&
            chars[length - 2] == '.' && chars[length - 1] == '*')
        {
            length -= 2;
        }
    }

    RegExpParser<CharT> parser(ts, &alloc, chars, chars + length, multiline);
    data->tree = parser.ParsePattern();
    if (!data->tree)
        return false;

    data->simple = parser.simple();
    data->contains_anchor = parser.contains_anchor();
    data->capture_count = parser.captures_started();
    return true;
}

bool
irregexp::ParsePattern(frontend::TokenStream &ts, LifoAlloc &alloc, JSAtom *str,
                       bool multiline, bool match_only,
                       RegExpCompileData *data)
{
    JS::AutoCheckCannotGC nogc;
    return str->hasLatin1Chars()
           ? ::ParsePattern(ts, alloc, str->latin1Chars(nogc), str->length(),
                            multiline, match_only, data)
           : ::ParsePattern(ts, alloc, str->twoByteChars(nogc), str->length(),
                            multiline, match_only, data);
}

template <typename CharT>
static bool
ParsePatternSyntax(frontend::TokenStream &ts, LifoAlloc &alloc, const CharT *chars, size_t length)
{
    LifoAllocScope scope(&alloc);

    RegExpParser<CharT> parser(ts, &alloc, chars, chars + length, false);
    return parser.ParsePattern() != nullptr;
}

bool
irregexp::ParsePatternSyntax(frontend::TokenStream &ts, LifoAlloc &alloc, JSAtom *str)
{
    JS::AutoCheckCannotGC nogc;
    return str->hasLatin1Chars()
           ? ::ParsePatternSyntax(ts, alloc, str->latin1Chars(nogc), str->length())
           : ::ParsePatternSyntax(ts, alloc, str->twoByteChars(nogc), str->length());
}
