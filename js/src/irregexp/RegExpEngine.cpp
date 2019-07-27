





























#include "irregexp/RegExpEngine.h"

#include "irregexp/NativeRegExpMacroAssembler.h"
#include "irregexp/RegExpMacroAssembler.h"
#include "jit/JitCommon.h"

using namespace js;
using namespace js::irregexp;

using mozilla::ArrayLength;
using mozilla::DebugOnly;
using mozilla::Maybe;

#define DEFINE_ACCEPT(Type)                                          \
    void Type##Node::Accept(NodeVisitor* visitor) {                  \
        visitor->Visit##Type(this);                                  \
    }
FOR_EACH_NODE_TYPE(DEFINE_ACCEPT)
#undef DEFINE_ACCEPT

void LoopChoiceNode::Accept(NodeVisitor* visitor) {
    visitor->VisitLoopChoice(this);
}

static const int kMaxLookaheadForBoyerMoore = 8;

RegExpNode::RegExpNode(LifoAlloc *alloc)
  : replacement_(nullptr), trace_count_(0), alloc_(alloc)
{
    bm_info_[0] = bm_info_[1] = nullptr;
}







static const int kSpaceRanges[] = { '\t', '\r' + 1, ' ', ' ' + 1,
    0x00A0, 0x00A1, 0x1680, 0x1681, 0x180E, 0x180F, 0x2000, 0x200B,
    0x2028, 0x202A, 0x202F, 0x2030, 0x205F, 0x2060, 0x3000, 0x3001,
    0xFEFF, 0xFF00, 0x10000 };
static const int kSpaceRangeCount = ArrayLength(kSpaceRanges);

static const int kWordRanges[] = {
    '0', '9' + 1, 'A', 'Z' + 1, '_', '_' + 1, 'a', 'z' + 1, 0x10000 };
static const int kWordRangeCount = ArrayLength(kWordRanges);
static const int kDigitRanges[] = { '0', '9' + 1, 0x10000 };
static const int kDigitRangeCount = ArrayLength(kDigitRanges);
static const int kSurrogateRanges[] = { 0xd800, 0xe000, 0x10000 };
static const int kSurrogateRangeCount = ArrayLength(kSurrogateRanges);
static const int kLineTerminatorRanges[] = { 0x000A, 0x000B, 0x000D, 0x000E,
    0x2028, 0x202A, 0x10000 };
static const int kLineTerminatorRangeCount = ArrayLength(kLineTerminatorRanges);
static const unsigned kMaxOneByteCharCode = 0xff;
static const int kMaxUtf16CodeUnit = 0xffff;

static char16_t
MaximumCharacter(bool ascii)
{
    return ascii ? kMaxOneByteCharCode : kMaxUtf16CodeUnit;
}

static void
AddClass(const int* elmv, int elmc,
         CharacterRangeVector *ranges)
{
    elmc--;
    JS_ASSERT(elmv[elmc] == 0x10000);
    for (int i = 0; i < elmc; i += 2) {
        JS_ASSERT(elmv[i] < elmv[i + 1]);
        ranges->append(CharacterRange(elmv[i], elmv[i + 1] - 1));
    }
}

static void
AddClassNegated(const int *elmv,
                int elmc,
                CharacterRangeVector *ranges)
{
    elmc--;
    JS_ASSERT(elmv[elmc] == 0x10000);
    JS_ASSERT(elmv[0] != 0x0000);
    JS_ASSERT(elmv[elmc-1] != kMaxUtf16CodeUnit);
    char16_t last = 0x0000;
    for (int i = 0; i < elmc; i += 2) {
        JS_ASSERT(last <= elmv[i] - 1);
        JS_ASSERT(elmv[i] < elmv[i + 1]);
        ranges->append(CharacterRange(last, elmv[i] - 1));
        last = elmv[i + 1];
    }
    ranges->append(CharacterRange(last, kMaxUtf16CodeUnit));
}

void
CharacterRange::AddClassEscape(LifoAlloc *alloc, char16_t type,
			       CharacterRangeVector *ranges)
{
    switch (type) {
      case 's':
        AddClass(kSpaceRanges, kSpaceRangeCount, ranges);
        break;
      case 'S':
        AddClassNegated(kSpaceRanges, kSpaceRangeCount, ranges);
        break;
      case 'w':
        AddClass(kWordRanges, kWordRangeCount, ranges);
        break;
      case 'W':
        AddClassNegated(kWordRanges, kWordRangeCount, ranges);
        break;
      case 'd':
        AddClass(kDigitRanges, kDigitRangeCount, ranges);
        break;
      case 'D':
        AddClassNegated(kDigitRanges, kDigitRangeCount, ranges);
        break;
      case '.':
        AddClassNegated(kLineTerminatorRanges, kLineTerminatorRangeCount, ranges);
        break;
        
        
        
      case '*':
        ranges->append(CharacterRange::Everything());
        break;
        
        
      case 'n':
        AddClass(kLineTerminatorRanges, kLineTerminatorRangeCount, ranges);
        break;
      default:
        MOZ_CRASH("Bad character class escape");
    }
}


static inline bool
RangeContainsLatin1Equivalents(CharacterRange range)
{
    
    return range.Contains(0x39c) || range.Contains(0x3bc) || range.Contains(0x178);
}

static bool
RangesContainLatin1Equivalents(const CharacterRangeVector &ranges)
{
    for (size_t i = 0; i < ranges.length(); i++) {
        
        if (RangeContainsLatin1Equivalents(ranges[i]))
            return true;
    }
    return false;
}

static const size_t kEcma262UnCanonicalizeMaxWidth = 4;



static int
GetCaseIndependentLetters(char16_t character,
                          bool ascii_subject,
                          char16_t *letters)
{
    const char16_t choices[] = {
        character,
        unicode::ToLowerCase(character),
        unicode::ToUpperCase(character)
    };

    size_t count = 0;
    for (size_t i = 0; i < ArrayLength(choices); i++) {
        char16_t c = choices[i];

        
        
        
        static const unsigned kMaxAsciiCharCode = 127;
        if (character > kMaxAsciiCharCode && c <= kMaxAsciiCharCode)
            continue;

        
        if (ascii_subject && c > kMaxOneByteCharCode)
            continue;

        
        bool found = false;
        for (size_t j = 0; j < count; j++) {
            if (letters[j] == c) {
                found = true;
                break;
            }
        }
        if (found)
            continue;

        letters[count++] = c;
    }

    return count;
}

static char16_t
ConvertNonLatin1ToLatin1(char16_t c)
{
    JS_ASSERT(c > kMaxOneByteCharCode);
    switch (c) {
      
      case 0x39c:
      case 0x3bc:
        return 0xb5;
      
      
      case 0x178:
        return 0xff;
    }
    return 0;
}

void
CharacterRange::AddCaseEquivalents(bool is_ascii, CharacterRangeVector *ranges)
{
    char16_t bottom = from();
    char16_t top = to();

    if (is_ascii && !RangeContainsLatin1Equivalents(*this)) {
        if (bottom > kMaxOneByteCharCode)
            return;
        if (top > kMaxOneByteCharCode)
            top = kMaxOneByteCharCode;
    }

    for (char16_t c = bottom;; c++) {
        char16_t chars[kEcma262UnCanonicalizeMaxWidth];
        size_t length = GetCaseIndependentLetters(c, is_ascii, chars);

        for (size_t i = 0; i < length; i++) {
            char16_t other = chars[i];
            if (other == c)
                continue;

            
            bool found = false;
            for (size_t i = 0; i < ranges->length(); i++) {
                CharacterRange &range = (*ranges)[i];
                if (range.Contains(other)) {
                    found = true;
                    break;
                } else if (other == range.from() - 1) {
                    range.set_from(other);
                    found = true;
                    break;
                } else if (other == range.to() + 1) {
                    range.set_to(other);
                    found = true;
                    break;
                }
            }

            if (!found)
                ranges->append(CharacterRange::Singleton(other));
        }

        if (c == top)
            break;
    }
}

static bool
CompareInverseRanges(const CharacterRangeVector &ranges, const int *special_class, size_t length)
{
    length--;  
    JS_ASSERT(special_class[length] == 0x10000);
    JS_ASSERT(ranges.length() != 0);
    JS_ASSERT(length != 0);
    JS_ASSERT(special_class[0] != 0);
    if (ranges.length() != (length >> 1) + 1)
        return false;
    CharacterRange range = ranges[0];
    if (range.from() != 0)
        return false;
    for (size_t i = 0; i < length; i += 2) {
        if (special_class[i] != (range.to() + 1))
            return false;
        range = ranges[(i >> 1) + 1];
        if (special_class[i+1] != range.from())
            return false;
    }
    if (range.to() != 0xffff)
        return false;
    return true;
}

static bool
CompareRanges(const CharacterRangeVector &ranges, const int *special_class, size_t length)
{
    length--;  
    JS_ASSERT(special_class[length] == 0x10000);
    if (ranges.length() * 2 != length)
        return false;
    for (size_t i = 0; i < length; i += 2) {
        CharacterRange range = ranges[i >> 1];
        if (range.from() != special_class[i] || range.to() != special_class[i + 1] - 1)
            return false;
    }
    return true;
}

bool
RegExpCharacterClass::is_standard(LifoAlloc *alloc)
{
    
    
    if (is_negated_)
        return false;
    if (set_.is_standard())
        return true;
    if (CompareRanges(set_.ranges(alloc), kSpaceRanges, kSpaceRangeCount)) {
        set_.set_standard_set_type('s');
        return true;
    }
    if (CompareInverseRanges(set_.ranges(alloc), kSpaceRanges, kSpaceRangeCount)) {
        set_.set_standard_set_type('S');
        return true;
    }
    if (CompareInverseRanges(set_.ranges(alloc),
                             kLineTerminatorRanges,
                             kLineTerminatorRangeCount)) {
        set_.set_standard_set_type('.');
        return true;
    }
    if (CompareRanges(set_.ranges(alloc),
                      kLineTerminatorRanges,
                      kLineTerminatorRangeCount)) {
        set_.set_standard_set_type('n');
        return true;
    }
    if (CompareRanges(set_.ranges(alloc), kWordRanges, kWordRangeCount)) {
        set_.set_standard_set_type('w');
        return true;
    }
    if (CompareInverseRanges(set_.ranges(alloc), kWordRanges, kWordRangeCount)) {
        set_.set_standard_set_type('W');
        return true;
    }
    return false;
}

bool
CharacterRange::IsCanonical(const CharacterRangeVector &ranges)
{
    int n = ranges.length();
    if (n <= 1)
        return true;

    int max = ranges[0].to();
    for (int i = 1; i < n; i++) {
        CharacterRange next_range = ranges[i];
        if (next_range.from() <= max + 1)
            return false;
        max = next_range.to();
    }
    return true;
}



static
void MoveRanges(CharacterRangeVector &list, int from, int to, int count)
{
    
    if (from < to) {
        for (int i = count - 1; i >= 0; i--)
            list[to + i] = list[from + i];
    } else {
        for (int i = 0; i < count; i++)
            list[to + i] = list[from + i];
    }
}

static int
InsertRangeInCanonicalList(CharacterRangeVector &list,
                           int count,
                           CharacterRange insert)
{
    
    
    
    
    
    char16_t from = insert.from();
    char16_t to = insert.to();
    int start_pos = 0;
    int end_pos = count;
    for (int i = count - 1; i >= 0; i--) {
        CharacterRange current = list[i];
        if (current.from() > to + 1) {
            end_pos = i;
        } else if (current.to() + 1 < from) {
            start_pos = i + 1;
            break;
        }
    }

    
    
    
    
    
    

    if (start_pos == end_pos) {
        
        if (start_pos < count) {
            MoveRanges(list, start_pos, start_pos + 1, count - start_pos);
        }
        list[start_pos] = insert;
        return count + 1;
    }
    if (start_pos + 1 == end_pos) {
        
        CharacterRange to_replace = list[start_pos];
        int new_from = Min(to_replace.from(), from);
        int new_to = Max(to_replace.to(), to);
        list[start_pos] = CharacterRange(new_from, new_to);
        return count;
    }
    
    

    int new_from = Min(list[start_pos].from(), from);
    int new_to = Max(list[end_pos - 1].to(), to);
    if (end_pos < count) {
        MoveRanges(list, end_pos, start_pos + 1, count - end_pos);
    }
    list[start_pos] = CharacterRange(new_from, new_to);
    return count - (end_pos - start_pos) + 1;
}

void
CharacterRange::Canonicalize(CharacterRangeVector &character_ranges)
{
    if (character_ranges.length() <= 1) return;
    
    
    int n = character_ranges.length();
    int max = character_ranges[0].to();
    int i = 1;
    while (i < n) {
        CharacterRange current = character_ranges[i];
        if (current.from() <= max + 1) {
            break;
        }
        max = current.to();
        i++;
    }
    
    if (i == n) return;

    
    
    
    
    
    int read = i;  
    size_t num_canonical = i;  
    do {
        num_canonical = InsertRangeInCanonicalList(character_ranges,
                                                   num_canonical,
                                                   character_ranges[read]);
        read++;
    } while (read < n);

    while (character_ranges.length() > num_canonical)
        character_ranges.popBack();

    JS_ASSERT(CharacterRange::IsCanonical(character_ranges));
}




class VisitMarker
{
  public:
    explicit VisitMarker(NodeInfo* info)
      : info_(info)
    {
        JS_ASSERT(!info->visited);
        info->visited = true;
    }
    ~VisitMarker() {
        info_->visited = false;
    }
  private:
    NodeInfo* info_;
};

bool
SeqRegExpNode::FillInBMInfo(int offset,
                            int budget,
                            BoyerMooreLookahead* bm,
                            bool not_at_start)
{
    if (!bm->CheckOverRecursed())
        return false;
    if (!on_success_->FillInBMInfo(offset, budget - 1, bm, not_at_start))
        return false;
    if (offset == 0)
        set_bm_info(not_at_start, bm);
    return true;
}

RegExpNode *
SeqRegExpNode::FilterASCII(int depth, bool ignore_case)
{
    if (info()->replacement_calculated)
        return replacement();

    if (depth < 0)
        return this;

    JS_ASSERT(!info()->visited);
    VisitMarker marker(info());
    return FilterSuccessor(depth - 1, ignore_case);
}

RegExpNode *
SeqRegExpNode::FilterSuccessor(int depth, bool ignore_case)
{
    RegExpNode* next = on_success_->FilterASCII(depth - 1, ignore_case);
    if (next == nullptr)
        return set_replacement(nullptr);

    on_success_ = next;
    return set_replacement(this);
}




int
ActionNode::EatsAtLeast(int still_to_find, int budget, bool not_at_start)
{
    if (budget <= 0)
        return 0;
    if (action_type_ == POSITIVE_SUBMATCH_SUCCESS)
        return 0;  
    return on_success()->EatsAtLeast(still_to_find,
                                     budget - 1,
                                     not_at_start);
}

bool
ActionNode::FillInBMInfo(int offset,
                         int budget,
                         BoyerMooreLookahead* bm,
                         bool not_at_start)
{
    if (!bm->CheckOverRecursed())
        return false;

    if (action_type_ == BEGIN_SUBMATCH) {
        bm->SetRest(offset);
    } else if (action_type_ != POSITIVE_SUBMATCH_SUCCESS) {
        if (!on_success()->FillInBMInfo(offset, budget - 1, bm, not_at_start))
            return false;
    }
    SaveBMInfo(bm, not_at_start, offset);

    return true;
}

 ActionNode *
ActionNode::SetRegister(int reg,
                        int val,
                        RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(SET_REGISTER, on_success);
    result->data_.u_store_register.reg = reg;
    result->data_.u_store_register.value = val;
    return result;
}

 ActionNode *
ActionNode::IncrementRegister(int reg, RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(INCREMENT_REGISTER, on_success);
    result->data_.u_increment_register.reg = reg;
    return result;
}

 ActionNode *
ActionNode::StorePosition(int reg, bool is_capture, RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(STORE_POSITION, on_success);
    result->data_.u_position_register.reg = reg;
    result->data_.u_position_register.is_capture = is_capture;
    return result;
}

 ActionNode *
ActionNode::ClearCaptures(Interval range, RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(CLEAR_CAPTURES, on_success);
    result->data_.u_clear_captures.range_from = range.from();
    result->data_.u_clear_captures.range_to = range.to();
    return result;
}

 ActionNode *
ActionNode::BeginSubmatch(int stack_pointer_reg, int position_reg, RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(BEGIN_SUBMATCH, on_success);
    result->data_.u_submatch.stack_pointer_register = stack_pointer_reg;
    result->data_.u_submatch.current_position_register = position_reg;
    return result;
}

 ActionNode *
