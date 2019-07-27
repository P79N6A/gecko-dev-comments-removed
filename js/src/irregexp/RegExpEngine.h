





























#ifndef V8_JSREGEXP_H_
#define V8_JSREGEXP_H_

#include "jscntxt.h"

#include "ds/SplayTree.h"
#include "jit/Label.h"
#include "vm/RegExpObject.h"

namespace js {

class MatchPairs;
class RegExpShared;

namespace jit {
    class Label;
    class JitCode;
}

namespace irregexp {

class RegExpTree;
class RegExpMacroAssembler;

struct RegExpCompileData
{
    RegExpCompileData()
      : tree(nullptr),
        simple(true),
        contains_anchor(false),
        capture_count(0)
    {}

    RegExpTree *tree;
    bool simple;
    bool contains_anchor;
    int capture_count;
};

struct RegExpCode
{
    jit::JitCode *jitCode;
    uint8_t *byteCode;

    RegExpCode()
      : jitCode(nullptr), byteCode(nullptr)
    {}

    bool empty() {
        return !jitCode && !byteCode;
    }

    void destroy() {
        js_free(byteCode);
    }
};

RegExpCode
CompilePattern(JSContext *cx, RegExpShared *shared, RegExpCompileData *data,
               HandleLinearString sample,  bool is_global, bool ignore_case = false,
               bool is_ascii = false);



template <typename CharT>
RegExpRunStatus
ExecuteCode(JSContext *cx, jit::JitCode *codeBlock, const CharT *chars, size_t start,
            size_t length, MatchPairs *matches);

template <typename CharT>
RegExpRunStatus
InterpretCode(JSContext *cx, const uint8_t *byteCode, const CharT *chars, size_t start,
              size_t length, MatchPairs *matches);

#define FOR_EACH_NODE_TYPE(VISIT)                                    \
  VISIT(End)                                                         \
  VISIT(Action)                                                      \
  VISIT(Choice)                                                      \
  VISIT(BackReference)                                               \
  VISIT(Assertion)                                                   \
  VISIT(Text)

#define FOR_EACH_REG_EXP_TREE_TYPE(VISIT)                            \
  VISIT(Disjunction)                                                 \
  VISIT(Alternative)                                                 \
  VISIT(Assertion)                                                   \
  VISIT(CharacterClass)                                              \
  VISIT(Atom)                                                        \
  VISIT(Quantifier)                                                  \
  VISIT(Capture)                                                     \
  VISIT(Lookahead)                                                   \
  VISIT(BackReference)                                               \
  VISIT(Empty)                                                       \
  VISIT(Text)

#define FORWARD_DECLARE(Name) class RegExp##Name;
FOR_EACH_REG_EXP_TREE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

class CharacterRange;
typedef Vector<CharacterRange, 1, LifoAllocPolicy<Infallible> > CharacterRangeVector;



class CharacterRange
{
  public:
    CharacterRange()
      : from_(0), to_(0)
    {}

    CharacterRange(char16_t from, char16_t to)
      : from_(from), to_(to)
    {}

    static void AddClassEscape(LifoAlloc *alloc, char16_t type, CharacterRangeVector *ranges);

    static inline CharacterRange Singleton(char16_t value) {
        return CharacterRange(value, value);
    }
    static inline CharacterRange Range(char16_t from, char16_t to) {
        JS_ASSERT(from <= to);
        return CharacterRange(from, to);
    }
    static inline CharacterRange Everything() {
        return CharacterRange(0, 0xFFFF);
    }
    bool Contains(char16_t i) { return from_ <= i && i <= to_; }
    char16_t from() const { return from_; }
    void set_from(char16_t value) { from_ = value; }
    char16_t to() const { return to_; }
    void set_to(char16_t value) { to_ = value; }
    bool is_valid() { return from_ <= to_; }
    bool IsEverything(char16_t max) { return from_ == 0 && to_ >= max; }
    bool IsSingleton() { return (from_ == to_); }
    void AddCaseEquivalents(bool is_ascii, CharacterRangeVector *ranges);

    static void Split(const LifoAlloc *alloc,
                      CharacterRangeVector base,
                      const Vector<int> &overlay,
                      CharacterRangeVector* included,
                      CharacterRangeVector* excluded);

    
    
    static bool IsCanonical(const CharacterRangeVector &ranges);

    
    
    
    
    static void Canonicalize(CharacterRangeVector &ranges);

    
    static void Negate(const LifoAlloc *alloc,
                       CharacterRangeVector src,
                       CharacterRangeVector *dst);

    static const int kStartMarker = (1 << 24);
    static const int kPayloadMask = (1 << 24) - 1;

  private:
    char16_t from_;
    char16_t to_;
};



class OutSet
{
  public:
    OutSet()
      : first_(0), remaining_(nullptr), successors_(nullptr)
    {}