ActionNode::PositiveSubmatchSuccess(int stack_pointer_reg,
                                    int restore_reg,
                                    int clear_capture_count,
                                    int clear_capture_from,
                                    RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(POSITIVE_SUBMATCH_SUCCESS, on_success);
    result->data_.u_submatch.stack_pointer_register = stack_pointer_reg;
    result->data_.u_submatch.current_position_register = restore_reg;
    result->data_.u_submatch.clear_register_count = clear_capture_count;
    result->data_.u_submatch.clear_register_from = clear_capture_from;
    return result;
}

 ActionNode *
ActionNode::EmptyMatchCheck(int start_register,
                            int repetition_register,
                            int repetition_limit,
                            RegExpNode *on_success)
{
    ActionNode *result = on_success->alloc()->newInfallible<ActionNode>(EMPTY_MATCH_CHECK, on_success);
    result->data_.u_empty_match_check.start_register = start_register;
    result->data_.u_empty_match_check.repetition_register = repetition_register;
    result->data_.u_empty_match_check.repetition_limit = repetition_limit;
    return result;
}




int
TextNode::EatsAtLeast(int still_to_find, int budget, bool not_at_start)
{
    int answer = Length();
    if (answer >= still_to_find)
        return answer;
    if (budget <= 0)
        return answer;

    
    return answer + on_success()->EatsAtLeast(still_to_find - answer,
                                              budget - 1,
                                              true);
}

int
TextNode::GreedyLoopTextLength()
{
    TextElement elm = elements()[elements().length() - 1];
    return elm.cp_offset() + elm.length();
}

RegExpNode *
TextNode::FilterASCII(int depth, bool ignore_case)
{
    if (info()->replacement_calculated)
        return replacement();

    if (depth < 0)
        return this;

    JS_ASSERT(!info()->visited);
    VisitMarker marker(info());
    int element_count = elements().length();
    for (int i = 0; i < element_count; i++) {
        TextElement elm = elements()[i];
        if (elm.text_type() == TextElement::ATOM) {
            CharacterVector &quarks = const_cast<CharacterVector &>(elm.atom()->data());
            for (size_t j = 0; j < quarks.length(); j++) {
                uint16_t c = quarks[j];
                if (c <= kMaxOneByteCharCode)
                    continue;
                if (!ignore_case)
                    return set_replacement(nullptr);

                
                
                char16_t converted = ConvertNonLatin1ToLatin1(c);
                if (converted == 0) {
                    
                    return set_replacement(nullptr);
                }

                
                quarks[j] = converted;
            }
        } else {
            JS_ASSERT(elm.text_type() == TextElement::CHAR_CLASS);
            RegExpCharacterClass* cc = elm.char_class();

            CharacterRangeVector &ranges = cc->ranges(alloc());
            if (!CharacterRange::IsCanonical(ranges))
                CharacterRange::Canonicalize(ranges);

            
            int range_count = ranges.length();
            if (cc->is_negated()) {
                if (range_count != 0 &&
                    ranges[0].from() == 0 &&
                    ranges[0].to() >= kMaxOneByteCharCode)
                {
                    
                    if (ignore_case && RangesContainLatin1Equivalents(ranges))
                        continue;
                    return set_replacement(nullptr);
                }
            } else {
                if (range_count == 0 ||
                    ranges[0].from() > kMaxOneByteCharCode)
                {
                    
                    if (ignore_case && RangesContainLatin1Equivalents(ranges))
                        continue;
                    return set_replacement(nullptr);
                }
            }
        }
    }
    return FilterSuccessor(depth - 1, ignore_case);
}

void
TextNode::CalculateOffsets()
{
    int element_count = elements().length();

    
    
    int cp_offset = 0;
    for (int i = 0; i < element_count; i++) {
        TextElement& elm = elements()[i];
        elm.set_cp_offset(cp_offset);
        cp_offset += elm.length();
    }
}

void TextNode::MakeCaseIndependent(bool is_ascii)
{
    int element_count = elements().length();
    for (int i = 0; i < element_count; i++) {
        TextElement elm = elements()[i];
        if (elm.text_type() == TextElement::CHAR_CLASS) {
            RegExpCharacterClass* cc = elm.char_class();

            
            
            if (cc->is_standard(alloc()))
                continue;

            CharacterRangeVector &ranges = cc->ranges(alloc());
            int range_count = ranges.length();
            for (int j = 0; j < range_count; j++)
                ranges[j].AddCaseEquivalents(is_ascii, &ranges);
        }
    }
}




int
AssertionNode::EatsAtLeast(int still_to_find, int budget, bool not_at_start)
{
    if (budget <= 0)
        return 0;

    
    
    
    
    
    if (assertion_type() == AT_START && not_at_start)
        return still_to_find;

    return on_success()->EatsAtLeast(still_to_find, budget - 1, not_at_start);
}

bool
AssertionNode::FillInBMInfo(int offset, int budget, BoyerMooreLookahead* bm, bool not_at_start)
{
    if (!bm->CheckOverRecursed())
        return false;

    
    if (assertion_type() == AT_START && not_at_start)
        return true;

    if (!on_success()->FillInBMInfo(offset, budget - 1, bm, not_at_start))
        return false;
    SaveBMInfo(bm, not_at_start, offset);
    return true;
}




int
BackReferenceNode::EatsAtLeast(int still_to_find, int budget, bool not_at_start)
{
    if (budget <= 0)
        return 0;
    return on_success()->EatsAtLeast(still_to_find, budget - 1, not_at_start);
}

bool
BackReferenceNode::FillInBMInfo(int offset, int budget, BoyerMooreLookahead* bm, bool not_at_start)
{
    
    
    bm->SetRest(offset);
    SaveBMInfo(bm, not_at_start, offset);
    return true;
}




int
ChoiceNode::EatsAtLeastHelper(int still_to_find,
                              int budget,
                              RegExpNode* ignore_this_node,
                              bool not_at_start)
{
    if (budget <= 0)
        return 0;

    int min = 100;
    size_t choice_count = alternatives().length();
    budget = (budget - 1) / choice_count;
    for (size_t i = 0; i < choice_count; i++) {
        RegExpNode* node = alternatives()[i].node();
        if (node == ignore_this_node) continue;
        int node_eats_at_least =
            node->EatsAtLeast(still_to_find, budget, not_at_start);
        if (node_eats_at_least < min)
            min = node_eats_at_least;
        if (min == 0)
            return 0;
    }
    return min;
}

int
ChoiceNode::EatsAtLeast(int still_to_find, int budget, bool not_at_start)
{
    return EatsAtLeastHelper(still_to_find,
                             budget,
                             nullptr,
                             not_at_start);
}

void
ChoiceNode::GetQuickCheckDetails(QuickCheckDetails* details,
                                 RegExpCompiler* compiler,
                                 int characters_filled_in,
                                 bool not_at_start)
{
    not_at_start = (not_at_start || not_at_start_);
    int choice_count = alternatives().length();
    JS_ASSERT(choice_count > 0);
    alternatives()[0].node()->GetQuickCheckDetails(details,
                                                   compiler,
                                                   characters_filled_in,
                                                   not_at_start);
    for (int i = 1; i < choice_count; i++) {
        QuickCheckDetails new_details(details->characters());
        RegExpNode* node = alternatives()[i].node();
        node->GetQuickCheckDetails(&new_details, compiler,
                                   characters_filled_in,
                                   not_at_start);
        
        details->Merge(&new_details, characters_filled_in);
    }
}

bool
ChoiceNode::FillInBMInfo(int offset,
                         int budget,
                         BoyerMooreLookahead* bm,
                         bool not_at_start)
{
    if (!bm->CheckOverRecursed())
        return false;

    const GuardedAlternativeVector &alts = alternatives();
    budget = (budget - 1) / alts.length();
    for (size_t i = 0; i < alts.length(); i++) {
        const GuardedAlternative& alt = alts[i];
        if (alt.guards() != nullptr && alt.guards()->length() != 0) {
            bm->SetRest(offset);  
            SaveBMInfo(bm, not_at_start, offset);
            return true;
        }
        if (!alt.node()->FillInBMInfo(offset, budget, bm, not_at_start))
            return false;
    }
    SaveBMInfo(bm, not_at_start, offset);
    return true;
}

RegExpNode*
ChoiceNode::FilterASCII(int depth, bool ignore_case)
{
    if (info()->replacement_calculated)
        return replacement();
    if (depth < 0)
        return this;
    if (info()->visited)
        return this;
    VisitMarker marker(info());
    int choice_count = alternatives().length();

    for (int i = 0; i < choice_count; i++) {
        const GuardedAlternative alternative = alternatives()[i];
        if (alternative.guards() != nullptr && alternative.guards()->length() != 0) {
            set_replacement(this);
            return this;
        }
    }

    int surviving = 0;
    RegExpNode* survivor = nullptr;
    for (int i = 0; i < choice_count; i++) {
        GuardedAlternative alternative = alternatives()[i];
        RegExpNode* replacement =
            alternative.node()->FilterASCII(depth - 1, ignore_case);
        JS_ASSERT(replacement != this);  
        if (replacement != nullptr) {
            alternatives()[i].set_node(replacement);
            surviving++;
            survivor = replacement;
        }
    }
    if (surviving < 2)
        return set_replacement(survivor);

    set_replacement(this);
    if (surviving == choice_count)
        return this;

    
    
    GuardedAlternativeVector new_alternatives(*alloc());
    new_alternatives.reserve(surviving);
    for (int i = 0; i < choice_count; i++) {
        RegExpNode* replacement =
            alternatives()[i].node()->FilterASCII(depth - 1, ignore_case);
        if (replacement != nullptr) {
            alternatives()[i].set_node(replacement);
            new_alternatives.append(alternatives()[i]);
        }
    }

    alternatives_.appendAll(new_alternatives);
    return this;
}




bool
NegativeLookaheadChoiceNode::FillInBMInfo(int offset,
                                          int budget,
                                          BoyerMooreLookahead* bm,
                                          bool not_at_start)
{
    if (!bm->CheckOverRecursed())
        return false;

    if (!alternatives()[1].node()->FillInBMInfo(offset, budget - 1, bm, not_at_start))
        return false;
    if (offset == 0)
        set_bm_info(not_at_start, bm);
    return true;
}

int
NegativeLookaheadChoiceNode::EatsAtLeast(int still_to_find, int budget, bool not_at_start)
{
    if (budget <= 0)
        return 0;

    
    
    RegExpNode* node = alternatives()[1].node();
    return node->EatsAtLeast(still_to_find, budget - 1, not_at_start);
}

void
NegativeLookaheadChoiceNode::GetQuickCheckDetails(QuickCheckDetails* details,
                                                  RegExpCompiler* compiler,
                                                  int filled_in,
                                                  bool not_at_start)
{
    
    
    RegExpNode* node = alternatives()[1].node();
    return node->GetQuickCheckDetails(details, compiler, filled_in, not_at_start);
}

RegExpNode *
NegativeLookaheadChoiceNode::FilterASCII(int depth, bool ignore_case)
{
    if (info()->replacement_calculated)
        return replacement();
    if (depth < 0)
        return this;
    if (info()->visited)
        return this;

    VisitMarker marker(info());

    
    
    RegExpNode* node = alternatives()[1].node();
    RegExpNode* replacement = node->FilterASCII(depth - 1, ignore_case);

    if (replacement == nullptr)
        return set_replacement(nullptr);
    alternatives()[1].set_node(replacement);

    RegExpNode* neg_node = alternatives()[0].node();
    RegExpNode* neg_replacement = neg_node->FilterASCII(depth - 1, ignore_case);

    
    
    if (neg_replacement == nullptr)
        return set_replacement(replacement);

    alternatives()[0].set_node(neg_replacement);
    return set_replacement(this);
}




void
GuardedAlternative::AddGuard(LifoAlloc *alloc, Guard *guard)
{
    if (guards_ == nullptr)
        guards_ = alloc->newInfallible<GuardVector>(*alloc);
    guards_->append(guard);
}

void
LoopChoiceNode::AddLoopAlternative(GuardedAlternative alt)
{
    JS_ASSERT(loop_node_ == nullptr);
    AddAlternative(alt);
    loop_node_ = alt.node();
}


void
LoopChoiceNode::AddContinueAlternative(GuardedAlternative alt)
{
    JS_ASSERT(continue_node_ == nullptr);
    AddAlternative(alt);
    continue_node_ = alt.node();
}

int
LoopChoiceNode::EatsAtLeast(int still_to_find,  int budget, bool not_at_start)
{
    return EatsAtLeastHelper(still_to_find,
                             budget - 1,
                             loop_node_,
                             not_at_start);
}

void
LoopChoiceNode::GetQuickCheckDetails(QuickCheckDetails* details,
                                     RegExpCompiler* compiler,
                                     int characters_filled_in,
                                     bool not_at_start)
{
    if (body_can_be_zero_length_ || info()->visited)
        return;
    VisitMarker marker(info());
    return ChoiceNode::GetQuickCheckDetails(details,
                                            compiler,
                                            characters_filled_in,
                                            not_at_start);
}

bool
LoopChoiceNode::FillInBMInfo(int offset,
                             int budget,
                             BoyerMooreLookahead* bm,
                             bool not_at_start)
{
    if (body_can_be_zero_length_ || budget <= 0) {
        bm->SetRest(offset);
        SaveBMInfo(bm, not_at_start, offset);
        return true;
    }
    if (!ChoiceNode::FillInBMInfo(offset, budget - 1, bm, not_at_start))
        return false;
    SaveBMInfo(bm, not_at_start, offset);
    return true;
}

RegExpNode *
LoopChoiceNode::FilterASCII(int depth, bool ignore_case)
{
    if (info()->replacement_calculated)
        return replacement();
    if (depth < 0)
        return this;
    if (info()->visited)
        return this;

    {
        VisitMarker marker(info());

        RegExpNode* continue_replacement =
            continue_node_->FilterASCII(depth - 1, ignore_case);

        
        
        if (continue_replacement == nullptr)
            return set_replacement(nullptr);
    }

    return ChoiceNode::FilterASCII(depth - 1, ignore_case);
}




void
Analysis::EnsureAnalyzed(RegExpNode* that)
{
    JS_CHECK_RECURSION(cx, fail("Stack overflow"); return);

    if (that->info()->been_analyzed || that->info()->being_analyzed)
        return;
    that->info()->being_analyzed = true;
    that->Accept(this);
    that->info()->being_analyzed = false;
    that->info()->been_analyzed = true;
}

void
Analysis::VisitEnd(EndNode* that)
{
    
}

void
Analysis::VisitText(TextNode* that)
{
    if (ignore_case_)
        that->MakeCaseIndependent(is_ascii_);
    EnsureAnalyzed(that->on_success());
    if (!has_failed()) {
        that->CalculateOffsets();
    }
}

void
Analysis::VisitAction(ActionNode* that)
{
    RegExpNode* target = that->on_success();
    EnsureAnalyzed(target);

    if (!has_failed()) {
        
        
        that->info()->AddFromFollowing(target->info());
    }
}

void
Analysis::VisitChoice(ChoiceNode* that)
{
    NodeInfo* info = that->info();

    for (size_t i = 0; i < that->alternatives().length(); i++) {
        RegExpNode* node = that->alternatives()[i].node();
        EnsureAnalyzed(node);
        if (has_failed()) return;

        
        
        info->AddFromFollowing(node->info());
    }
}

void
Analysis::VisitLoopChoice(LoopChoiceNode* that)
{
    NodeInfo* info = that->info();
    for (size_t i = 0; i < that->alternatives().length(); i++) {
        RegExpNode* node = that->alternatives()[i].node();
        if (node != that->loop_node()) {
            EnsureAnalyzed(node);
            if (has_failed()) return;
            info->AddFromFollowing(node->info());
        }
    }

    
    
    EnsureAnalyzed(that->loop_node());
    if (!has_failed())
        info->AddFromFollowing(that->loop_node()->info());
}

void
Analysis::VisitBackReference(BackReferenceNode* that)
{
    EnsureAnalyzed(that->on_success());
}

void
Analysis::VisitAssertion(AssertionNode* that)
{
    EnsureAnalyzed(that->on_success());
}






















































































































































 TextElement
TextElement::Atom(RegExpAtom* atom)
{
    return TextElement(ATOM, atom);
}

 TextElement
TextElement::CharClass(RegExpCharacterClass* char_class)
{
    return TextElement(CHAR_CLASS, char_class);
}

int
TextElement::length() const
{
    switch (text_type()) {
      case ATOM:
        return atom()->length();
      case CHAR_CLASS:
        return 1;
    }
    MOZ_CRASH("Bad text type");
}

class FrequencyCollator
{
  public:
    FrequencyCollator() : total_samples_(0) {
        for (int i = 0; i < RegExpMacroAssembler::kTableSize; i++) {
            frequencies_[i] = CharacterFrequency(i);
        }
    }

    void CountCharacter(int character) {
        int index = (character & RegExpMacroAssembler::kTableMask);
        frequencies_[index].Increment();
        total_samples_++;
    }

    
    
    int Frequency(int in_character) {
        JS_ASSERT((in_character & RegExpMacroAssembler::kTableMask) == in_character);
        if (total_samples_ < 1) return 1;  
        int freq_in_per128 =
            (frequencies_[in_character].counter() * 128) / total_samples_;
        return freq_in_per128;
    }

  private:
    class CharacterFrequency {
      public:
        CharacterFrequency() : counter_(0), character_(-1) { }
        explicit CharacterFrequency(int character)
          : counter_(0), character_(character)
        {}

        void Increment() { counter_++; }
        int counter() { return counter_; }
        int character() { return character_; }

     private:
        int counter_;
        int character_;
    };

  private:
    CharacterFrequency frequencies_[RegExpMacroAssembler::kTableSize];
    int total_samples_;
};

class irregexp::RegExpCompiler
{
  public:
    RegExpCompiler(JSContext *cx, LifoAlloc *alloc, int capture_count,
                   bool ignore_case, bool is_ascii, bool match_only);

    int AllocateRegister() {
        if (next_register_ >= RegExpMacroAssembler::kMaxRegister) {
            reg_exp_too_big_ = true;
            return next_register_;
        }
        return next_register_++;
    }

    RegExpCode Assemble(JSContext *cx,
                        RegExpMacroAssembler *assembler,
                        RegExpNode *start,
                        int capture_count);

    inline void AddWork(RegExpNode* node) {
        if (!work_list_.append(node))
            CrashAtUnhandlableOOM("AddWork");
    }

    static const int kImplementationOffset = 0;
    static const int kNumberOfRegistersOffset = 0;
    static const int kCodeOffset = 1;

    RegExpMacroAssembler* macro_assembler() { return macro_assembler_; }
    EndNode* accept() { return accept_; }

    static const int kMaxRecursion = 100;
    inline int recursion_depth() { return recursion_depth_; }
    inline void IncrementRecursionDepth() { recursion_depth_++; }
    inline void DecrementRecursionDepth() { recursion_depth_--; }

    void SetRegExpTooBig() { reg_exp_too_big_ = true; }

    inline bool ignore_case() { return ignore_case_; }
    inline bool ascii() { return ascii_; }
    FrequencyCollator* frequency_collator() { return &frequency_collator_; }

    int current_expansion_factor() { return current_expansion_factor_; }
    void set_current_expansion_factor(int value) {
        current_expansion_factor_ = value;
    }

    JSContext *cx() const { return cx_; }
    LifoAlloc *alloc() const { return alloc_; }

    static const int kNoRegister = -1;

  private:
    EndNode* accept_;
    int next_register_;
    Vector<RegExpNode *, 4, SystemAllocPolicy> work_list_;
    int recursion_depth_;
    RegExpMacroAssembler* macro_assembler_;
    bool ignore_case_;
    bool ascii_;
    bool match_only_;
    bool reg_exp_too_big_;
    int current_expansion_factor_;
    FrequencyCollator frequency_collator_;
    JSContext *cx_;
    LifoAlloc *alloc_;
};

class RecursionCheck
{
  public:
    explicit RecursionCheck(RegExpCompiler* compiler) : compiler_(compiler) {
        compiler->IncrementRecursionDepth();
    }
    ~RecursionCheck() { compiler_->DecrementRecursionDepth(); }

  private:
    RegExpCompiler* compiler_;
};



RegExpCompiler::RegExpCompiler(JSContext *cx, LifoAlloc *alloc, int capture_count,
                               bool ignore_case, bool ascii, bool match_only)
  : next_register_(2 * (capture_count + 1)),
    recursion_depth_(0),
    ignore_case_(ignore_case),
    ascii_(ascii),
    match_only_(match_only),
    reg_exp_too_big_(false),
    current_expansion_factor_(1),
    frequency_collator_(),
    cx_(cx),
    alloc_(alloc)
{
    accept_ = alloc->newInfallible<EndNode>(alloc, EndNode::ACCEPT);
    JS_ASSERT(next_register_ - 1 <= RegExpMacroAssembler::kMaxRegister);
}

RegExpCode
RegExpCompiler::Assemble(JSContext *cx,
                         RegExpMacroAssembler *assembler,
                         RegExpNode *start,
                         int capture_count)
{
    macro_assembler_ = assembler;
    macro_assembler_->set_slow_safe(false);

    jit::Label fail;
    macro_assembler_->PushBacktrack(&fail);
    Trace new_trace;
    start->Emit(this, &new_trace);
    macro_assembler_->BindBacktrack(&fail);
    macro_assembler_->Fail();

    while (!work_list_.empty())
        work_list_.popCopy()->Emit(this, &new_trace);

    RegExpCode code = macro_assembler_->GenerateCode(cx, match_only_);
    if (code.empty())
        return RegExpCode();

    if (reg_exp_too_big_) {
        JS_ReportError(cx, "regexp too big");
        code.destroy();
        return RegExpCode();
    }

    return code;
}

template <typename CharT>
static void
SampleChars(FrequencyCollator *collator, const CharT *chars, size_t length)
{
    
    static const int kSampleSize = 128;

    int chars_sampled = 0;
    int half_way = (int(length) - kSampleSize) / 2;
    for (size_t i = Max(0, half_way);
         i < length && chars_sampled < kSampleSize;
         i++, chars_sampled++)
    {
        collator->CountCharacter(chars[i]);
    }
}

static bool
IsNativeRegExpEnabled(JSContext *cx)
{
#ifdef JS_CODEGEN_NONE
    return false;
#else
    return cx->runtime()->options().nativeRegExp();
#endif
}

RegExpCode
irregexp::CompilePattern(JSContext *cx, RegExpShared *shared, RegExpCompileData *data,
                         HandleLinearString sample, bool is_global, bool ignore_case,
                         bool is_ascii, bool match_only)
{
    if ((data->capture_count + 1) * 2 - 1 > RegExpMacroAssembler::kMaxRegister) {
        JS_ReportError(cx, "regexp too big");
        return RegExpCode();
    }

    LifoAlloc &alloc = cx->tempLifoAlloc();
    RegExpCompiler compiler(cx, &alloc, data->capture_count, ignore_case, is_ascii, match_only);

    
    if (sample->hasLatin1Chars()) {
        JS::AutoCheckCannotGC nogc;
        SampleChars(compiler.frequency_collator(), sample->latin1Chars(nogc), sample->length());
    } else {
        JS::AutoCheckCannotGC nogc;
        SampleChars(compiler.frequency_collator(), sample->twoByteChars(nogc), sample->length());
    }

    
    RegExpNode* captured_body = RegExpCapture::ToNode(data->tree,
                                                      0,
                                                      &compiler,
                                                      compiler.accept());
    RegExpNode* node = captured_body;
    bool is_end_anchored = data->tree->IsAnchoredAtEnd();
    bool is_start_anchored = data->tree->IsAnchoredAtStart();
    int max_length = data->tree->max_match();
    if (!is_start_anchored) {
        
        
        RegExpNode* loop_node =
            RegExpQuantifier::ToNode(0,
                                     RegExpTree::kInfinity,
                                     false,
                                     alloc.newInfallible<RegExpCharacterClass>('*'),
                                     &compiler,
                                     captured_body,
                                     data->contains_anchor);

        if (data->contains_anchor) {
            
            
            ChoiceNode *first_step_node = alloc.newInfallible<ChoiceNode>(&alloc, 2);
            RegExpNode *char_class =
                alloc.newInfallible<TextNode>(alloc.newInfallible<RegExpCharacterClass>('*'), loop_node);
            first_step_node->AddAlternative(GuardedAlternative(captured_body));
            first_step_node->AddAlternative(GuardedAlternative(char_class));
            node = first_step_node;
        } else {
            node = loop_node;
        }
    }
    if (is_ascii) {
        node = node->FilterASCII(RegExpCompiler::kMaxRecursion, ignore_case);
        
        
        if (node != nullptr) {
            node = node->FilterASCII(RegExpCompiler::kMaxRecursion, ignore_case);
        }
    }

    if (node == nullptr)
        node = alloc.newInfallible<EndNode>(&alloc, EndNode::BACKTRACK);

    Analysis analysis(cx, ignore_case, is_ascii);
    analysis.EnsureAnalyzed(node);
    if (analysis.has_failed()) {
        JS_ReportError(cx, analysis.errorMessage());
        return RegExpCode();
    }

    Maybe<jit::IonContext> ctx;
    Maybe<NativeRegExpMacroAssembler> native_assembler;
    Maybe<InterpretedRegExpMacroAssembler> interpreted_assembler;

    RegExpMacroAssembler *assembler;
    if (IsNativeRegExpEnabled(cx)) {
        NativeRegExpMacroAssembler::Mode mode =
            is_ascii ? NativeRegExpMacroAssembler::ASCII
                     : NativeRegExpMacroAssembler::CHAR16;

        ctx.emplace(cx, (jit::TempAllocator *) nullptr);
        native_assembler.emplace(&alloc, shared, cx->runtime(), mode, (data->capture_count + 1) * 2);
        assembler = native_assembler.ptr();
    } else {
        interpreted_assembler.emplace(&alloc, shared, (data->capture_count + 1) * 2);
        assembler = interpreted_assembler.ptr();
    }

    
    
    static const int kMaxBacksearchLimit = 1024;
    if (is_end_anchored &&
        !is_start_anchored &&
        max_length < kMaxBacksearchLimit) {
        assembler->SetCurrentPositionFromEnd(max_length);
    }

    if (is_global) {
        assembler->set_global_mode((data->tree->min_match() > 0)
                                   ? RegExpMacroAssembler::GLOBAL_NO_ZERO_LENGTH_CHECK
                                   : RegExpMacroAssembler::GLOBAL);
    }

    return compiler.Assemble(cx, assembler, node, data->capture_count);
}

template <typename CharT>
RegExpRunStatus
irregexp::ExecuteCode(JSContext *cx, jit::JitCode *codeBlock, const CharT *chars, size_t start,
                      size_t length, MatchPairs *matches)
{
    typedef void (*RegExpCodeSignature)(InputOutputData *);

    InputOutputData data(chars, chars + length, start, matches);

    RegExpCodeSignature function = reinterpret_cast<RegExpCodeSignature>(codeBlock->raw());

    {
        JS::AutoSuppressGCAnalysis nogc;
        CALL_GENERATED_REGEXP(function, &data);
    }

    return (RegExpRunStatus) data.result;
}

template RegExpRunStatus
irregexp::ExecuteCode(JSContext *cx, jit::JitCode *codeBlock, const Latin1Char *chars, size_t start,
                      size_t length, MatchPairs *matches);

template RegExpRunStatus
irregexp::ExecuteCode(JSContext *cx, jit::JitCode *codeBlock, const char16_t *chars, size_t start,
                      size_t length, MatchPairs *matches);




RegExpNode *
RegExpAtom::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    TextElementVector *elms =
        compiler->alloc()->newInfallible<TextElementVector>(*compiler->alloc());
    elms->append(TextElement::Atom(this));
    return compiler->alloc()->newInfallible<TextNode>(elms, on_success);
}

RegExpNode *
RegExpText::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    return compiler->alloc()->newInfallible<TextNode>(&elements_, on_success);
}

RegExpNode *
RegExpCharacterClass::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    return compiler->alloc()->newInfallible<TextNode>(this, on_success);
}

RegExpNode *
RegExpDisjunction::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    const RegExpTreeVector &alternatives = this->alternatives();
    size_t length = alternatives.length();
    ChoiceNode* result = compiler->alloc()->newInfallible<ChoiceNode>(compiler->alloc(), length);
    for (size_t i = 0; i < length; i++) {
        GuardedAlternative alternative(alternatives[i]->ToNode(compiler, on_success));
        result->AddAlternative(alternative);
    }
    return result;
}

RegExpNode *
RegExpQuantifier::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    return ToNode(min(),
                  max(),
                  is_greedy(),
                  body(),
                  compiler,
                  on_success);
}



class RegExpExpansionLimiter
{
  public:
    static const int kMaxExpansionFactor = 6;
    RegExpExpansionLimiter(RegExpCompiler* compiler, int factor)
      : compiler_(compiler),
        saved_expansion_factor_(compiler->current_expansion_factor()),
        ok_to_expand_(saved_expansion_factor_ <= kMaxExpansionFactor)
    {
        JS_ASSERT(factor > 0);
        if (ok_to_expand_) {
            if (factor > kMaxExpansionFactor) {
                
                ok_to_expand_ = false;
                compiler->set_current_expansion_factor(kMaxExpansionFactor + 1);
            } else {
                int new_factor = saved_expansion_factor_ * factor;
                ok_to_expand_ = (new_factor <= kMaxExpansionFactor);
                compiler->set_current_expansion_factor(new_factor);
            }
        }
    }

    ~RegExpExpansionLimiter() {
        compiler_->set_current_expansion_factor(saved_expansion_factor_);
    }

    bool ok_to_expand() { return ok_to_expand_; }

  private:
    RegExpCompiler* compiler_;
    int saved_expansion_factor_;
    bool ok_to_expand_;
};

 RegExpNode *
RegExpQuantifier::ToNode(int min,
                         int max,
                         bool is_greedy,
                         RegExpTree* body,
                         RegExpCompiler* compiler,
                         RegExpNode* on_success,
                         bool not_at_start )
{
    
    
    
    
    
    
    
    
    
    

    
    
    
    

    
    
    
    
    static const int kMaxUnrolledMinMatches = 3;  
    static const int kMaxUnrolledMaxMatches = 3;  

    if (max == 0)
        return on_success;  

    bool body_can_be_empty = (body->min_match() == 0);
    int body_start_reg = RegExpCompiler::kNoRegister;
    Interval capture_registers = body->CaptureRegisters();
    bool needs_capture_clearing = !capture_registers.is_empty();
    LifoAlloc *alloc = compiler->alloc();

    if (body_can_be_empty) {
        body_start_reg = compiler->AllocateRegister();
    } else if (!needs_capture_clearing) {
        
        
        {
            RegExpExpansionLimiter limiter(compiler, min + ((max != min) ? 1 : 0));
            if (min > 0 && min <= kMaxUnrolledMinMatches && limiter.ok_to_expand()) {
                int new_max = (max == kInfinity) ? max : max - min;
                
                
                RegExpNode* answer = ToNode(0, new_max, is_greedy, body, compiler, on_success, true);
                
                
                
                for (int i = 0; i < min; i++)
                    answer = body->ToNode(compiler, answer);
                return answer;
            }
        }
        if (max <= kMaxUnrolledMaxMatches && min == 0) {
            JS_ASSERT(max > 0);  
            RegExpExpansionLimiter limiter(compiler, max);
            if (limiter.ok_to_expand()) {
                
                RegExpNode* answer = on_success;
                for (int i = 0; i < max; i++) {
                    ChoiceNode* alternation = alloc->newInfallible<ChoiceNode>(alloc, 2);
                    if (is_greedy) {
                        alternation->AddAlternative(GuardedAlternative(body->ToNode(compiler, answer)));
                        alternation->AddAlternative(GuardedAlternative(on_success));
                    } else {
                        alternation->AddAlternative(GuardedAlternative(on_success));
                        alternation->AddAlternative(GuardedAlternative(body->ToNode(compiler, answer)));
                    }
                    answer = alternation;
                    if (not_at_start) alternation->set_not_at_start();
                }
                return answer;
            }
        }
    }
    bool has_min = min > 0;
    bool has_max = max < RegExpTree::kInfinity;
    bool needs_counter = has_min || has_max;
    int reg_ctr = needs_counter
        ? compiler->AllocateRegister()
        : RegExpCompiler::kNoRegister;
    LoopChoiceNode* center = alloc->newInfallible<LoopChoiceNode>(alloc, body->min_match() == 0);
    if (not_at_start)
        center->set_not_at_start();
    RegExpNode* loop_return = needs_counter
        ? static_cast<RegExpNode*>(ActionNode::IncrementRegister(reg_ctr, center))
        : static_cast<RegExpNode*>(center);
    if (body_can_be_empty) {
        
        
        loop_return = ActionNode::EmptyMatchCheck(body_start_reg,
                                                  reg_ctr,
                                                  min,
                                                  loop_return);
    }
    RegExpNode* body_node = body->ToNode(compiler, loop_return);
    if (body_can_be_empty) {
        
        
        body_node = ActionNode::StorePosition(body_start_reg, false, body_node);
    }
    if (needs_capture_clearing) {
        
        body_node = ActionNode::ClearCaptures(capture_registers, body_node);
    }
    GuardedAlternative body_alt(body_node);
    if (has_max) {
        Guard* body_guard = alloc->newInfallible<Guard>(reg_ctr, Guard::LT, max);
        body_alt.AddGuard(alloc, body_guard);
    }
    GuardedAlternative rest_alt(on_success);
    if (has_min) {
        Guard* rest_guard = alloc->newInfallible<Guard>(reg_ctr, Guard::GEQ, min);
        rest_alt.AddGuard(alloc, rest_guard);
    }
    if (is_greedy) {
        center->AddLoopAlternative(body_alt);
        center->AddContinueAlternative(rest_alt);
    } else {
        center->AddContinueAlternative(rest_alt);
        center->AddLoopAlternative(body_alt);
    }
    if (needs_counter)
        return ActionNode::SetRegister(reg_ctr, 0, center);
    return center;
}