    OutSet* Extend(LifoAlloc *alloc, unsigned value);
    bool Get(unsigned value);
    static const unsigned kFirstLimit = 32;

  private:
    typedef Vector<OutSet *, 1, LifoAllocPolicy<Infallible> > OutSetVector;
    typedef Vector<unsigned, 1, LifoAllocPolicy<Infallible> > RemainingVector;

    
    
    
    void Set(LifoAlloc *alloc, unsigned value);

    
    
    
    OutSetVector *successors() { return successors_; }

    OutSet(uint32_t first, RemainingVector *remaining)
      : first_(first), remaining_(remaining), successors_(nullptr)
    {}

    RemainingVector &remaining() { return *remaining_; }

    uint32_t first_;
    RemainingVector *remaining_;
    OutSetVector *successors_;
    friend class Trace;
};



class DispatchTable
{
  public:
    explicit DispatchTable(LifoAlloc *alloc)
    {}

    class Entry {
      public:
        Entry()
          : from_(0), to_(0), out_set_(nullptr)
        {}

        Entry(char16_t from, char16_t to, OutSet* out_set)
          : from_(from), to_(to), out_set_(out_set)
        {}

        char16_t from() { return from_; }
        char16_t to() { return to_; }
        void set_to(char16_t value) { to_ = value; }
        void AddValue(LifoAlloc *alloc, int value) {
            out_set_ = out_set_->Extend(alloc, value);
        }
        OutSet* out_set() { return out_set_; }
      private:
        char16_t from_;
        char16_t to_;
        OutSet* out_set_;
    };

    void AddRange(LifoAlloc *alloc, CharacterRange range, int value);
    OutSet* Get(char16_t value);
    void Dump();

  private:
    
    
    OutSet* empty() { return &empty_; }
    OutSet empty_;
};

class TextElement
{
  public:
    enum TextType {
        ATOM,
        CHAR_CLASS
    };

    static TextElement Atom(RegExpAtom* atom);
    static TextElement CharClass(RegExpCharacterClass* char_class);

    int cp_offset() const { return cp_offset_; }
    void set_cp_offset(int cp_offset) { cp_offset_ = cp_offset; }
    int length() const;

    TextType text_type() const { return text_type_; }

    RegExpTree* tree() const { return tree_; }

    RegExpAtom* atom() const {
        JS_ASSERT(text_type() == ATOM);
        return reinterpret_cast<RegExpAtom*>(tree());
    }

    RegExpCharacterClass* char_class() const {
        JS_ASSERT(text_type() == CHAR_CLASS);
        return reinterpret_cast<RegExpCharacterClass*>(tree());
    }

  private:
    TextElement(TextType text_type, RegExpTree* tree)
      : cp_offset_(-1), text_type_(text_type), tree_(tree)
    {}

    int cp_offset_;
    TextType text_type_;
    RegExpTree* tree_;
};

typedef Vector<TextElement, 1, LifoAllocPolicy<Infallible> > TextElementVector;

class NodeVisitor;
class RegExpCompiler;
class Trace;
class BoyerMooreLookahead;

struct NodeInfo
{
    NodeInfo()
      : being_analyzed(false),
        been_analyzed(false),
        follows_word_interest(false),
        follows_newline_interest(false),
        follows_start_interest(false),
        at_end(false),
        visited(false),
        replacement_calculated(false)
    {}

    
    
    bool Matches(NodeInfo* that) {
        return (at_end == that->at_end) &&
            (follows_word_interest == that->follows_word_interest) &&
            (follows_newline_interest == that->follows_newline_interest) &&
            (follows_start_interest == that->follows_start_interest);
    }

    
    
    void AddFromPreceding(NodeInfo* that) {
        at_end |= that->at_end;
        follows_word_interest |= that->follows_word_interest;
        follows_newline_interest |= that->follows_newline_interest;
        follows_start_interest |= that->follows_start_interest;
    }

    bool HasLookbehind() {
        return follows_word_interest ||
            follows_newline_interest ||
            follows_start_interest;
    }

    
    
    void AddFromFollowing(NodeInfo* that) {
        follows_word_interest |= that->follows_word_interest;
        follows_newline_interest |= that->follows_newline_interest;
        follows_start_interest |= that->follows_start_interest;
    }

    void ResetCompilationState() {
        being_analyzed = false;
        been_analyzed = false;
    }

    bool being_analyzed: 1;
    bool been_analyzed: 1;

    
    
    bool follows_word_interest: 1;
    bool follows_newline_interest: 1;
    bool follows_start_interest: 1;