RegExpNode*
RegExpAssertion::ToNode(RegExpCompiler* compiler,
                        RegExpNode* on_success)
{
    NodeInfo info;
    LifoAlloc *alloc = compiler->alloc();

    switch (assertion_type()) {
      case START_OF_LINE:
        return AssertionNode::AfterNewline(on_success);
      case START_OF_INPUT:
        return AssertionNode::AtStart(on_success);
      case BOUNDARY:
        return AssertionNode::AtBoundary(on_success);
      case NON_BOUNDARY:
        return AssertionNode::AtNonBoundary(on_success);
      case END_OF_INPUT:
        return AssertionNode::AtEnd(on_success);
      case END_OF_LINE: {
        
        
        
        int stack_pointer_register = compiler->AllocateRegister();
        int position_register = compiler->AllocateRegister();
        
        ChoiceNode* result = alloc->newInfallible<ChoiceNode>(alloc, 2);
        
        CharacterRangeVector *newline_ranges = alloc->newInfallible<CharacterRangeVector>(*alloc);
        CharacterRange::AddClassEscape(alloc, 'n', newline_ranges);
        RegExpCharacterClass* newline_atom = alloc->newInfallible<RegExpCharacterClass>('n');
        TextNode* newline_matcher =
            alloc->newInfallible<TextNode>(newline_atom,
                ActionNode::PositiveSubmatchSuccess(stack_pointer_register,
                                                    position_register,
                                                    0,  
                                                    -1,  
                                                    on_success));
        
        RegExpNode* end_of_line =
            ActionNode::BeginSubmatch(stack_pointer_register, position_register, newline_matcher);

        
        GuardedAlternative eol_alternative(end_of_line);
        result->AddAlternative(eol_alternative);
        GuardedAlternative end_alternative(AssertionNode::AtEnd(on_success));
        result->AddAlternative(end_alternative);
        return result;
      }
      default:
        MOZ_CRASH("Bad assertion type");
    }
    return on_success;
}

RegExpNode *
RegExpBackReference::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    return compiler->alloc()->newInfallible<BackReferenceNode>(RegExpCapture::StartRegister(index()),
                                                               RegExpCapture::EndRegister(index()),
                                                               on_success);
}

RegExpNode *
RegExpEmpty::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    return on_success;
}

RegExpNode *
RegExpLookahead::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    int stack_pointer_register = compiler->AllocateRegister();
    int position_register = compiler->AllocateRegister();

    const int registers_per_capture = 2;
    const int register_of_first_capture = 2;
    int register_count = capture_count_ * registers_per_capture;
    int register_start =
        register_of_first_capture + capture_from_ * registers_per_capture;

    if (is_positive()) {
        RegExpNode *bodyNode =
            body()->ToNode(compiler,
                           ActionNode::PositiveSubmatchSuccess(stack_pointer_register,
                                                               position_register,
                                                               register_count,
                                                               register_start,
                                                               on_success));
        return ActionNode::BeginSubmatch(stack_pointer_register,
                                         position_register,
                                         bodyNode);
    }

    
    
    
    
    
    
    
    
    
    
    LifoAlloc *alloc = compiler->alloc();

    RegExpNode *success =
        alloc->newInfallible<NegativeSubmatchSuccess>(alloc,
                                                      stack_pointer_register,
                                                      position_register,
                                                      register_count,
                                                      register_start);
    GuardedAlternative body_alt(body()->ToNode(compiler, success));

    ChoiceNode *choice_node =
        alloc->newInfallible<NegativeLookaheadChoiceNode>(alloc, body_alt, GuardedAlternative(on_success));

    return ActionNode::BeginSubmatch(stack_pointer_register,
                                     position_register,
                                     choice_node);
}

RegExpNode *
RegExpCapture::ToNode(RegExpCompiler *compiler, RegExpNode* on_success)
{
    return ToNode(body(), index(), compiler, on_success);
}

 RegExpNode *
RegExpCapture::ToNode(RegExpTree* body,
                      int index,
                      RegExpCompiler* compiler,
                      RegExpNode* on_success)
{
    int start_reg = RegExpCapture::StartRegister(index);
    int end_reg = RegExpCapture::EndRegister(index);
    RegExpNode* store_end = ActionNode::StorePosition(end_reg, true, on_success);
    RegExpNode* body_node = body->ToNode(compiler, store_end);
    return ActionNode::StorePosition(start_reg, true, body_node);
}

RegExpNode*
RegExpAlternative::ToNode(RegExpCompiler* compiler, RegExpNode* on_success)
{
    const RegExpTreeVector &children = nodes();
    RegExpNode *current = on_success;
    for (int i = children.length() - 1; i >= 0; i--)
        current = children[i]->ToNode(compiler, current);
    return current;
}




ContainedInLattice
irregexp::AddRange(ContainedInLattice containment,
                   const int* ranges,
                   int ranges_length,
                   Interval new_range)
{
    JS_ASSERT((ranges_length & 1) == 1);
    JS_ASSERT(ranges[ranges_length - 1] == kMaxUtf16CodeUnit + 1);
    if (containment == kLatticeUnknown) return containment;
    bool inside = false;
    int last = 0;
    for (int i = 0; i < ranges_length; inside = !inside, last = ranges[i], i++) {
        
        
        if (ranges[i] <= new_range.from())
            continue;

        
        
        if (last <= new_range.from() && new_range.to() < ranges[i])
            return Combine(containment, inside ? kLatticeIn : kLatticeOut);

        return kLatticeUnknown;
    }
    return containment;
}

void
BoyerMoorePositionInfo::Set(int character)
{
    SetInterval(Interval(character, character));
}

void
BoyerMoorePositionInfo::SetInterval(const Interval& interval)
{
    s_ = AddRange(s_, kSpaceRanges, kSpaceRangeCount, interval);
    w_ = AddRange(w_, kWordRanges, kWordRangeCount, interval);
    d_ = AddRange(d_, kDigitRanges, kDigitRangeCount, interval);
    surrogate_ =
        AddRange(surrogate_, kSurrogateRanges, kSurrogateRangeCount, interval);
    if (interval.to() - interval.from() >= kMapSize - 1) {
        if (map_count_ != kMapSize) {
            map_count_ = kMapSize;
            for (int i = 0; i < kMapSize; i++)
                map_[i] = true;
        }
        return;
    }
    for (int i = interval.from(); i <= interval.to(); i++) {
        int mod_character = (i & kMask);
        if (!map_[mod_character]) {
            map_count_++;
            map_[mod_character] = true;
        }
        if (map_count_ == kMapSize)
            return;
    }
}

void
BoyerMoorePositionInfo::SetAll()
{
    s_ = w_ = d_ = kLatticeUnknown;
    if (map_count_ != kMapSize) {
        map_count_ = kMapSize;
        for (int i = 0; i < kMapSize; i++)
            map_[i] = true;
    }
}

BoyerMooreLookahead::BoyerMooreLookahead(LifoAlloc *alloc, size_t length, RegExpCompiler* compiler)
  : length_(length), compiler_(compiler), bitmaps_(*alloc)
{
    max_char_ = MaximumCharacter(compiler->ascii());

    bitmaps_.reserve(length);
    for (size_t i = 0; i < length; i++)
        bitmaps_.append(alloc->newInfallible<BoyerMoorePositionInfo>(alloc));
}




bool BoyerMooreLookahead::FindWorthwhileInterval(int* from, int* to) {
  int biggest_points = 0;
  
  
  const int kMaxMax = 32;
  for (int max_number_of_chars = 4;
       max_number_of_chars < kMaxMax;
       max_number_of_chars *= 2) {
    biggest_points =
        FindBestInterval(max_number_of_chars, biggest_points, from, to);
  }
  if (biggest_points == 0) return false;
  return true;
}







int
BoyerMooreLookahead::FindBestInterval(int max_number_of_chars, int old_biggest_points,
                                      int* from, int* to)
{
    int biggest_points = old_biggest_points;
    static const int kSize = RegExpMacroAssembler::kTableSize;
    for (int i = 0; i < length_; ) {
        while (i < length_ && Count(i) > max_number_of_chars) i++;
        if (i == length_) break;
        int remembered_from = i;
        bool union_map[kSize];
        for (int j = 0; j < kSize; j++) union_map[j] = false;
        while (i < length_ && Count(i) <= max_number_of_chars) {
            BoyerMoorePositionInfo* map = bitmaps_[i];
            for (int j = 0; j < kSize; j++) union_map[j] |= map->at(j);
            i++;
        }
        int frequency = 0;
        for (int j = 0; j < kSize; j++) {
            if (union_map[j]) {
                
                
                
                
                
                frequency += compiler_->frequency_collator()->Frequency(j) + 1;
            }
        }
        
        
        
        
        
        bool in_quickcheck_range = ((i - remembered_from < 4) ||
                                    (compiler_->ascii() ? remembered_from <= 4 : remembered_from <= 2));
        
        
        int probability = (in_quickcheck_range ? kSize / 2 : kSize) - frequency;
        int points = (i - remembered_from) * probability;
        if (points > biggest_points) {
            *from = remembered_from;
            *to = i - 1;
            biggest_points = points;
        }
    }
    return biggest_points;
}






int BoyerMooreLookahead::GetSkipTable(int min_lookahead,
                                      int max_lookahead,
                                      uint8_t *boolean_skip_table)
{
    const int kSize = RegExpMacroAssembler::kTableSize;

    const int kSkipArrayEntry = 0;
    const int kDontSkipArrayEntry = 1;

    for (int i = 0; i < kSize; i++)
        boolean_skip_table[i] = kSkipArrayEntry;
    int skip = max_lookahead + 1 - min_lookahead;

    for (int i = max_lookahead; i >= min_lookahead; i--) {
        BoyerMoorePositionInfo* map = bitmaps_[i];
        for (int j = 0; j < kSize; j++) {
            if (map->at(j))
                boolean_skip_table[j] = kDontSkipArrayEntry;
        }
    }

    return skip;
}


bool
BoyerMooreLookahead::EmitSkipInstructions(RegExpMacroAssembler* masm)
{
    const int kSize = RegExpMacroAssembler::kTableSize;

    int min_lookahead = 0;
    int max_lookahead = 0;

    if (!FindWorthwhileInterval(&min_lookahead, &max_lookahead))
        return false;

    bool found_single_character = false;
    int single_character = 0;
    for (int i = max_lookahead; i >= min_lookahead; i--) {
        BoyerMoorePositionInfo* map = bitmaps_[i];
        if (map->map_count() > 1 ||
            (found_single_character && map->map_count() != 0)) {
            found_single_character = false;
            break;
        }
        for (int j = 0; j < kSize; j++) {
            if (map->at(j)) {
                found_single_character = true;
                single_character = j;
                break;
            }
        }
    }

    int lookahead_width = max_lookahead + 1 - min_lookahead;

    if (found_single_character && lookahead_width == 1 && max_lookahead < 3) {
        
        return false;
    }

    if (found_single_character) {
        jit::Label cont, again;
        masm->Bind(&again);
        masm->LoadCurrentCharacter(max_lookahead, &cont, true);
        if (max_char_ > kSize) {
            masm->CheckCharacterAfterAnd(single_character,
                                         RegExpMacroAssembler::kTableMask,
                                         &cont);
        } else {
            masm->CheckCharacter(single_character, &cont);
        }
        masm->AdvanceCurrentPosition(lookahead_width);
        masm->JumpOrBacktrack(&again);
        masm->Bind(&cont);
        return true;
    }

    uint8_t *boolean_skip_table = static_cast<uint8_t *>(js_malloc(kSize));
    if (!boolean_skip_table || !masm->shared->addTable(boolean_skip_table))
        CrashAtUnhandlableOOM("Table malloc");

    int skip_distance = GetSkipTable(min_lookahead, max_lookahead, boolean_skip_table);
    JS_ASSERT(skip_distance != 0);

    jit::Label cont, again;
    masm->Bind(&again);
    masm->LoadCurrentCharacter(max_lookahead, &cont, true);
    masm->CheckBitInTable(boolean_skip_table, &cont);
    masm->AdvanceCurrentPosition(skip_distance);
    masm->JumpOrBacktrack(&again);
    masm->Bind(&cont);

    return true;
}

bool
BoyerMooreLookahead::CheckOverRecursed()
{
    JS_CHECK_RECURSION(compiler()->cx(), compiler()->SetRegExpTooBig(); return false);
    return true;
}




bool Trace::DeferredAction::Mentions(int that)
{
    if (action_type() == ActionNode::CLEAR_CAPTURES) {
        Interval range = static_cast<DeferredClearCaptures*>(this)->range();
        return range.Contains(that);
    }
    return reg() == that;
}

bool Trace::mentions_reg(int reg)
{
    for (DeferredAction* action = actions_; action != nullptr; action = action->next()) {
        if (action->Mentions(reg))
            return true;
    }
    return false;
}

bool
Trace::GetStoredPosition(int reg, int* cp_offset)
{
    JS_ASSERT(0 == *cp_offset);
    for (DeferredAction* action = actions_; action != nullptr; action = action->next()) {
        if (action->Mentions(reg)) {
            if (action->action_type() == ActionNode::STORE_POSITION) {
                *cp_offset = static_cast<DeferredCapture*>(action)->cp_offset();
                return true;
            }
            return false;
        }
    }
    return false;
}

int
Trace::FindAffectedRegisters(LifoAlloc *alloc, OutSet* affected_registers)
{
    int max_register = RegExpCompiler::kNoRegister;
    for (DeferredAction* action = actions_; action != nullptr; action = action->next()) {
        if (action->action_type() == ActionNode::CLEAR_CAPTURES) {
            Interval range = static_cast<DeferredClearCaptures*>(action)->range();
            for (int i = range.from(); i <= range.to(); i++)
                affected_registers->Set(alloc, i);
            if (range.to() > max_register) max_register = range.to();
        } else {
            affected_registers->Set(alloc, action->reg());
            if (action->reg() > max_register) max_register = action->reg();
        }
    }
    return max_register;
}

void
Trace::RestoreAffectedRegisters(RegExpMacroAssembler* assembler,
                                int max_register,
                                OutSet& registers_to_pop,
                                OutSet& registers_to_clear)
{
    for (int reg = max_register; reg >= 0; reg--) {
        if (registers_to_pop.Get(reg)) assembler->PopRegister(reg);
        else if (registers_to_clear.Get(reg)) {
            int clear_to = reg;
            while (reg > 0 && registers_to_clear.Get(reg - 1))
                reg--;
            assembler->ClearRegisters(reg, clear_to);
        }
    }
}

enum DeferredActionUndoType {
    DEFER_IGNORE,
    DEFER_RESTORE,
    DEFER_CLEAR
};

void
Trace::PerformDeferredActions(LifoAlloc *alloc,
                              RegExpMacroAssembler* assembler,
                              int max_register,
                              OutSet& affected_registers,
                              OutSet* registers_to_pop,
                              OutSet* registers_to_clear)
{
    
    const int push_limit = (assembler->stack_limit_slack() + 1) / 2;

    
    int pushes = 0;

    for (int reg = 0; reg <= max_register; reg++) {
        if (!affected_registers.Get(reg))
            continue;

        
        
        
        DeferredActionUndoType undo_action = DEFER_IGNORE;

        int value = 0;
        bool absolute = false;
        bool clear = false;
        int store_position = -1;
        
        
        for (DeferredAction* action = actions_;
             action != nullptr;
             action = action->next()) {
            if (action->Mentions(reg)) {
                switch (action->action_type()) {
                  case ActionNode::SET_REGISTER: {
                    Trace::DeferredSetRegister* psr =
                        static_cast<Trace::DeferredSetRegister*>(action);
                    if (!absolute) {
                        value += psr->value();
                        absolute = true;
                    }
                    
                    
                    
                    
                    
                    undo_action = DEFER_RESTORE;
                    JS_ASSERT(store_position == -1);
                    JS_ASSERT(!clear);
                    break;
                  }
                  case ActionNode::INCREMENT_REGISTER:
                    if (!absolute) {
                        value++;
                    }
                    JS_ASSERT(store_position == -1);
                    JS_ASSERT(!clear);
                    undo_action = DEFER_RESTORE;
                    break;
                  case ActionNode::STORE_POSITION: {
                    Trace::DeferredCapture* pc =
                        static_cast<Trace::DeferredCapture*>(action);
                    if (!clear && store_position == -1) {
                        store_position = pc->cp_offset();
                    }

                    
                    
                    
                    if (reg <= 1) {
                        
                        
                        
                        
                        undo_action = DEFER_IGNORE;
                    } else {
                        undo_action = pc->is_capture() ? DEFER_CLEAR : DEFER_RESTORE;
                    }
                    JS_ASSERT(!absolute);
                    JS_ASSERT(value == 0);
                    break;
                  }
                  case ActionNode::CLEAR_CAPTURES: {
                    
                    
                    
                    if (store_position == -1) {
                        clear = true;
                    }
                    undo_action = DEFER_RESTORE;
                    JS_ASSERT(!absolute);
                    JS_ASSERT(value == 0);
                    break;
                  }
                  default:
                    MOZ_CRASH("Bad action");
                }
            }
        }
        
        if (undo_action == DEFER_RESTORE) {
            pushes++;
            RegExpMacroAssembler::StackCheckFlag stack_check =
                RegExpMacroAssembler::kNoStackLimitCheck;
            if (pushes == push_limit) {
                stack_check = RegExpMacroAssembler::kCheckStackLimit;
                pushes = 0;
            }

            assembler->PushRegister(reg, stack_check);
            registers_to_pop->Set(alloc, reg);
        } else if (undo_action == DEFER_CLEAR) {
            registers_to_clear->Set(alloc, reg);
        }
        
        
        if (store_position != -1) {
            assembler->WriteCurrentPositionToRegister(reg, store_position);
        } else if (clear) {
            assembler->ClearRegisters(reg, reg);
        } else if (absolute) {
            assembler->SetRegister(reg, value);
        } else if (value != 0) {
            assembler->AdvanceRegister(reg, value);
        }
    }
}




void Trace::Flush(RegExpCompiler* compiler, RegExpNode* successor)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();

    JS_ASSERT(!is_trivial());

    if (actions_ == nullptr && backtrack() == nullptr) {
        
        
        
        if (cp_offset_ != 0) assembler->AdvanceCurrentPosition(cp_offset_);
        
        Trace new_state;
        successor->Emit(compiler, &new_state);
        return;
    }

    
    OutSet affected_registers;

    if (backtrack() != nullptr) {
        
        
        
        assembler->PushCurrentPosition();
    }

    int max_register = FindAffectedRegisters(compiler->alloc(), &affected_registers);
    OutSet registers_to_pop;
    OutSet registers_to_clear;
    PerformDeferredActions(compiler->alloc(),
                           assembler,
                           max_register,
                           affected_registers,
                           &registers_to_pop,
                           &registers_to_clear);
    if (cp_offset_ != 0)
        assembler->AdvanceCurrentPosition(cp_offset_);

    
    jit::Label undo;
    assembler->PushBacktrack(&undo);
    Trace new_state;
    successor->Emit(compiler, &new_state);

    
    assembler->BindBacktrack(&undo);
    RestoreAffectedRegisters(assembler,
                             max_register,
                             registers_to_pop,
                             registers_to_clear);
    if (backtrack() == nullptr) {
        assembler->Backtrack();
    } else {
        assembler->PopCurrentPosition();
        assembler->JumpOrBacktrack(backtrack());
    }
}

void
Trace::InvalidateCurrentCharacter()
{
    characters_preloaded_ = 0;
}

void
Trace::AdvanceCurrentPositionInTrace(int by, RegExpCompiler* compiler)
{
    JS_ASSERT(by > 0);
    
    
    
    characters_preloaded_ = 0;
    
    
    
    quick_check_performed_.Advance(by, compiler->ascii());
    cp_offset_ += by;
    if (cp_offset_ > RegExpMacroAssembler::kMaxCPOffset) {
        compiler->SetRegExpTooBig();
        cp_offset_ = 0;
    }
    bound_checked_up_to_ = Max(0, bound_checked_up_to_ - by);
}

void
OutSet::Set(LifoAlloc *alloc, unsigned value)
{
    if (value < kFirstLimit) {
        first_ |= (1 << value);
    } else {
        if (remaining_ == nullptr)
            remaining_ = alloc->newInfallible<RemainingVector>(*alloc);

        for (size_t i = 0; i < remaining().length(); i++) {
            if (remaining()[i] == value)
                return;
        }
        remaining().append(value);
    }
}

bool
OutSet::Get(unsigned value)
{
    if (value < kFirstLimit)
        return (first_ & (1 << value)) != 0;
    if (remaining_ == nullptr)
        return false;
    for (size_t i = 0; i < remaining().length(); i++) {
        if (remaining()[i] == value)
            return true;
    }
    return false;
}




void
NegativeSubmatchSuccess::Emit(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();

    

    if (!label()->bound()) {
        
        
        assembler->Bind(label());
    }

    
    
    assembler->ReadCurrentPositionFromRegister(current_position_register_);
    assembler->ReadBacktrackStackPointerFromRegister(stack_pointer_register_);

    if (clear_capture_count_ > 0) {
        
        
        int clear_capture_end = clear_capture_start_ + clear_capture_count_ - 1;
        assembler->ClearRegisters(clear_capture_start_, clear_capture_end);
    }

    
    
    assembler->Backtrack();
}

void
EndNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    if (!trace->is_trivial()) {
        trace->Flush(compiler, this);
        return;
    }
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    if (!label()->bound()) {
        assembler->Bind(label());
    }
    switch (action_) {
    case ACCEPT:
        assembler->Succeed();
        return;
    case BACKTRACK:
        assembler->JumpOrBacktrack(trace->backtrack());
        return;
    case NEGATIVE_SUBMATCH_SUCCESS:
        
        MOZ_CRASH("Bad action: NEGATIVE_SUBMATCH_SUCCESS");
    }
    MOZ_CRASH("Bad action");
}



static void
EmitHat(RegExpCompiler* compiler, RegExpNode* on_success, Trace* trace)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();

    
    
    Trace new_trace(*trace);
    new_trace.InvalidateCurrentCharacter();

    jit::Label ok;
    if (new_trace.cp_offset() == 0) {
        
        
        assembler->CheckAtStart(&ok);
    }

    
    
    assembler->LoadCurrentCharacter(new_trace.cp_offset() -1, new_trace.backtrack(), false);

    if (!assembler->CheckSpecialCharacterClass('n', new_trace.backtrack())) {
        
        if (!compiler->ascii())
            assembler->CheckCharacterAfterAnd(0x2028, 0xfffe, &ok);
        assembler->CheckCharacter('\n', &ok);
        assembler->CheckNotCharacter('\r', new_trace.backtrack());
    }
    assembler->Bind(&ok);
    on_success->Emit(compiler, &new_trace);
}


static void
EmitWordCheck(RegExpMacroAssembler* assembler,
              jit::Label* word, jit::Label* non_word, bool fall_through_on_word)
{
    if (assembler->CheckSpecialCharacterClass(fall_through_on_word ? 'w' : 'W',
                                              fall_through_on_word ? non_word : word))
    {
        
        return;
    }

    assembler->CheckCharacterGT('z', non_word);
    assembler->CheckCharacterLT('0', non_word);
    assembler->CheckCharacterGT('a' - 1, word);
    assembler->CheckCharacterLT('9' + 1, word);
    assembler->CheckCharacterLT('A', non_word);
    assembler->CheckCharacterLT('Z' + 1, word);

    if (fall_through_on_word)
        assembler->CheckNotCharacter('_', non_word);
    else
        assembler->CheckCharacter('_', word);
}


void
AssertionNode::EmitBoundaryCheck(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    Trace::TriBool next_is_word_character = Trace::UNKNOWN;
    bool not_at_start = (trace->at_start() == Trace::FALSE_VALUE);
    BoyerMooreLookahead* lookahead = bm_info(not_at_start);
    if (lookahead == nullptr) {
        int eats_at_least =
            Min(kMaxLookaheadForBoyerMoore, EatsAtLeast(kMaxLookaheadForBoyerMoore,
                                                        kRecursionBudget,
                                                        not_at_start));
        if (eats_at_least >= 1) {
            BoyerMooreLookahead* bm =
                alloc()->newInfallible<BoyerMooreLookahead>(alloc(), eats_at_least, compiler);
            FillInBMInfo(0, kRecursionBudget, bm, not_at_start);
            if (bm->at(0)->is_non_word())
                next_is_word_character = Trace::FALSE_VALUE;
            if (bm->at(0)->is_word()) next_is_word_character = Trace::TRUE_VALUE;
        }
    } else {
        if (lookahead->at(0)->is_non_word())
            next_is_word_character = Trace::FALSE_VALUE;
        if (lookahead->at(0)->is_word())
            next_is_word_character = Trace::TRUE_VALUE;
    }
    bool at_boundary = (assertion_type_ == AssertionNode::AT_BOUNDARY);
    if (next_is_word_character == Trace::UNKNOWN) {
        jit::Label before_non_word;
        jit::Label before_word;
        if (trace->characters_preloaded() != 1) {
            assembler->LoadCurrentCharacter(trace->cp_offset(), &before_non_word);
        }
        
        EmitWordCheck(assembler, &before_word, &before_non_word, false);
        
        assembler->Bind(&before_non_word);
        jit::Label ok;
        BacktrackIfPrevious(compiler, trace, at_boundary ? kIsNonWord : kIsWord);
        assembler->JumpOrBacktrack(&ok);

        assembler->Bind(&before_word);
        BacktrackIfPrevious(compiler, trace, at_boundary ? kIsWord : kIsNonWord);
        assembler->Bind(&ok);
    } else if (next_is_word_character == Trace::TRUE_VALUE) {
        BacktrackIfPrevious(compiler, trace, at_boundary ? kIsWord : kIsNonWord);
    } else {
        JS_ASSERT(next_is_word_character == Trace::FALSE_VALUE);
        BacktrackIfPrevious(compiler, trace, at_boundary ? kIsNonWord : kIsWord);
    }
}

void
AssertionNode::BacktrackIfPrevious(RegExpCompiler* compiler,
                                   Trace* trace,
                                   AssertionNode::IfPrevious backtrack_if_previous)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    Trace new_trace(*trace);
    new_trace.InvalidateCurrentCharacter();

    jit::Label fall_through, dummy;

    jit::Label* non_word = backtrack_if_previous == kIsNonWord ? new_trace.backtrack() : &fall_through;
    jit::Label* word     = backtrack_if_previous == kIsNonWord ? &fall_through : new_trace.backtrack();

    if (new_trace.cp_offset() == 0) {
        
        
        assembler->CheckAtStart(non_word);
    }
    
    
    assembler->LoadCurrentCharacter(new_trace.cp_offset() - 1, &dummy, false);
    EmitWordCheck(assembler, word, non_word, backtrack_if_previous == kIsNonWord);

    assembler->Bind(&fall_through);
    on_success()->Emit(compiler, &new_trace);
}

void
AssertionNode::GetQuickCheckDetails(QuickCheckDetails* details,
                                    RegExpCompiler* compiler,
                                    int filled_in,
                                    bool not_at_start)
{
    if (assertion_type_ == AT_START && not_at_start) {
        details->set_cannot_match();
        return;
    }
    return on_success()->GetQuickCheckDetails(details, compiler, filled_in, not_at_start);
}

void
AssertionNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    switch (assertion_type_) {
      case AT_END: {
        jit::Label ok;
        assembler->CheckPosition(trace->cp_offset(), &ok);
        assembler->JumpOrBacktrack(trace->backtrack());
        assembler->Bind(&ok);
        break;
      }
      case AT_START: {
        if (trace->at_start() == Trace::FALSE_VALUE) {
            assembler->JumpOrBacktrack(trace->backtrack());
            return;
        }
        if (trace->at_start() == Trace::UNKNOWN) {
            assembler->CheckNotAtStart(trace->backtrack());
            Trace at_start_trace = *trace;
            at_start_trace.set_at_start(true);
            on_success()->Emit(compiler, &at_start_trace);
            return;
        }
      }
        break;
      case AFTER_NEWLINE:
        EmitHat(compiler, on_success(), trace);
        return;
      case AT_BOUNDARY:
      case AT_NON_BOUNDARY: {
        EmitBoundaryCheck(compiler, trace);
        return;
      }
    }
    on_success()->Emit(compiler, trace);
}

static bool
DeterminedAlready(QuickCheckDetails* quick_check, int offset)
{
    if (quick_check == nullptr)
        return false;
    if (offset >= quick_check->characters())
        return false;
    return quick_check->positions(offset)->determines_perfectly;
}

static void
UpdateBoundsCheck(int index, int* checked_up_to)
{
    if (index > *checked_up_to)
        *checked_up_to = index;
}

static void
EmitBoundaryTest(RegExpMacroAssembler* masm,
                 int border,
                 jit::Label* fall_through,
                 jit::Label* above_or_equal,
                 jit::Label* below)
{
    if (below != fall_through) {
        masm->CheckCharacterLT(border, below);
        if (above_or_equal != fall_through)
            masm->JumpOrBacktrack(above_or_equal);
    } else {
        masm->CheckCharacterGT(border - 1, above_or_equal);
    }
}

static void
EmitDoubleBoundaryTest(RegExpMacroAssembler* masm,
                       int first,
                       int last,
                       jit::Label* fall_through,
                       jit::Label* in_range,
                       jit::Label* out_of_range)
{
    if (in_range == fall_through) {
        if (first == last)
            masm->CheckNotCharacter(first, out_of_range);
        else
            masm->CheckCharacterNotInRange(first, last, out_of_range);
    } else {
        if (first == last)
            masm->CheckCharacter(first, in_range);
        else
            masm->CheckCharacterInRange(first, last, in_range);
        if (out_of_range != fall_through)
            masm->JumpOrBacktrack(out_of_range);
    }
}

typedef Vector<int, 4, LifoAllocPolicy<Infallible> > RangeBoundaryVector;



static void
EmitUseLookupTable(RegExpMacroAssembler* masm,
                   RangeBoundaryVector &ranges,
                   int start_index,
                   int end_index,
                   int min_char,
                   jit::Label* fall_through,
                   jit::Label* even_label,
                   jit::Label* odd_label)
{
    static const int kSize = RegExpMacroAssembler::kTableSize;
    static const int kMask = RegExpMacroAssembler::kTableMask;

    DebugOnly<int> base = (min_char & ~kMask);

    
    for (int i = start_index; i <= end_index; i++)
        JS_ASSERT((ranges[i] & ~kMask) == base);
    JS_ASSERT(start_index == 0 || (ranges[start_index - 1] & ~kMask) <= base);

    char templ[kSize];
    jit::Label* on_bit_set;
    jit::Label* on_bit_clear;
    int bit;
    if (even_label == fall_through) {
        on_bit_set = odd_label;
        on_bit_clear = even_label;
        bit = 1;
    } else {
        on_bit_set = even_label;
        on_bit_clear = odd_label;
        bit = 0;
    }
    for (int i = 0; i < (ranges[start_index] & kMask) && i < kSize; i++)
        templ[i] = bit;
    int j = 0;
    bit ^= 1;
    for (int i = start_index; i < end_index; i++) {
        for (j = (ranges[i] & kMask); j < (ranges[i + 1] & kMask); j++) {
            templ[j] = bit;
        }
        bit ^= 1;
    }
    for (int i = j; i < kSize; i++) {
        templ[i] = bit;
    }

    
    uint8_t *ba = static_cast<uint8_t *>(js_malloc(kSize));
    if (!ba || !masm->shared->addTable(ba))
        CrashAtUnhandlableOOM("Table malloc");

    for (int i = 0; i < kSize; i++)
        ba[i] = templ[i];

    masm->CheckBitInTable(ba, on_bit_set);
    if (on_bit_clear != fall_through)
        masm->JumpOrBacktrack(on_bit_clear);
}