    bool at_end: 1;
    bool visited: 1;
    bool replacement_calculated: 1;
};



class QuickCheckDetails
{
  public:
    QuickCheckDetails()
      : characters_(0),
        mask_(0),
        value_(0),
        cannot_match_(false)
    {}

    explicit QuickCheckDetails(int characters)
      : characters_(characters),
        mask_(0),
        value_(0),
        cannot_match_(false)
    {}

    bool Rationalize(bool ascii);

    
    void Merge(QuickCheckDetails* other, int from_index);

    
    void Advance(int by, bool ascii);

    void Clear();

    bool cannot_match() { return cannot_match_; }
    void set_cannot_match() { cannot_match_ = true; }

    int characters() { return characters_; }
    void set_characters(int characters) { characters_ = characters; }

    struct Position {
        Position() : mask(0), value(0), determines_perfectly(false) { }
        char16_t mask;
        char16_t value;
        bool determines_perfectly;
    };

    Position* positions(int index) {
        JS_ASSERT(index >= 0);
        JS_ASSERT(index < characters_);
        return positions_ + index;
    }

    uint32_t mask() { return mask_; }
    uint32_t value() { return value_; }

  private:
    
    
    int characters_;
    Position positions_[4];

    
    uint32_t mask_;
    uint32_t value_;

    
    
    bool cannot_match_;
};

class RegExpNode
{
  public:
    explicit RegExpNode(LifoAlloc *alloc);
    virtual ~RegExpNode() {}
    virtual void Accept(NodeVisitor* visitor) = 0;

    
    virtual void Emit(RegExpCompiler* compiler, Trace* trace) = 0;

    
    
    
    
    
    
    
    virtual int EatsAtLeast(int still_to_find, int budget, bool not_at_start) = 0;

    
    
    
    bool EmitQuickCheck(RegExpCompiler* compiler,
                        Trace* trace,
                        bool preload_has_checked_bounds,
                        jit::Label* on_possible_success,
                        QuickCheckDetails* details_return,
                        bool fall_through_on_failure);

    
    
    
    
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start) = 0;

    static const int kNodeIsTooComplexForGreedyLoops = -1;

    virtual int GreedyLoopTextLength() { return kNodeIsTooComplexForGreedyLoops; }

    
    
    virtual RegExpNode* GetSuccessorOfOmnivorousTextNode(RegExpCompiler* compiler) {
        return nullptr;
    }

    static const int kRecursionBudget = 200;

    
    
    
    
    
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start) {
        MOZ_CRASH("Bad call");
    }

    
    
    
    virtual RegExpNode* FilterASCII(int depth, bool ignore_case) { return this; }

    
    RegExpNode* replacement() {
        JS_ASSERT(info()->replacement_calculated);
        return replacement_;
    }
    RegExpNode* set_replacement(RegExpNode* replacement) {
        info()->replacement_calculated = true;
        replacement_ =  replacement;
        return replacement;  
    }

    
    
    
    
    void SaveBMInfo(BoyerMooreLookahead* bm, bool not_at_start, int offset) {
        if (offset == 0) set_bm_info(not_at_start, bm);
    }

    jit::Label* label() { return &label_; }

    
    
    
    
    
    static const int kMaxCopiesCodeGenerated = 10;

    NodeInfo* info() { return &info_; }

    BoyerMooreLookahead* bm_info(bool not_at_start) {
        return bm_info_[not_at_start ? 1 : 0];
    }

    LifoAlloc *alloc() const { return alloc_; }

  protected:
    enum LimitResult { DONE, CONTINUE };
    RegExpNode* replacement_;

    LimitResult LimitVersions(RegExpCompiler* compiler, Trace* trace);

    void set_bm_info(bool not_at_start, BoyerMooreLookahead* bm) {
        bm_info_[not_at_start ? 1 : 0] = bm;
    }

  private:
    static const int kFirstCharBudget = 10;
    jit::Label label_;
    NodeInfo info_;

    
    
    
    
    
    int trace_count_;
    BoyerMooreLookahead* bm_info_[2];

    LifoAlloc *alloc_;
};


class Interval
{
  public:
    Interval() : from_(kNone), to_(kNone) { }

    Interval(int from, int to) : from_(from), to_(to) { }

    Interval Union(Interval that) {
        if (that.from_ == kNone)
            return *this;
        else if (from_ == kNone)
            return that;
        else
            return Interval(Min(from_, that.from_), Max(to_, that.to_));
    }

    bool Contains(int value) {
        return (from_ <= value) && (value <= to_);
    }

    bool is_empty() { return from_ == kNone; }

    int from() const { return from_; }
    int to() const { return to_; }

    static Interval Empty() { return Interval(); }
    static const int kNone = -1;

  private:
    int from_;
    int to_;
};

class SeqRegExpNode : public RegExpNode
{
  public:
    explicit SeqRegExpNode(RegExpNode* on_success)
      : RegExpNode(on_success->alloc()), on_success_(on_success)
    {}