static void
CutOutRange(RegExpMacroAssembler* masm,
            RangeBoundaryVector &ranges,
            int start_index,
            int end_index,
            int cut_index,
            jit::Label* even_label,
            jit::Label* odd_label)
{
    bool odd = (((cut_index - start_index) & 1) == 1);
    jit::Label* in_range_label = odd ? odd_label : even_label;
    jit::Label dummy;
    EmitDoubleBoundaryTest(masm,
                           ranges[cut_index],
                           ranges[cut_index + 1] - 1,
                           &dummy,
                           in_range_label,
                           &dummy);
    JS_ASSERT(!dummy.used());
    
    
    
    for (int j = cut_index; j > start_index; j--)
        ranges[j] = ranges[j - 1];
    for (int j = cut_index + 1; j < end_index; j++)
        ranges[j] = ranges[j + 1];
}



static void
SplitSearchSpace(RangeBoundaryVector &ranges,
                 int start_index,
                 int end_index,
                 int* new_start_index,
                 int* new_end_index,
                 int* border)
{
    static const int kSize = RegExpMacroAssembler::kTableSize;
    static const int kMask = RegExpMacroAssembler::kTableMask;

    int first = ranges[start_index];
    int last = ranges[end_index] - 1;

    *new_start_index = start_index;
    *border = (ranges[start_index] & ~kMask) + kSize;
    while (*new_start_index < end_index) {
        if (ranges[*new_start_index] > *border)
            break;
        (*new_start_index)++;
    }
    
    

    
    
    
    
    
    
    
    
    int binary_chop_index = (end_index + start_index) / 2;
    
    
    
    
    if (*border - 1 > (int) kMaxOneByteCharCode &&  
        end_index - start_index > (*new_start_index - start_index) * 2 &&
        last - first > kSize * 2 &&
        binary_chop_index > *new_start_index &&
        ranges[binary_chop_index] >= first + 2 * kSize)
    {
        int scan_forward_for_section_border = binary_chop_index;;
        int new_border = (ranges[binary_chop_index] | kMask) + 1;

        while (scan_forward_for_section_border < end_index) {
            if (ranges[scan_forward_for_section_border] > new_border) {
                *new_start_index = scan_forward_for_section_border;
                *border = new_border;
                break;
            }
            scan_forward_for_section_border++;
        }
    }

    JS_ASSERT(*new_start_index > start_index);
    *new_end_index = *new_start_index - 1;
    if (ranges[*new_end_index] == *border)
        (*new_end_index)--;
    if (*border >= ranges[end_index]) {
        *border = ranges[end_index];
        *new_start_index = end_index;  
        *new_end_index = end_index - 1;
    }
}







static void
GenerateBranches(RegExpMacroAssembler* masm,
                 RangeBoundaryVector &ranges,
                 int start_index,
                 int end_index,
                 char16_t min_char,
                 char16_t max_char,
                 jit::Label* fall_through,
                 jit::Label* even_label,
                 jit::Label* odd_label)
{
    int first = ranges[start_index];
    int last = ranges[end_index] - 1;

    JS_ASSERT(min_char < first);

    
    
    if (start_index == end_index) {
        EmitBoundaryTest(masm, first, fall_through, even_label, odd_label);
        return;
    }

    
    
    if (start_index + 1 == end_index) {
        EmitDoubleBoundaryTest(masm, first, last, fall_through, even_label, odd_label);
        return;
    }

    
    
    if (end_index - start_index <= 6) {
        
        
        static int kNoCutIndex = -1;
        int cut = kNoCutIndex;
        for (int i = start_index; i < end_index; i++) {
            if (ranges[i] == ranges[i + 1] - 1) {
                cut = i;
                break;
            }
        }
        if (cut == kNoCutIndex) cut = start_index;
        CutOutRange(masm, ranges, start_index, end_index, cut, even_label, odd_label);
        JS_ASSERT(end_index - start_index >= 2);
        GenerateBranches(masm,
                         ranges,
                         start_index + 1,
                         end_index - 1,
                         min_char,
                         max_char,
                         fall_through,
                         even_label,
                         odd_label);
        return;
    }

    
    
    static const int kBits = RegExpMacroAssembler::kTableSizeBits;

    if ((max_char >> kBits) == (min_char >> kBits)) {
        EmitUseLookupTable(masm,
                           ranges,
                           start_index,
                           end_index,
                           min_char,
                           fall_through,
                           even_label,
                           odd_label);
        return;
    }

    if ((min_char >> kBits) != (first >> kBits)) {
        masm->CheckCharacterLT(first, odd_label);
        GenerateBranches(masm,
                         ranges,
                         start_index + 1,
                         end_index,
                         first,
                         max_char,
                         fall_through,
                         odd_label,
                         even_label);
        return;
    }

    int new_start_index = 0;
    int new_end_index = 0;
    int border = 0;

    SplitSearchSpace(ranges,
                     start_index,
                     end_index,
                     &new_start_index,
                     &new_end_index,
                     &border);

    jit::Label handle_rest;
    jit::Label* above = &handle_rest;
    if (border == last + 1) {
        
        
        above = (end_index & 1) != (start_index & 1) ? odd_label : even_label;
        JS_ASSERT(new_end_index == end_index - 1);
    }

    JS_ASSERT(start_index <= new_end_index);
    JS_ASSERT(new_start_index <= end_index);
    JS_ASSERT(start_index < new_start_index);
    JS_ASSERT(new_end_index < end_index);
    JS_ASSERT(new_end_index + 1 == new_start_index ||
              (new_end_index + 2 == new_start_index &&
               border == ranges[new_end_index + 1]));
    JS_ASSERT(min_char < border - 1);
    JS_ASSERT(border < max_char);
    JS_ASSERT(ranges[new_end_index] < border);
    JS_ASSERT(border < ranges[new_start_index] ||
              (border == ranges[new_start_index] &&
               new_start_index == end_index &&
               new_end_index == end_index - 1 &&
               border == last + 1));
    JS_ASSERT(new_start_index == 0 || border >= ranges[new_start_index - 1]);

    masm->CheckCharacterGT(border - 1, above);
    jit::Label dummy;
    GenerateBranches(masm,
                     ranges,
                     start_index,
                     new_end_index,
                     min_char,
                     border - 1,
                     &dummy,
                     even_label,
                     odd_label);
    if (handle_rest.used()) {
        masm->Bind(&handle_rest);
        bool flip = (new_start_index & 1) != (start_index & 1);
        GenerateBranches(masm,
                         ranges,
                         new_start_index,
                         end_index,
                         border,
                         max_char,
                         &dummy,
                         flip ? odd_label : even_label,
                         flip ? even_label : odd_label);
    }
}

static void
EmitCharClass(LifoAlloc *alloc,
              RegExpMacroAssembler* macro_assembler,
              RegExpCharacterClass* cc,
              bool ascii,
              jit::Label* on_failure,
              int cp_offset,
              bool check_offset,
              bool preloaded)
{
    CharacterRangeVector &ranges = cc->ranges(alloc);
    if (!CharacterRange::IsCanonical(ranges)) {
        CharacterRange::Canonicalize(ranges);
    }

    int max_char = MaximumCharacter(ascii);
    int range_count = ranges.length();

    int last_valid_range = range_count - 1;
    while (last_valid_range >= 0) {
        CharacterRange& range = ranges[last_valid_range];
        if (range.from() <= max_char) {
            break;
        }
        last_valid_range--;
    }

    if (last_valid_range < 0) {
        if (!cc->is_negated()) {
            macro_assembler->JumpOrBacktrack(on_failure);
        }
        if (check_offset) {
            macro_assembler->CheckPosition(cp_offset, on_failure);
        }
        return;
    }

    if (last_valid_range == 0 &&
        ranges[0].IsEverything(max_char)) {
        if (cc->is_negated()) {
            macro_assembler->JumpOrBacktrack(on_failure);
        } else {
            
            if (check_offset) {
                macro_assembler->CheckPosition(cp_offset, on_failure);
            }
        }
        return;
    }
    if (last_valid_range == 0 &&
        !cc->is_negated() &&
        ranges[0].IsEverything(max_char)) {
        
        if (check_offset) {
            macro_assembler->CheckPosition(cp_offset, on_failure);
        }
        return;
    }

    if (!preloaded) {
        macro_assembler->LoadCurrentCharacter(cp_offset, on_failure, check_offset);
    }

    if (cc->is_standard(alloc) &&
        macro_assembler->CheckSpecialCharacterClass(cc->standard_type(),
                                                    on_failure)) {
        return;
    }

    
    
    
    
    
    
    RangeBoundaryVector *range_boundaries =
        alloc->newInfallible<RangeBoundaryVector>(*alloc);

    bool zeroth_entry_is_failure = !cc->is_negated();

    range_boundaries->reserve(last_valid_range);
    for (int i = 0; i <= last_valid_range; i++) {
        CharacterRange& range = ranges[i];
        if (range.from() == 0) {
            JS_ASSERT(i == 0);
            zeroth_entry_is_failure = !zeroth_entry_is_failure;
        } else {
            range_boundaries->append(range.from());
        }
        range_boundaries->append(range.to() + 1);
    }
    int end_index = range_boundaries->length() - 1;
    if ((*range_boundaries)[end_index] > max_char)
        end_index--;

    jit::Label fall_through;
    GenerateBranches(macro_assembler,
                     *range_boundaries,
                     0,  
                     end_index,
                     0,  
                     max_char,
                     &fall_through,
                     zeroth_entry_is_failure ? &fall_through : on_failure,
                     zeroth_entry_is_failure ? on_failure : &fall_through);
    macro_assembler->Bind(&fall_through);
}

typedef bool EmitCharacterFunction(RegExpCompiler* compiler,
                                   char16_t c,
                                   jit::Label* on_failure,
                                   int cp_offset,
                                   bool check,
                                   bool preloaded);

static inline bool
EmitSimpleCharacter(RegExpCompiler* compiler,
                    char16_t c,
                    jit::Label* on_failure,
                    int cp_offset,
                    bool check,
                    bool preloaded)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    bool bound_checked = false;
    if (!preloaded) {
        assembler->LoadCurrentCharacter(cp_offset, on_failure, check);
        bound_checked = true;
    }
    assembler->CheckNotCharacter(c, on_failure);
    return bound_checked;
}



static inline bool
EmitAtomNonLetter(RegExpCompiler* compiler,
                  char16_t c,
                  jit::Label* on_failure,
                  int cp_offset,
                  bool check,
                  bool preloaded)
{
    RegExpMacroAssembler* macro_assembler = compiler->macro_assembler();
    bool ascii = compiler->ascii();
    char16_t chars[kEcma262UnCanonicalizeMaxWidth];
    int length = GetCaseIndependentLetters(c, ascii, chars);
    if (length < 1) {
        
        
        return false;  
    }
    bool checked = false;
    
    if (length == 1) {
        if (ascii && c > kMaxOneByteCharCode) {
            
            return false;  
        }
        if (!preloaded) {
            macro_assembler->LoadCurrentCharacter(cp_offset, on_failure, check);
            checked = check;
        }
        macro_assembler->CheckNotCharacter(c, on_failure);
    }
    return checked;
}

static bool
ShortCutEmitCharacterPair(RegExpMacroAssembler* macro_assembler,
                          bool ascii,
                          char16_t c1,
                          char16_t c2,
                          jit::Label* on_failure)
{
    char16_t char_mask = MaximumCharacter(ascii);

    JS_ASSERT(c1 != c2);
    if (c1 > c2) {
        char16_t tmp = c1;
        c1 = c2;
        c2 = tmp;
    }

    char16_t exor = c1 ^ c2;
    
    if (((exor - 1) & exor) == 0) {
        
        char16_t mask = char_mask ^ exor;
        macro_assembler->CheckNotCharacterAfterAnd(c1, mask, on_failure);
        return true;
    }

    char16_t diff = c2 - c1;
    if (((diff - 1) & diff) == 0 && c1 >= diff) {
        
        
        
        
        char16_t mask = char_mask ^ diff;
        macro_assembler->CheckNotCharacterAfterMinusAnd(c1 - diff,
                                                        diff,
                                                        mask,
                                                        on_failure);
        return true;
    }
    return false;
}



static inline bool
EmitAtomLetter(RegExpCompiler* compiler,
               char16_t c,
               jit::Label* on_failure,
               int cp_offset,
               bool check,
               bool preloaded)
{
    RegExpMacroAssembler* macro_assembler = compiler->macro_assembler();
    bool ascii = compiler->ascii();
    char16_t chars[kEcma262UnCanonicalizeMaxWidth];
    int length = GetCaseIndependentLetters(c, ascii, chars);
    if (length <= 1) return false;
    
    
    if (!preloaded)
        macro_assembler->LoadCurrentCharacter(cp_offset, on_failure, check);
    jit::Label ok;
    JS_ASSERT(kEcma262UnCanonicalizeMaxWidth == 4);
    switch (length) {
      case 2: {
        if (ShortCutEmitCharacterPair(macro_assembler,
                                      ascii,
                                      chars[0],
                                      chars[1],
                                      on_failure)) {
        } else {
            macro_assembler->CheckCharacter(chars[0], &ok);
            macro_assembler->CheckNotCharacter(chars[1], on_failure);
            macro_assembler->Bind(&ok);
        }
        break;
      }
      case 4:
        macro_assembler->CheckCharacter(chars[3], &ok);
        
      case 3:
        macro_assembler->CheckCharacter(chars[0], &ok);
        macro_assembler->CheckCharacter(chars[1], &ok);
        macro_assembler->CheckNotCharacter(chars[2], on_failure);
        macro_assembler->Bind(&ok);
        break;
      default:
        MOZ_CRASH("Bad length");
    }
    return true;
}






























void
TextNode::TextEmitPass(RegExpCompiler* compiler,
                       TextEmitPassType pass,
                       bool preloaded,
                       Trace* trace,
                       bool first_element_checked,
                       int* checked_up_to)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    bool ascii = compiler->ascii();
    jit::Label* backtrack = trace->backtrack();
    QuickCheckDetails* quick_check = trace->quick_check_performed();
    int element_count = elements().length();
    for (int i = preloaded ? 0 : element_count - 1; i >= 0; i--) {
        TextElement elm = elements()[i];
        int cp_offset = trace->cp_offset() + elm.cp_offset();
        if (elm.text_type() == TextElement::ATOM) {
            const CharacterVector &quarks = elm.atom()->data();
            for (int j = preloaded ? 0 : quarks.length() - 1; j >= 0; j--) {
                if (first_element_checked && i == 0 && j == 0) continue;
                if (DeterminedAlready(quick_check, elm.cp_offset() + j)) continue;
                EmitCharacterFunction* emit_function = nullptr;
                switch (pass) {
                  case NON_ASCII_MATCH:
                    JS_ASSERT(ascii);
                    if (quarks[j] > kMaxOneByteCharCode) {
                        assembler->JumpOrBacktrack(backtrack);
                        return;
                    }
                    break;
                  case NON_LETTER_CHARACTER_MATCH:
                    emit_function = &EmitAtomNonLetter;
                    break;
                  case SIMPLE_CHARACTER_MATCH:
                    emit_function = &EmitSimpleCharacter;
                    break;
                  case CASE_CHARACTER_MATCH:
                    emit_function = &EmitAtomLetter;
                    break;
                  default:
                    break;
                }
                if (emit_function != nullptr) {
                    bool bound_checked = emit_function(compiler,
                                                       quarks[j],
                                                       backtrack,
                                                       cp_offset + j,
                                                       *checked_up_to < cp_offset + j,
                                                       preloaded);
                    if (bound_checked) UpdateBoundsCheck(cp_offset + j, checked_up_to);
                }
            }
        } else {
            JS_ASSERT(TextElement::CHAR_CLASS == elm.text_type());
            if (pass == CHARACTER_CLASS_MATCH) {
                if (first_element_checked && i == 0) continue;
                if (DeterminedAlready(quick_check, elm.cp_offset())) continue;
                RegExpCharacterClass* cc = elm.char_class();
                EmitCharClass(alloc(),
                              assembler,
                              cc,
                              ascii,
                              backtrack,
                              cp_offset,
                              *checked_up_to < cp_offset,
                              preloaded);
                UpdateBoundsCheck(cp_offset, checked_up_to);
            }
        }
    }
}

int
TextNode::Length()
{
    TextElement elm = elements()[elements().length() - 1];
    JS_ASSERT(elm.cp_offset() >= 0);
    return elm.cp_offset() + elm.length();
}