    RegExpNode* on_success() { return on_success_; }
    void set_on_success(RegExpNode* node) { on_success_ = node; }
    virtual RegExpNode* FilterASCII(int depth, bool ignore_case);
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);

  protected:
    RegExpNode* FilterSuccessor(int depth, bool ignore_case);

  private:
    RegExpNode* on_success_;
};

class ActionNode : public SeqRegExpNode
{
  public:
    enum ActionType {
        SET_REGISTER,
        INCREMENT_REGISTER,
        STORE_POSITION,
        BEGIN_SUBMATCH,
        POSITIVE_SUBMATCH_SUCCESS,
        EMPTY_MATCH_CHECK,
        CLEAR_CAPTURES
    };

    ActionNode(ActionType action_type, RegExpNode* on_success)
      : SeqRegExpNode(on_success),
        action_type_(action_type)
    {}

    static ActionNode* SetRegister(int reg, int val, RegExpNode* on_success);
    static ActionNode* IncrementRegister(int reg, RegExpNode* on_success);
    static ActionNode* StorePosition(int reg,
                                     bool is_capture,
                                     RegExpNode* on_success);
    static ActionNode* ClearCaptures(Interval range, RegExpNode* on_success);
    static ActionNode* BeginSubmatch(int stack_pointer_reg,
                                     int position_reg,
                                     RegExpNode* on_success);
    static ActionNode* PositiveSubmatchSuccess(int stack_pointer_reg,
                                               int restore_reg,
                                               int clear_capture_count,
                                               int clear_capture_from,
                                               RegExpNode* on_success);
    static ActionNode* EmptyMatchCheck(int start_register,
                                       int repetition_register,
                                       int repetition_limit,
                                       RegExpNode* on_success);
    virtual void Accept(NodeVisitor* visitor);
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find, int budget, bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int filled_in,
                                      bool not_at_start) {
        return on_success()->GetQuickCheckDetails(
                                                  details, compiler, filled_in, not_at_start);
    }
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);
    ActionType action_type() { return action_type_; }
    
    virtual int GreedyLoopTextLength() { return kNodeIsTooComplexForGreedyLoops; }

  private:
    union {
        struct {
            int reg;
            int value;
        } u_store_register;
        struct {
            int reg;
        } u_increment_register;
        struct {
            int reg;
            bool is_capture;
        } u_position_register;
        struct {
            int stack_pointer_register;
            int current_position_register;
            int clear_register_count;
            int clear_register_from;
        } u_submatch;
        struct {
            int start_register;
            int repetition_register;
            int repetition_limit;
        } u_empty_match_check;
        struct {
            int range_from;
            int range_to;
        } u_clear_captures;
    } data_;
    ActionType action_type_;
    friend class DotPrinter;
};

class TextNode : public SeqRegExpNode
{
  public:
    TextNode(TextElementVector *elements,
             RegExpNode *on_success)
      : SeqRegExpNode(on_success),
        elements_(elements)
    {}

    TextNode(RegExpCharacterClass* that,
             RegExpNode* on_success)
      : SeqRegExpNode(on_success),
        elements_(alloc()->newInfallible<TextElementVector>(*alloc()))
    {
        elements_->append(TextElement::CharClass(that));
    }

    virtual void Accept(NodeVisitor* visitor);
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find, int budget, bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start);
    TextElementVector &elements() { return *elements_; }
    void MakeCaseIndependent(bool is_ascii);
    virtual int GreedyLoopTextLength();
    virtual RegExpNode* GetSuccessorOfOmnivorousTextNode(
                                                         RegExpCompiler* compiler);
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);
    void CalculateOffsets();
    virtual RegExpNode* FilterASCII(int depth, bool ignore_case);

  private:
    enum TextEmitPassType {
        NON_ASCII_MATCH,             
        SIMPLE_CHARACTER_MATCH,      
        NON_LETTER_CHARACTER_MATCH,  
        CASE_CHARACTER_MATCH,        
        CHARACTER_CLASS_MATCH        
    };
    static bool SkipPass(int pass, bool ignore_case);
    static const int kFirstRealPass = SIMPLE_CHARACTER_MATCH;
    static const int kLastPass = CHARACTER_CLASS_MATCH;
    void TextEmitPass(RegExpCompiler* compiler,
                      TextEmitPassType pass,
                      bool preloaded,
                      Trace* trace,
                      bool first_element_checked,
                      int* checked_up_to);
    int Length();
    TextElementVector *elements_;
};