bool
TextNode::SkipPass(int int_pass, bool ignore_case)
{
    TextEmitPassType pass = static_cast<TextEmitPassType>(int_pass);
    if (ignore_case)
        return pass == SIMPLE_CHARACTER_MATCH;
    return pass == NON_LETTER_CHARACTER_MATCH || pass == CASE_CHARACTER_MATCH;
}







void
TextNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    LimitResult limit_result = LimitVersions(compiler, trace);
    if (limit_result == DONE) return;
    JS_ASSERT(limit_result == CONTINUE);

    if (trace->cp_offset() + Length() > RegExpMacroAssembler::kMaxCPOffset) {
        compiler->SetRegExpTooBig();
        return;
    }

    if (compiler->ascii()) {
        int dummy = 0;
        TextEmitPass(compiler, NON_ASCII_MATCH, false, trace, false, &dummy);
    }

    bool first_elt_done = false;
    int bound_checked_to = trace->cp_offset() - 1;
    bound_checked_to += trace->bound_checked_up_to();

    
    
    if (trace->characters_preloaded() == 1) {
        for (int pass = kFirstRealPass; pass <= kLastPass; pass++) {
            if (!SkipPass(pass, compiler->ignore_case())) {
                TextEmitPass(compiler,
                             static_cast<TextEmitPassType>(pass),
                             true,
                             trace,
                             false,
                             &bound_checked_to);
            }
        }
        first_elt_done = true;
    }

    for (int pass = kFirstRealPass; pass <= kLastPass; pass++) {
        if (!SkipPass(pass, compiler->ignore_case())) {
            TextEmitPass(compiler,
                         static_cast<TextEmitPassType>(pass),
                         false,
                         trace,
                         first_elt_done,
                         &bound_checked_to);
        }
    }

    Trace successor_trace(*trace);
    successor_trace.set_at_start(false);
    successor_trace.AdvanceCurrentPositionInTrace(Length(), compiler);
    RecursionCheck rc(compiler);
    on_success()->Emit(compiler, &successor_trace);
}

void
LoopChoiceNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* macro_assembler = compiler->macro_assembler();
    if (trace->stop_node() == this) {
        int text_length =
            GreedyLoopTextLengthForAlternative(&alternatives()[0]);
        JS_ASSERT(text_length != kNodeIsTooComplexForGreedyLoops);
        
        
        JS_ASSERT(trace->cp_offset() == text_length);
        macro_assembler->AdvanceCurrentPosition(text_length);
        macro_assembler->JumpOrBacktrack(trace->loop_label());
        return;
    }
    JS_ASSERT(trace->stop_node() == nullptr);
    if (!trace->is_trivial()) {
        trace->Flush(compiler, this);
        return;
    }
    ChoiceNode::Emit(compiler, trace);
}

















































































class irregexp::AlternativeGeneration
{
  public:
    AlternativeGeneration()
      : possible_success(),
        expects_preload(false),
        after(),
        quick_check_details()
    {}

    jit::Label possible_success;
    bool expects_preload;
    jit::Label after;
    QuickCheckDetails quick_check_details;
};

void
ChoiceNode::GenerateGuard(RegExpMacroAssembler* macro_assembler,
                          Guard* guard, Trace* trace)
{
    switch (guard->op()) {
      case Guard::LT:
        JS_ASSERT(!trace->mentions_reg(guard->reg()));
        macro_assembler->IfRegisterGE(guard->reg(),
                                      guard->value(),
                                      trace->backtrack());
        break;
      case Guard::GEQ:
        JS_ASSERT(!trace->mentions_reg(guard->reg()));
        macro_assembler->IfRegisterLT(guard->reg(),
                                      guard->value(),
                                      trace->backtrack());
        break;
    }
}

int
ChoiceNode::CalculatePreloadCharacters(RegExpCompiler* compiler, int eats_at_least)
{
    int preload_characters = Min(4, eats_at_least);
    if (compiler->macro_assembler()->CanReadUnaligned()) {
        bool ascii = compiler->ascii();
        if (ascii) {
            if (preload_characters > 4)
                preload_characters = 4;
            
            
            
            if (preload_characters == 3)
                preload_characters = 2;
        } else {
            if (preload_characters > 2)
                preload_characters = 2;
        }
    } else {
        if (preload_characters > 1)
            preload_characters = 1;
    }
    return preload_characters;
}

RegExpNode *
TextNode::GetSuccessorOfOmnivorousTextNode(RegExpCompiler* compiler)
{
    if (elements().length() != 1)
        return nullptr;

    TextElement elm = elements()[0];
    if (elm.text_type() != TextElement::CHAR_CLASS)
        return nullptr;

    RegExpCharacterClass* node = elm.char_class();
    CharacterRangeVector &ranges = node->ranges(alloc());

    if (!CharacterRange::IsCanonical(ranges))
        CharacterRange::Canonicalize(ranges);

    if (node->is_negated())
        return ranges.length() == 0 ? on_success() : nullptr;

    if (ranges.length() != 1)
        return nullptr;

    uint32_t max_char = MaximumCharacter(compiler->ascii());
    return ranges[0].IsEverything(max_char) ? on_success() : nullptr;
}





int
ChoiceNode::GreedyLoopTextLengthForAlternative(GuardedAlternative* alternative)
{
    int length = 0;
    RegExpNode* node = alternative->node();
    
    
    int recursion_depth = 0;
    while (node != this) {
        if (recursion_depth++ > RegExpCompiler::kMaxRecursion) {
            return kNodeIsTooComplexForGreedyLoops;
        }
        int node_length = node->GreedyLoopTextLength();
        if (node_length == kNodeIsTooComplexForGreedyLoops) {
            return kNodeIsTooComplexForGreedyLoops;
        }
        length += node_length;
        SeqRegExpNode* seq_node = static_cast<SeqRegExpNode*>(node);
        node = seq_node->on_success();
    }
    return length;
}



class AlternativeGenerationList
{
  public:
    AlternativeGenerationList(LifoAlloc *alloc, size_t count)
      : alt_gens_(*alloc)
    {
        alt_gens_.reserve(count);
        for (size_t i = 0; i < count && i < kAFew; i++)
            alt_gens_.append(a_few_alt_gens_ + i);
        for (size_t i = kAFew; i < count; i++)
            alt_gens_.append(js_new<AlternativeGeneration>());
    }

    ~AlternativeGenerationList() {
        for (size_t i = kAFew; i < alt_gens_.length(); i++) {
            js_delete(alt_gens_[i]);
            alt_gens_[i] = nullptr;
        }
    }

    AlternativeGeneration *at(int i) {
        return alt_gens_[i];
    }

  private:
    static const size_t kAFew = 10;
    Vector<AlternativeGeneration *, 1, LifoAllocPolicy<Infallible> > alt_gens_;
    AlternativeGeneration a_few_alt_gens_[kAFew];
};

void
ChoiceNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* macro_assembler = compiler->macro_assembler();
    size_t choice_count = alternatives().length();
#ifdef DEBUG
    for (size_t i = 0; i < choice_count - 1; i++) {
        const GuardedAlternative &alternative = alternatives()[i];
        const GuardVector *guards = alternative.guards();
        if (guards) {
            for (size_t j = 0; j < guards->length(); j++)
                JS_ASSERT(!trace->mentions_reg((*guards)[j]->reg()));
        }
    }
#endif

    LimitResult limit_result = LimitVersions(compiler, trace);
    if (limit_result == DONE) return;
    JS_ASSERT(limit_result == CONTINUE);

    int new_flush_budget = trace->flush_budget() / choice_count;
    if (trace->flush_budget() == 0 && trace->actions() != nullptr) {
        trace->Flush(compiler, this);
        return;
    }

    RecursionCheck rc(compiler);

    Trace* current_trace = trace;

    int text_length = GreedyLoopTextLengthForAlternative(&alternatives()[0]);
    bool greedy_loop = false;
    jit::Label greedy_loop_label;
    Trace counter_backtrack_trace;
    counter_backtrack_trace.set_backtrack(&greedy_loop_label);
    if (not_at_start()) counter_backtrack_trace.set_at_start(false);

    if (choice_count > 1 && text_length != kNodeIsTooComplexForGreedyLoops) {
        
        
        
        
        
        
        
        greedy_loop = true;
        JS_ASSERT(trace->stop_node() == nullptr);
        macro_assembler->PushCurrentPosition();
        current_trace = &counter_backtrack_trace;
        jit::Label greedy_match_failed;
        Trace greedy_match_trace;
        if (not_at_start()) greedy_match_trace.set_at_start(false);
        greedy_match_trace.set_backtrack(&greedy_match_failed);
        jit::Label loop_label;
        macro_assembler->Bind(&loop_label);
        greedy_match_trace.set_stop_node(this);
        greedy_match_trace.set_loop_label(&loop_label);
        alternatives()[0].node()->Emit(compiler, &greedy_match_trace);
        macro_assembler->Bind(&greedy_match_failed);
    }

    jit::Label second_choice;  
    macro_assembler->Bind(&second_choice);

    size_t first_normal_choice = greedy_loop ? 1 : 0;

    bool not_at_start = current_trace->at_start() == Trace::FALSE_VALUE;
    const int kEatsAtLeastNotYetInitialized = -1;
    int eats_at_least = kEatsAtLeastNotYetInitialized;

    bool skip_was_emitted = false;

    if (!greedy_loop && choice_count == 2) {
        GuardedAlternative alt1 = alternatives()[1];
        if (!alt1.guards() || alt1.guards()->length() == 0) {
            RegExpNode* eats_anything_node = alt1.node();
            if (eats_anything_node->GetSuccessorOfOmnivorousTextNode(compiler) == this) {
                
                
                
                
                
                
                
                JS_ASSERT(trace->is_trivial());  
                BoyerMooreLookahead* lookahead = bm_info(not_at_start);
                if (lookahead == nullptr) {
                    eats_at_least = Min(kMaxLookaheadForBoyerMoore,
                                        EatsAtLeast(kMaxLookaheadForBoyerMoore,
                                                    kRecursionBudget,
                                                    not_at_start));
                    if (eats_at_least >= 1) {
                        BoyerMooreLookahead* bm =
                            alloc()->newInfallible<BoyerMooreLookahead>(alloc(), eats_at_least, compiler);
                        GuardedAlternative alt0 = alternatives()[0];
                        alt0.node()->FillInBMInfo(0, kRecursionBudget, bm, not_at_start);
                        skip_was_emitted = bm->EmitSkipInstructions(macro_assembler);
                    }
                } else {
                    skip_was_emitted = lookahead->EmitSkipInstructions(macro_assembler);
                }
            }
        }
    }

    if (eats_at_least == kEatsAtLeastNotYetInitialized) {
        
        eats_at_least =
            EatsAtLeast(compiler->ascii() ? 4 : 2, kRecursionBudget, not_at_start);
    }
    int preload_characters = CalculatePreloadCharacters(compiler, eats_at_least);

    bool preload_is_current = !skip_was_emitted &&
        (current_trace->characters_preloaded() == preload_characters);
    bool preload_has_checked_bounds = preload_is_current;

    AlternativeGenerationList alt_gens(alloc(), choice_count);

    
    
    for (size_t i = first_normal_choice; i < choice_count; i++) {
        GuardedAlternative alternative = alternatives()[i];
        AlternativeGeneration* alt_gen = alt_gens.at(i);
        alt_gen->quick_check_details.set_characters(preload_characters);
        const GuardVector *guards = alternative.guards();
        Trace new_trace(*current_trace);
        new_trace.set_characters_preloaded(preload_is_current ?
                                           preload_characters :
                                           0);
        if (preload_has_checked_bounds) {
            new_trace.set_bound_checked_up_to(preload_characters);
        }
        new_trace.quick_check_performed()->Clear();
        if (not_at_start_) new_trace.set_at_start(Trace::FALSE_VALUE);
        alt_gen->expects_preload = preload_is_current;
        bool generate_full_check_inline = false;
        if (try_to_emit_quick_check_for_alternative(i) &&
            alternative.node()->EmitQuickCheck(compiler,
                                               &new_trace,
                                               preload_has_checked_bounds,
                                               &alt_gen->possible_success,
                                               &alt_gen->quick_check_details,
                                               i < choice_count - 1)) {
            
            preload_is_current = true;
            preload_has_checked_bounds = true;
            
            
            
            if (i == choice_count - 1) {
                macro_assembler->Bind(&alt_gen->possible_success);
                new_trace.set_quick_check_performed(&alt_gen->quick_check_details);
                new_trace.set_characters_preloaded(preload_characters);
                new_trace.set_bound_checked_up_to(preload_characters);
                generate_full_check_inline = true;
            }
        } else if (alt_gen->quick_check_details.cannot_match()) {
            if (i == choice_count - 1 && !greedy_loop) {
                macro_assembler->JumpOrBacktrack(trace->backtrack());
            }
            continue;
        } else {
            
            
            
            
            
            if (i != first_normal_choice) {
                alt_gen->expects_preload = false;
                new_trace.InvalidateCurrentCharacter();
            }
            if (i < choice_count - 1) {
                new_trace.set_backtrack(&alt_gen->after);
            }
            generate_full_check_inline = true;
        }
        if (generate_full_check_inline) {
            if (new_trace.actions() != nullptr)
                new_trace.set_flush_budget(new_flush_budget);
            if (guards) {
                for (size_t j = 0; j < guards->length(); j++)
                    GenerateGuard(macro_assembler, (*guards)[j], &new_trace);
            }
            alternative.node()->Emit(compiler, &new_trace);
            preload_is_current = false;
        }
        macro_assembler->Bind(&alt_gen->after);
    }
    if (greedy_loop) {
        macro_assembler->Bind(&greedy_loop_label);
        
        macro_assembler->CheckGreedyLoop(trace->backtrack());
        
        macro_assembler->AdvanceCurrentPosition(-text_length);
        macro_assembler->JumpOrBacktrack(&second_choice);
    }

    
    
    
    for (size_t i = first_normal_choice; i < choice_count - 1; i++) {
        AlternativeGeneration* alt_gen = alt_gens.at(i);
        Trace new_trace(*current_trace);
        
        
        
        if (new_trace.actions() != nullptr) {
            new_trace.set_flush_budget(new_flush_budget);
        }
        EmitOutOfLineContinuation(compiler,
                                  &new_trace,
                                  alternatives()[i],
                                  alt_gen,
                                  preload_characters,
                                  alt_gens.at(i + 1)->expects_preload);
    }
}

void
ChoiceNode::EmitOutOfLineContinuation(RegExpCompiler* compiler,
                                      Trace* trace,
                                      GuardedAlternative alternative,
                                      AlternativeGeneration* alt_gen,
                                      int preload_characters,
                                      bool next_expects_preload)
{
    if (!alt_gen->possible_success.used())
        return;

    RegExpMacroAssembler* macro_assembler = compiler->macro_assembler();
    macro_assembler->Bind(&alt_gen->possible_success);
    Trace out_of_line_trace(*trace);
    out_of_line_trace.set_characters_preloaded(preload_characters);
    out_of_line_trace.set_quick_check_performed(&alt_gen->quick_check_details);
    if (not_at_start_) out_of_line_trace.set_at_start(Trace::FALSE_VALUE);
    const GuardVector *guards = alternative.guards();
    if (next_expects_preload) {
        jit::Label reload_current_char;
        out_of_line_trace.set_backtrack(&reload_current_char);
        if (guards) {
            for (size_t j = 0; j < guards->length(); j++)
                GenerateGuard(macro_assembler, (*guards)[j], &out_of_line_trace);
        }
        alternative.node()->Emit(compiler, &out_of_line_trace);
        macro_assembler->Bind(&reload_current_char);
        
        
        
        macro_assembler->LoadCurrentCharacter(trace->cp_offset(),
                                              nullptr,
                                              false,
                                              preload_characters);
        macro_assembler->JumpOrBacktrack(&(alt_gen->after));
    } else {
        out_of_line_trace.set_backtrack(&(alt_gen->after));
        if (guards) {
            for (size_t j = 0; j < guards->length(); j++)
                GenerateGuard(macro_assembler, (*guards)[j], &out_of_line_trace);
        }
        alternative.node()->Emit(compiler, &out_of_line_trace);
    }
}