class AssertionNode : public SeqRegExpNode
{
  public:
    enum AssertionType {
        AT_END,
        AT_START,
        AT_BOUNDARY,
        AT_NON_BOUNDARY,
        AFTER_NEWLINE
    };
    AssertionNode(AssertionType t, RegExpNode* on_success)
      : SeqRegExpNode(on_success), assertion_type_(t)
    {}

    static AssertionNode* AtEnd(RegExpNode* on_success) {
        return on_success->alloc()->newInfallible<AssertionNode>(AT_END, on_success);
    }
    static AssertionNode* AtStart(RegExpNode* on_success) {
        return on_success->alloc()->newInfallible<AssertionNode>(AT_START, on_success);
    }
    static AssertionNode* AtBoundary(RegExpNode* on_success) {
        return on_success->alloc()->newInfallible<AssertionNode>(AT_BOUNDARY, on_success);
    }
    static AssertionNode* AtNonBoundary(RegExpNode* on_success) {
        return on_success->alloc()->newInfallible<AssertionNode>(AT_NON_BOUNDARY, on_success);
    }
    static AssertionNode* AfterNewline(RegExpNode* on_success) {
        return on_success->alloc()->newInfallible<AssertionNode>(AFTER_NEWLINE, on_success);
    }
    virtual void Accept(NodeVisitor* visitor);
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find, int budget, bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int filled_in,
                                      bool not_at_start);
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);
    AssertionType assertion_type() { return assertion_type_; }

  private:
    void EmitBoundaryCheck(RegExpCompiler* compiler, Trace* trace);
    enum IfPrevious { kIsNonWord, kIsWord };
    void BacktrackIfPrevious(RegExpCompiler* compiler,
                             Trace* trace,
                             IfPrevious backtrack_if_previous);
    AssertionType assertion_type_;
};

class BackReferenceNode : public SeqRegExpNode
{
  public:
    BackReferenceNode(int start_reg,
                      int end_reg,
                      RegExpNode* on_success)
      : SeqRegExpNode(on_success),
        start_reg_(start_reg),
        end_reg_(end_reg)
    {}

    virtual void Accept(NodeVisitor* visitor);
    int start_register() { return start_reg_; }
    int end_register() { return end_reg_; }
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find,
                            int recursion_depth,
                            bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start) {
        return;
    }
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);

  private:
    int start_reg_;
    int end_reg_;
};

class EndNode : public RegExpNode
{
  public:
    enum Action { ACCEPT, BACKTRACK, NEGATIVE_SUBMATCH_SUCCESS };

    explicit EndNode(LifoAlloc *alloc, Action action)
      : RegExpNode(alloc), action_(action)
    {}

    virtual void Accept(NodeVisitor* visitor);
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find,
                            int recursion_depth,
                            bool not_at_start) { return 0; }
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start)
    {
        
        MOZ_CRASH("Bad call");
    }
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start) {
        
        MOZ_CRASH("Bad call");
    }

  private:
    Action action_;
};

class NegativeSubmatchSuccess : public EndNode
{
  public:
    NegativeSubmatchSuccess(LifoAlloc *alloc,
                            int stack_pointer_reg,
                            int position_reg,
                            int clear_capture_count,
                            int clear_capture_start)
      : EndNode(alloc, NEGATIVE_SUBMATCH_SUCCESS),
        stack_pointer_register_(stack_pointer_reg),
        current_position_register_(position_reg),
        clear_capture_count_(clear_capture_count),
        clear_capture_start_(clear_capture_start)
    {}

    virtual void Emit(RegExpCompiler* compiler, Trace* trace);

  private:
    int stack_pointer_register_;
    int current_position_register_;
    int clear_capture_count_;
    int clear_capture_start_;
};

class Guard
{
  public:
    enum Relation { LT, GEQ };
    Guard(int reg, Relation op, int value)
        : reg_(reg),
          op_(op),
          value_(value)
    {}

    int reg() { return reg_; }
    Relation op() { return op_; }
    int value() { return value_; }

  private:
    int reg_;
    Relation op_;
    int value_;
};

typedef Vector<Guard *, 1, LifoAllocPolicy<Infallible> > GuardVector;

class GuardedAlternative
{
  public:
    explicit GuardedAlternative(RegExpNode* node)
      : node_(node), guards_(nullptr)
    {}

    void AddGuard(LifoAlloc *alloc, Guard *guard);
    RegExpNode *node() const { return node_; }
    void set_node(RegExpNode* node) { node_ = node; }
    const GuardVector *guards() const { return guards_; }

  private:
    RegExpNode *node_;
    GuardVector *guards_;
};

typedef Vector<GuardedAlternative, 0, LifoAllocPolicy<Infallible> > GuardedAlternativeVector;

class AlternativeGeneration;

class ChoiceNode : public RegExpNode
{
  public:
    explicit ChoiceNode(LifoAlloc *alloc, int expected_size)
      : RegExpNode(alloc),
        alternatives_(*alloc),
        table_(nullptr),
        not_at_start_(false),
        being_calculated_(false)
    {
        alternatives_.reserve(expected_size);
    }

    virtual void Accept(NodeVisitor* visitor);
    void AddAlternative(GuardedAlternative node) {
        alternatives_.append(node);
    }

    GuardedAlternativeVector &alternatives() { return alternatives_; }
    DispatchTable* GetTable(bool ignore_case);
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find, int budget, bool not_at_start);
    int EatsAtLeastHelper(int still_to_find,
                          int budget,
                          RegExpNode* ignore_this_node,
                          bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start);
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);

    bool being_calculated() { return being_calculated_; }
    bool not_at_start() { return not_at_start_; }
    void set_not_at_start() { not_at_start_ = true; }
    void set_being_calculated(bool b) { being_calculated_ = b; }
    virtual bool try_to_emit_quick_check_for_alternative(int i) { return true; }
    virtual RegExpNode* FilterASCII(int depth, bool ignore_case);

  protected:
    int GreedyLoopTextLengthForAlternative(GuardedAlternative* alternative);
    GuardedAlternativeVector alternatives_;

  private:
    friend class Analysis;
    void GenerateGuard(RegExpMacroAssembler* macro_assembler,
                       Guard* guard,
                       Trace* trace);
    int CalculatePreloadCharacters(RegExpCompiler* compiler, int eats_at_least);
    void EmitOutOfLineContinuation(RegExpCompiler* compiler,
                                   Trace* trace,
                                   GuardedAlternative alternative,
                                   AlternativeGeneration* alt_gen,
                                   int preload_characters,
                                   bool next_expects_preload);
    DispatchTable* table_;

    
    
    bool not_at_start_;
    bool being_calculated_;
};

class NegativeLookaheadChoiceNode : public ChoiceNode
{
  public:
    explicit NegativeLookaheadChoiceNode(LifoAlloc *alloc,
                                         GuardedAlternative this_must_fail,
                                         GuardedAlternative then_do_this)
      : ChoiceNode(alloc, 2)
    {
        AddAlternative(this_must_fail);
        AddAlternative(then_do_this);
    }
    virtual int EatsAtLeast(int still_to_find, int budget, bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start);
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);

    
    
    
    
    
    virtual bool try_to_emit_quick_check_for_alternative(int i) { return i != 0; }
    virtual RegExpNode* FilterASCII(int depth, bool ignore_case);
};

class LoopChoiceNode : public ChoiceNode
{
  public:
    explicit LoopChoiceNode(LifoAlloc *alloc, bool body_can_be_zero_length)
      : ChoiceNode(alloc, 2),
        loop_node_(nullptr),
        continue_node_(nullptr),
        body_can_be_zero_length_(body_can_be_zero_length)
    {}

    void AddLoopAlternative(GuardedAlternative alt);
    void AddContinueAlternative(GuardedAlternative alt);
    virtual void Emit(RegExpCompiler* compiler, Trace* trace);
    virtual int EatsAtLeast(int still_to_find,  int budget, bool not_at_start);
    virtual void GetQuickCheckDetails(QuickCheckDetails* details,
                                      RegExpCompiler* compiler,
                                      int characters_filled_in,
                                      bool not_at_start);
    virtual bool FillInBMInfo(int offset,
                              int budget,
                              BoyerMooreLookahead* bm,
                              bool not_at_start);
    RegExpNode* loop_node() { return loop_node_; }
    RegExpNode* continue_node() { return continue_node_; }
    bool body_can_be_zero_length() { return body_can_be_zero_length_; }
    virtual void Accept(NodeVisitor* visitor);
    virtual RegExpNode* FilterASCII(int depth, bool ignore_case);

  private:
    
    
    
    void AddAlternative(GuardedAlternative node) {
        ChoiceNode::AddAlternative(node);
    }

    RegExpNode* loop_node_;
    RegExpNode* continue_node_;
    bool body_can_be_zero_length_;
};


























enum ContainedInLattice {
  kNotYet = 0,
  kLatticeIn = 1,
  kLatticeOut = 2,
  kLatticeUnknown = 3  
};

inline ContainedInLattice
Combine(ContainedInLattice a, ContainedInLattice b) {
    return static_cast<ContainedInLattice>(a | b);
}

ContainedInLattice
AddRange(ContainedInLattice a,
         const int* ranges,
         int ranges_size,
         Interval new_range);

class BoyerMoorePositionInfo
{
  public:
    explicit BoyerMoorePositionInfo(LifoAlloc *alloc)
      : map_(*alloc),
        map_count_(0),
        w_(kNotYet),
        s_(kNotYet),
        d_(kNotYet),
        surrogate_(kNotYet)
    {
        map_.reserve(kMapSize);
        for (int i = 0; i < kMapSize; i++)
            map_.append(false);
    }