void
ActionNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    LimitResult limit_result = LimitVersions(compiler, trace);
    if (limit_result == DONE) return;
    JS_ASSERT(limit_result == CONTINUE);

    RecursionCheck rc(compiler);

    switch (action_type_) {
      case STORE_POSITION: {
        Trace::DeferredCapture
            new_capture(data_.u_position_register.reg,
                        data_.u_position_register.is_capture,
                        trace);
        Trace new_trace = *trace;
        new_trace.add_action(&new_capture);
        on_success()->Emit(compiler, &new_trace);
        break;
      }
      case INCREMENT_REGISTER: {
        Trace::DeferredIncrementRegister
            new_increment(data_.u_increment_register.reg);
        Trace new_trace = *trace;
        new_trace.add_action(&new_increment);
        on_success()->Emit(compiler, &new_trace);
        break;
      }
      case SET_REGISTER: {
        Trace::DeferredSetRegister
            new_set(data_.u_store_register.reg, data_.u_store_register.value);
        Trace new_trace = *trace;
        new_trace.add_action(&new_set);
        on_success()->Emit(compiler, &new_trace);
        break;
      }
      case CLEAR_CAPTURES: {
        Trace::DeferredClearCaptures
            new_capture(Interval(data_.u_clear_captures.range_from,
                                 data_.u_clear_captures.range_to));
        Trace new_trace = *trace;
        new_trace.add_action(&new_capture);
        on_success()->Emit(compiler, &new_trace);
        break;
      }
      case BEGIN_SUBMATCH:
        if (!trace->is_trivial()) {
            trace->Flush(compiler, this);
        } else {
            assembler->WriteCurrentPositionToRegister(data_.u_submatch.current_position_register, 0);
            assembler->WriteBacktrackStackPointerToRegister(data_.u_submatch.stack_pointer_register);
            on_success()->Emit(compiler, trace);
        }
        break;
      case EMPTY_MATCH_CHECK: {
        int start_pos_reg = data_.u_empty_match_check.start_register;
        int stored_pos = 0;
        int rep_reg = data_.u_empty_match_check.repetition_register;
        bool has_minimum = (rep_reg != RegExpCompiler::kNoRegister);
        bool know_dist = trace->GetStoredPosition(start_pos_reg, &stored_pos);
        if (know_dist && !has_minimum && stored_pos == trace->cp_offset()) {
            
            
            assembler->JumpOrBacktrack(trace->backtrack());
        } else if (know_dist && stored_pos < trace->cp_offset()) {
            
            
            on_success()->Emit(compiler, trace);
        } else if (!trace->is_trivial()) {
            trace->Flush(compiler, this);
        } else {
            jit::Label skip_empty_check;
            
            
            if (has_minimum) {
                int limit = data_.u_empty_match_check.repetition_limit;
                assembler->IfRegisterLT(rep_reg, limit, &skip_empty_check);
            }
            
            
            assembler->IfRegisterEqPos(data_.u_empty_match_check.start_register,
                                       trace->backtrack());
            assembler->Bind(&skip_empty_check);
            on_success()->Emit(compiler, trace);
        }
        break;
      }
      case POSITIVE_SUBMATCH_SUCCESS: {
        if (!trace->is_trivial()) {
            trace->Flush(compiler, this);
            return;
        }
        assembler->ReadCurrentPositionFromRegister(data_.u_submatch.current_position_register);
        assembler->ReadBacktrackStackPointerFromRegister(data_.u_submatch.stack_pointer_register);
        int clear_register_count = data_.u_submatch.clear_register_count;
        if (clear_register_count == 0) {
            on_success()->Emit(compiler, trace);
            return;
        }
        int clear_registers_from = data_.u_submatch.clear_register_from;
        jit::Label clear_registers_backtrack;
        Trace new_trace = *trace;
        new_trace.set_backtrack(&clear_registers_backtrack);
        on_success()->Emit(compiler, &new_trace);

        assembler->Bind(&clear_registers_backtrack);
        int clear_registers_to = clear_registers_from + clear_register_count - 1;
        assembler->ClearRegisters(clear_registers_from, clear_registers_to);

        JS_ASSERT(trace->backtrack() == nullptr);
        assembler->Backtrack();
        return;
      }
      default:
        MOZ_CRASH("Bad action");
    }
}

void
BackReferenceNode::Emit(RegExpCompiler* compiler, Trace* trace)
{
    RegExpMacroAssembler* assembler = compiler->macro_assembler();
    if (!trace->is_trivial()) {
        trace->Flush(compiler, this);
        return;
    }

    LimitResult limit_result = LimitVersions(compiler, trace);
    if (limit_result == DONE) return;
    JS_ASSERT(limit_result == CONTINUE);

    RecursionCheck rc(compiler);

    JS_ASSERT(start_reg_ + 1 == end_reg_);
    if (compiler->ignore_case()) {
        assembler->CheckNotBackReferenceIgnoreCase(start_reg_,
                                                   trace->backtrack());
    } else {
        assembler->CheckNotBackReference(start_reg_, trace->backtrack());
    }
    on_success()->Emit(compiler, trace);
}

RegExpNode::LimitResult
RegExpNode::LimitVersions(RegExpCompiler* compiler, Trace* trace)
{
    
    if (trace->stop_node() != nullptr)
        return CONTINUE;

    RegExpMacroAssembler* macro_assembler = compiler->macro_assembler();
    if (trace->is_trivial()) {
        if (label()->bound()) {
            
            
            macro_assembler->JumpOrBacktrack(label());
            return DONE;
        }
        if (compiler->recursion_depth() >= RegExpCompiler::kMaxRecursion) {
            
            
            compiler->AddWork(this);
            macro_assembler->JumpOrBacktrack(label());
            return DONE;
        }
        
        macro_assembler->Bind(label());
        return CONTINUE;
    }

    
    
    trace_count_++;
    if (trace_count_ < kMaxCopiesCodeGenerated &&
        compiler->recursion_depth() <= RegExpCompiler::kMaxRecursion) {
        return CONTINUE;
    }

    
    
    
    trace->Flush(compiler, this);
    return DONE;
}

bool
RegExpNode::EmitQuickCheck(RegExpCompiler* compiler,
                           Trace* trace,
                           bool preload_has_checked_bounds,
                           jit::Label* on_possible_success,
                           QuickCheckDetails* details,
                           bool fall_through_on_failure)
{
    if (details->characters() == 0) return false;
    GetQuickCheckDetails(
                         details, compiler, 0, trace->at_start() == Trace::FALSE_VALUE);
    if (details->cannot_match()) return false;
    if (!details->Rationalize(compiler->ascii())) return false;
    JS_ASSERT(details->characters() == 1 ||
              compiler->macro_assembler()->CanReadUnaligned());
    uint32_t mask = details->mask();
    uint32_t value = details->value();

    RegExpMacroAssembler* assembler = compiler->macro_assembler();

    if (trace->characters_preloaded() != details->characters()) {
        assembler->LoadCurrentCharacter(trace->cp_offset(),
                                        trace->backtrack(),
                                        !preload_has_checked_bounds,
                                        details->characters());
    }

    bool need_mask = true;

    if (details->characters() == 1) {
        
        
        uint32_t char_mask = MaximumCharacter(compiler->ascii());
        if ((mask & char_mask) == char_mask) need_mask = false;
        mask &= char_mask;
    } else {
        
        
        if (details->characters() == 2 && compiler->ascii()) {
            if ((mask & 0xffff) == 0xffff) need_mask = false;
        } else if (details->characters() == 1 && !compiler->ascii()) {
            if ((mask & 0xffff) == 0xffff) need_mask = false;
        } else {
            if (mask == 0xffffffff) need_mask = false;
        }
    }

    if (fall_through_on_failure) {
        if (need_mask) {
            assembler->CheckCharacterAfterAnd(value, mask, on_possible_success);
        } else {
            assembler->CheckCharacter(value, on_possible_success);
        }
    } else {
        if (need_mask) {
            assembler->CheckNotCharacterAfterAnd(value, mask, trace->backtrack());
        } else {
            assembler->CheckNotCharacter(value, trace->backtrack());
        }
    }
    return true;
}

bool
TextNode::FillInBMInfo(int initial_offset,
                       int budget,
                       BoyerMooreLookahead* bm,
                       bool not_at_start)
{
    if (!bm->CheckOverRecursed())
        return false;

    if (initial_offset >= bm->length())
        return true;

    int offset = initial_offset;
    int max_char = bm->max_char();
    for (size_t i = 0; i < elements().length(); i++) {
        if (offset >= bm->length()) {
            if (initial_offset == 0)
                set_bm_info(not_at_start, bm);
            return true;
        }
        TextElement text = elements()[i];
        if (text.text_type() == TextElement::ATOM) {
            RegExpAtom* atom = text.atom();
            for (int j = 0; j < atom->length(); j++, offset++) {
                if (offset >= bm->length()) {
                    if (initial_offset == 0)
                        set_bm_info(not_at_start, bm);
                    return true;
                }
                char16_t character = atom->data()[j];
                if (bm->compiler()->ignore_case()) {
                    char16_t chars[kEcma262UnCanonicalizeMaxWidth];
                    int length = GetCaseIndependentLetters(character,
                                                           bm->max_char() == kMaxOneByteCharCode,
                                                           chars);
                    for (int j = 0; j < length; j++)
                        bm->Set(offset, chars[j]);
                } else {
                    if (character <= max_char) bm->Set(offset, character);
                }
            }
        } else {
            JS_ASSERT(TextElement::CHAR_CLASS == text.text_type());
            RegExpCharacterClass* char_class = text.char_class();
            const CharacterRangeVector &ranges = char_class->ranges(alloc());
            if (char_class->is_negated()) {
                bm->SetAll(offset);
            } else {
                for (size_t k = 0; k < ranges.length(); k++) {
                    const CharacterRange &range = ranges[k];
                    if (range.from() > max_char)
                        continue;
                    int to = Min(max_char, static_cast<int>(range.to()));
                    bm->SetInterval(offset, Interval(range.from(), to));
                }
            }
            offset++;
        }
    }
    if (offset >= bm->length()) {
        if (initial_offset == 0) set_bm_info(not_at_start, bm);
        return true;
    }
    if (!on_success()->FillInBMInfo(offset,
                                    budget - 1,
                                    bm,
                                    true))   
        return false;
    if (initial_offset == 0)
        set_bm_info(not_at_start, bm);
    return true;
}





static inline uint32_t
SmearBitsRight(uint32_t v)
{
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v;
}









void
TextNode::GetQuickCheckDetails(QuickCheckDetails* details,
                               RegExpCompiler* compiler,
                               int characters_filled_in,
                               bool not_at_start)
{
    JS_ASSERT(characters_filled_in < details->characters());
    int characters = details->characters();
    int char_mask = MaximumCharacter(compiler->ascii());

    for (size_t k = 0; k < elements().length(); k++) {
        TextElement elm = elements()[k];
        if (elm.text_type() == TextElement::ATOM) {
            const CharacterVector &quarks = elm.atom()->data();
            for (size_t i = 0; i < (size_t) characters && i < quarks.length(); i++) {
                QuickCheckDetails::Position* pos =
                    details->positions(characters_filled_in);
                char16_t c = quarks[i];
                if (c > char_mask) {
                    
                    
                    
                    
                    details->set_cannot_match();
                    pos->determines_perfectly = false;
                    return;
                }
                if (compiler->ignore_case()) {
                    char16_t chars[kEcma262UnCanonicalizeMaxWidth];
                    size_t length = GetCaseIndependentLetters(c, compiler->ascii(), chars);
                    JS_ASSERT(length != 0);  
                    if (length == 1) {
                        
                        
                        
                        pos->mask = char_mask;
                        pos->value = c;
                        pos->determines_perfectly = true;
                    } else {
                        uint32_t common_bits = char_mask;
                        uint32_t bits = chars[0];
                        for (size_t j = 1; j < length; j++) {
                            uint32_t differing_bits = ((chars[j] & common_bits) ^ bits);
                            common_bits ^= differing_bits;
                            bits &= common_bits;
                        }
                        
                        
                        
                        
                        uint32_t one_zero = (common_bits | ~char_mask);
                        if (length == 2 && ((~one_zero) & ((~one_zero) - 1)) == 0) {
                            pos->determines_perfectly = true;
                        }
                        pos->mask = common_bits;
                        pos->value = bits;
                    }
                } else {
                    
                    
                    
                    pos->mask = char_mask;
                    pos->value = c;
                    pos->determines_perfectly = true;
                }
                characters_filled_in++;
                JS_ASSERT(characters_filled_in <= details->characters());
                if (characters_filled_in == details->characters()) {
                    return;
                }
            }
        } else {
            QuickCheckDetails::Position* pos =
                details->positions(characters_filled_in);
            RegExpCharacterClass* tree = elm.char_class();
            const CharacterRangeVector &ranges = tree->ranges(alloc());
            if (tree->is_negated()) {
                
                
                
                
                pos->mask = 0;
                pos->value = 0;
            } else {
                size_t first_range = 0;
                while (ranges[first_range].from() > char_mask) {
                    first_range++;
                    if (first_range == ranges.length()) {
                        details->set_cannot_match();
                        pos->determines_perfectly = false;
                        return;
                    }
                }
                CharacterRange range = ranges[first_range];
                char16_t from = range.from();
                char16_t to = range.to();
                if (to > char_mask) {
                    to = char_mask;
                }
                uint32_t differing_bits = (from ^ to);
                
                
                if ((differing_bits & (differing_bits + 1)) == 0 &&
                    from + differing_bits == to) {
                    pos->determines_perfectly = true;
                }
                uint32_t common_bits = ~SmearBitsRight(differing_bits);
                uint32_t bits = (from & common_bits);
                for (size_t i = first_range + 1; i < ranges.length(); i++) {
                    CharacterRange range = ranges[i];
                    char16_t from = range.from();
                    char16_t to = range.to();
                    if (from > char_mask) continue;
                    if (to > char_mask) to = char_mask;
                    
                    
                    
                    
                    
                    pos->determines_perfectly = false;
                    uint32_t new_common_bits = (from ^ to);
                    new_common_bits = ~SmearBitsRight(new_common_bits);
                    common_bits &= new_common_bits;
                    bits &= new_common_bits;
                    uint32_t differing_bits = (from & common_bits) ^ bits;
                    common_bits ^= differing_bits;
                    bits &= common_bits;
                }
                pos->mask = common_bits;
                pos->value = bits;
            }
            characters_filled_in++;
            JS_ASSERT(characters_filled_in <= details->characters());
            if (characters_filled_in == details->characters()) {
                return;
            }
        }
    }
    JS_ASSERT(characters_filled_in != details->characters());
    if (!details->cannot_match()) {
        on_success()-> GetQuickCheckDetails(details,
                                            compiler,
                                            characters_filled_in,
                                            true);
    }
}

void
QuickCheckDetails::Clear()
{
    for (int i = 0; i < characters_; i++) {
        positions_[i].mask = 0;
        positions_[i].value = 0;
        positions_[i].determines_perfectly = false;
    }
    characters_ = 0;
}

void
QuickCheckDetails::Advance(int by, bool ascii)
{
    JS_ASSERT(by >= 0);
    if (by >= characters_) {
        Clear();
        return;
    }
    for (int i = 0; i < characters_ - by; i++) {
        positions_[i] = positions_[by + i];
    }
    for (int i = characters_ - by; i < characters_; i++) {
        positions_[i].mask = 0;
        positions_[i].value = 0;
        positions_[i].determines_perfectly = false;
    }
    characters_ -= by;
    
    
    
}

bool
QuickCheckDetails::Rationalize(bool is_ascii)
{
    bool found_useful_op = false;
    uint32_t char_mask = MaximumCharacter(is_ascii);

    mask_ = 0;
    value_ = 0;
    int char_shift = 0;
    for (int i = 0; i < characters_; i++) {
        Position* pos = &positions_[i];
        if ((pos->mask & kMaxOneByteCharCode) != 0)
            found_useful_op = true;
        mask_ |= (pos->mask & char_mask) << char_shift;
        value_ |= (pos->value & char_mask) << char_shift;
        char_shift += is_ascii ? 8 : 16;
    }
    return found_useful_op;
}

void QuickCheckDetails::Merge(QuickCheckDetails* other, int from_index)
{
    JS_ASSERT(characters_ == other->characters_);
    if (other->cannot_match_)
        return;
    if (cannot_match_) {
        *this = *other;
        return;
    }
    for (int i = from_index; i < characters_; i++) {
        QuickCheckDetails::Position* pos = positions(i);
        QuickCheckDetails::Position* other_pos = other->positions(i);
        if (pos->mask != other_pos->mask ||
            pos->value != other_pos->value ||
            !other_pos->determines_perfectly) {
            
            
            pos->determines_perfectly = false;
        }
        pos->mask &= other_pos->mask;
        pos->value &= pos->mask;
        other_pos->value &= pos->mask;
        char16_t differing_bits = (pos->value ^ other_pos->value);
        pos->mask &= ~differing_bits;
        pos->value &= pos->mask;
    }
}