    bool& at(int i) { return map_[i]; }

    static const int kMapSize = 128;
    static const int kMask = kMapSize - 1;

    int map_count() const { return map_count_; }

    void Set(int character);
    void SetInterval(const Interval& interval);
    void SetAll();
    bool is_non_word() { return w_ == kLatticeOut; }
    bool is_word() { return w_ == kLatticeIn; }

  private:
    Vector<bool, 0, LifoAllocPolicy<Infallible> > map_;
    int map_count_;  
    ContainedInLattice w_;  
    ContainedInLattice s_;  
    ContainedInLattice d_;  
    ContainedInLattice surrogate_;  
};

typedef Vector<BoyerMoorePositionInfo *, 1, LifoAllocPolicy<Infallible> > BoyerMoorePositionInfoVector;

class BoyerMooreLookahead
{
  public:
    BoyerMooreLookahead(LifoAlloc *alloc, size_t length, RegExpCompiler* compiler);

    int length() { return length_; }
    int max_char() { return max_char_; }
    RegExpCompiler* compiler() { return compiler_; }

    int Count(int map_number) {
        return bitmaps_[map_number]->map_count();
    }

    BoyerMoorePositionInfo* at(int i) { return bitmaps_[i]; }

    void Set(int map_number, int character) {
        if (character > max_char_) return;
        BoyerMoorePositionInfo* info = bitmaps_[map_number];
        info->Set(character);
    }

    void SetInterval(int map_number, const Interval& interval) {
        if (interval.from() > max_char_) return;
        BoyerMoorePositionInfo* info = bitmaps_[map_number];
        if (interval.to() > max_char_) {
            info->SetInterval(Interval(interval.from(), max_char_));
        } else {
            info->SetInterval(interval);
        }
    }

    void SetAll(int map_number) {
        bitmaps_[map_number]->SetAll();
    }

    void SetRest(int from_map) {
        for (int i = from_map; i < length_; i++) SetAll(i);
    }
    bool EmitSkipInstructions(RegExpMacroAssembler* masm);

    bool CheckOverRecursed();

  private:
    
    
    
    
    int length_;
    RegExpCompiler* compiler_;

    
    int max_char_;
    BoyerMoorePositionInfoVector bitmaps_;

    int GetSkipTable(int min_lookahead,
                     int max_lookahead,
                     uint8_t *boolean_skip_table);
    bool FindWorthwhileInterval(int* from, int* to);
    int FindBestInterval(int max_number_of_chars, int old_biggest_points, int* from, int* to);
};












class Trace
{
  public:
    
    
    enum TriBool {
        UNKNOWN = -1, FALSE_VALUE = 0, TRUE_VALUE = 1
    };

    class DeferredAction {
      public:
        DeferredAction(ActionNode::ActionType action_type, int reg)
          : action_type_(action_type), reg_(reg), next_(nullptr)
        {}

        DeferredAction* next() { return next_; }
        bool Mentions(int reg);
        int reg() { return reg_; }
        ActionNode::ActionType action_type() { return action_type_; }
      private:
        ActionNode::ActionType action_type_;
        int reg_;
        DeferredAction* next_;
        friend class Trace;
    };

    class DeferredCapture : public DeferredAction {
      public:
        DeferredCapture(int reg, bool is_capture, Trace* trace)
          : DeferredAction(ActionNode::STORE_POSITION, reg),
            cp_offset_(trace->cp_offset()),
            is_capture_(is_capture)
        {}

        int cp_offset() { return cp_offset_; }
        bool is_capture() { return is_capture_; }
      private:
        int cp_offset_;
        bool is_capture_;
        void set_cp_offset(int cp_offset) { cp_offset_ = cp_offset; }
    };

    class DeferredSetRegister : public DeferredAction {
      public:
        DeferredSetRegister(int reg, int value)
          : DeferredAction(ActionNode::SET_REGISTER, reg),
            value_(value)
        {}
        int value() { return value_; }
      private:
        int value_;
    };

    class DeferredClearCaptures : public DeferredAction {
      public:
        explicit DeferredClearCaptures(Interval range)
          : DeferredAction(ActionNode::CLEAR_CAPTURES, -1),
            range_(range)
        {}

        Interval range() { return range_; }
      private:
        Interval range_;
    };

    class DeferredIncrementRegister : public DeferredAction {
      public:
        explicit DeferredIncrementRegister(int reg)
          : DeferredAction(ActionNode::INCREMENT_REGISTER, reg)
        {}
    };

    Trace()
      : cp_offset_(0),
        actions_(nullptr),
        backtrack_(nullptr),
        stop_node_(nullptr),
        loop_label_(nullptr),
        characters_preloaded_(0),
        bound_checked_up_to_(0),
        flush_budget_(100),
        at_start_(UNKNOWN)
    {}

    
    
    
    
    void Flush(RegExpCompiler* compiler, RegExpNode* successor);

    int cp_offset() { return cp_offset_; }
    DeferredAction* actions() { return actions_; }

    
    
    
    
    
    
    
    
    
    
    bool is_trivial() {
        return backtrack_ == nullptr &&
            actions_ == nullptr &&
            cp_offset_ == 0 &&
            characters_preloaded_ == 0 &&
            bound_checked_up_to_ == 0 &&
            quick_check_performed_.characters() == 0 &&
            at_start_ == UNKNOWN;
    }

    TriBool at_start() { return at_start_; }
    void set_at_start(bool at_start) {
        at_start_ = at_start ? TRUE_VALUE : FALSE_VALUE;
    }
    jit::Label* backtrack() { return backtrack_; }
    jit::Label* loop_label() { return loop_label_; }
    RegExpNode* stop_node() { return stop_node_; }
    int characters_preloaded() { return characters_preloaded_; }
    int bound_checked_up_to() { return bound_checked_up_to_; }
    int flush_budget() { return flush_budget_; }
    QuickCheckDetails* quick_check_performed() { return &quick_check_performed_; }
    bool mentions_reg(int reg);

    
    
    
    bool GetStoredPosition(int reg, int* cp_offset);

    
    
    void add_action(DeferredAction* new_action) {
        JS_ASSERT(new_action->next_ == nullptr);
        new_action->next_ = actions_;
        actions_ = new_action;
    }

    void set_backtrack(jit::Label* backtrack) { backtrack_ = backtrack; }
    void set_stop_node(RegExpNode* node) { stop_node_ = node; }
    void set_loop_label(jit::Label* label) { loop_label_ = label; }
    void set_characters_preloaded(int count) { characters_preloaded_ = count; }
    void set_bound_checked_up_to(int to) { bound_checked_up_to_ = to; }
    void set_flush_budget(int to) { flush_budget_ = to; }
    void set_quick_check_performed(QuickCheckDetails* d) {
        quick_check_performed_ = *d;
    }
    void InvalidateCurrentCharacter();
    void AdvanceCurrentPositionInTrace(int by, RegExpCompiler* compiler);

  private:
    int FindAffectedRegisters(LifoAlloc *alloc, OutSet* affected_registers);
    void PerformDeferredActions(LifoAlloc *alloc,
                                RegExpMacroAssembler* macro,
                                int max_register,
                                OutSet& affected_registers,
                                OutSet* registers_to_pop,
                                OutSet* registers_to_clear);
    void RestoreAffectedRegisters(RegExpMacroAssembler* macro,
                                  int max_register,
                                  OutSet& registers_to_pop,
                                  OutSet& registers_to_clear);
    int cp_offset_;
    DeferredAction* actions_;
    jit::Label* backtrack_;
    RegExpNode* stop_node_;
    jit::Label* loop_label_;
    int characters_preloaded_;
    int bound_checked_up_to_;
    QuickCheckDetails quick_check_performed_;
    int flush_budget_;
    TriBool at_start_;
};

class NodeVisitor
{
  public:
    virtual ~NodeVisitor() { }
#define DECLARE_VISIT(Type)                                          \
    virtual void Visit##Type(Type##Node* that) = 0;
    FOR_EACH_NODE_TYPE(DECLARE_VISIT)
#undef DECLARE_VISIT
    virtual void VisitLoopChoice(LoopChoiceNode* that) { VisitChoice(that); }
};













class Analysis : public NodeVisitor
{
  public:
    Analysis(JSContext *cx, bool ignore_case, bool is_ascii)
      : cx(cx),
        ignore_case_(ignore_case),
        is_ascii_(is_ascii),
        error_message_(nullptr)
    {}

    void EnsureAnalyzed(RegExpNode* node);

#define DECLARE_VISIT(Type)                     \
    virtual void Visit##Type(Type##Node* that);
    FOR_EACH_NODE_TYPE(DECLARE_VISIT)
#undef DECLARE_VISIT
    virtual void VisitLoopChoice(LoopChoiceNode* that);

    bool has_failed() { return error_message_ != nullptr; }
    const char* errorMessage() {
        JS_ASSERT(error_message_ != nullptr);
        return error_message_;
    }
    void fail(const char* error_message) {
        error_message_ = error_message;
    }

  private:
    JSContext *cx;
    bool ignore_case_;
    bool is_ascii_;
    const char* error_message_;

    Analysis(Analysis &) MOZ_DELETE;
    void operator=(Analysis &) MOZ_DELETE;
};

} }  

#endif
