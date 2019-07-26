









#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/parseerr.h"
#include "unicode/parsepos.h"
#include "unicode/putil.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/uniset.h"
#include "unicode/utf16.h"
#include "cstring.h"
#include "funcrepl.h"
#include "hash.h"
#include "quant.h"
#include "rbt.h"
#include "rbt_data.h"
#include "rbt_pars.h"
#include "rbt_rule.h"
#include "strmatch.h"
#include "strrepl.h"
#include "unicode/symtable.h"
#include "tridpars.h"
#include "uvector.h"
#include "hash.h"
#include "patternprops.h"
#include "util.h"
#include "cmemory.h"
#include "uprops.h"
#include "putilimp.h"


#define VARIABLE_DEF_OP ((UChar)0x003D) /*=*/
#define FORWARD_RULE_OP ((UChar)0x003E) /*>*/
#define REVERSE_RULE_OP ((UChar)0x003C) /*<*/
#define FWDREV_RULE_OP  ((UChar)0x007E) /*~*/ // internal rep of <> op


#define QUOTE             ((UChar)0x0027) /*'*/
#define ESCAPE            ((UChar)0x005C) /*\*/
#define END_OF_RULE       ((UChar)0x003B) /*;*/
#define RULE_COMMENT_CHAR ((UChar)0x0023) /*#*/

#define SEGMENT_OPEN       ((UChar)0x0028) /*(*/
#define SEGMENT_CLOSE      ((UChar)0x0029) /*)*/
#define CONTEXT_ANTE       ((UChar)0x007B) /*{*/
#define CONTEXT_POST       ((UChar)0x007D) /*}*/
#define CURSOR_POS         ((UChar)0x007C) /*|*/
#define CURSOR_OFFSET      ((UChar)0x0040) /*@*/
#define ANCHOR_START       ((UChar)0x005E) /*^*/
#define KLEENE_STAR        ((UChar)0x002A) /***/
#define ONE_OR_MORE        ((UChar)0x002B) /*+*/
#define ZERO_OR_ONE        ((UChar)0x003F) /*?*/

#define DOT                ((UChar)46)     /*.*/

static const UChar DOT_SET[] = { 
    91, 94, 91, 58, 90, 112, 58, 93, 91, 58, 90,
    108, 58, 93, 92, 114, 92, 110, 36, 93, 0
};


#define FUNCTION           ((UChar)38)     /*&*/




#define ALT_REVERSE_RULE_OP ((UChar)0x2190) // Left Arrow
#define ALT_FORWARD_RULE_OP ((UChar)0x2192) // Right Arrow
#define ALT_FWDREV_RULE_OP  ((UChar)0x2194) // Left Right Arrow
#define ALT_FUNCTION        ((UChar)0x2206) // Increment (~Greek Capital Delta)


static const UChar ILLEGAL_TOP[] = {41,0}; 


static const UChar ILLEGAL_SEG[] = {123,125,124,64,0}; 


static const UChar ILLEGAL_FUNC[] = {94,40,46,42,43,63,123,125,124,64,0}; 





static const UChar gOPERATORS[] = { 
    VARIABLE_DEF_OP, FORWARD_RULE_OP, REVERSE_RULE_OP,
    ALT_FORWARD_RULE_OP, ALT_REVERSE_RULE_OP, ALT_FWDREV_RULE_OP,
    0
};

static const UChar HALF_ENDERS[] = { 
    VARIABLE_DEF_OP, FORWARD_RULE_OP, REVERSE_RULE_OP,
    ALT_FORWARD_RULE_OP, ALT_REVERSE_RULE_OP, ALT_FWDREV_RULE_OP,
    END_OF_RULE,
    0
};


static const int32_t ID_TOKEN_LEN = 2;
static const UChar   ID_TOKEN[]   = { 0x3A, 0x3A }; 










U_NAMESPACE_BEGIN











class ParseData : public UMemory, public SymbolTable {
public:
    const TransliterationRuleData* data; 

    const UVector* variablesVector; 

    const Hashtable* variableNames; 

    ParseData(const TransliterationRuleData* data = 0,
              const UVector* variablesVector = 0,
              const Hashtable* variableNames = 0);

    virtual ~ParseData();

    virtual const UnicodeString* lookup(const UnicodeString& s) const;

    virtual const UnicodeFunctor* lookupMatcher(UChar32 ch) const;

    virtual UnicodeString parseReference(const UnicodeString& text,
                                         ParsePosition& pos, int32_t limit) const;
    



    UBool isMatcher(UChar32 ch);

    



    UBool isReplacer(UChar32 ch);

private:
    ParseData(const ParseData &other); 
    ParseData &operator=(const ParseData &other); 
};

ParseData::ParseData(const TransliterationRuleData* d,
                     const UVector* sets,
                     const Hashtable* vNames) :
    data(d), variablesVector(sets), variableNames(vNames) {}

ParseData::~ParseData() {}




const UnicodeString* ParseData::lookup(const UnicodeString& name) const {
    return (const UnicodeString*) variableNames->get(name);
}




const UnicodeFunctor* ParseData::lookupMatcher(UChar32 ch) const {
    
    
    const UnicodeFunctor* set = NULL;
    int32_t i = ch - data->variablesBase;
    if (i >= 0 && i < variablesVector->size()) {
        int32_t i = ch - data->variablesBase;
        set = (i < variablesVector->size()) ?
            (UnicodeFunctor*) variablesVector->elementAt(i) : 0;
    }
    return set;
}





UnicodeString ParseData::parseReference(const UnicodeString& text,
                                        ParsePosition& pos, int32_t limit) const {
    int32_t start = pos.getIndex();
    int32_t i = start;
    UnicodeString result;
    while (i < limit) {
        UChar c = text.charAt(i);
        if ((i==start && !u_isIDStart(c)) || !u_isIDPart(c)) {
            break;
        }
        ++i;
    }
    if (i == start) { 
        return result; 
    }
    pos.setIndex(i);
    text.extractBetween(start, i, result);
    return result;
}

UBool ParseData::isMatcher(UChar32 ch) {
    
    
    int32_t i = ch - data->variablesBase;
    if (i >= 0 && i < variablesVector->size()) {
        UnicodeFunctor *f = (UnicodeFunctor*) variablesVector->elementAt(i);
        return f != NULL && f->toMatcher() != NULL;
    }
    return TRUE;
}





UBool ParseData::isReplacer(UChar32 ch) {
    
    
    int i = ch - data->variablesBase;
    if (i >= 0 && i < variablesVector->size()) {
        UnicodeFunctor *f = (UnicodeFunctor*) variablesVector->elementAt(i);
        return f != NULL && f->toReplacer() != NULL;
    }
    return TRUE;
}










class RuleHalf : public UMemory {

public:

    UnicodeString text;

    int32_t cursor; 
    int32_t ante;   
    int32_t post;   

    
    
    
    
    
    
    
    
    
    int32_t cursorOffset; 

    
    
    int32_t cursorOffsetPos;

    UBool anchorStart;
    UBool anchorEnd;

    



    int32_t nextSegmentNumber;

    TransliteratorParser& parser;

    
    

    RuleHalf(TransliteratorParser& parser);
    ~RuleHalf();

    int32_t parse(const UnicodeString& rule, int32_t pos, int32_t limit, UErrorCode& status);

    int32_t parseSection(const UnicodeString& rule, int32_t pos, int32_t limit,
                         UnicodeString& buf,
                         const UnicodeString& illegal,
                         UBool isSegment,
                         UErrorCode& status);

    


    void removeContext();

    



    UBool isValidOutput(TransliteratorParser& parser);

    



    UBool isValidInput(TransliteratorParser& parser);

    int syntaxError(UErrorCode code,
                    const UnicodeString& rule,
                    int32_t start,
                    UErrorCode& status) {
        return parser.syntaxError(code, rule, start, status);
    }

private:
    
    RuleHalf(const RuleHalf&);
    RuleHalf& operator=(const RuleHalf&);
};

RuleHalf::RuleHalf(TransliteratorParser& p) :
    parser(p)
{
    cursor = -1;
    ante = -1;
    post = -1;
    cursorOffset = 0;
    cursorOffsetPos = 0;
    anchorStart = anchorEnd = FALSE;
    nextSegmentNumber = 1;
}

RuleHalf::~RuleHalf() {
}







int32_t RuleHalf::parse(const UnicodeString& rule, int32_t pos, int32_t limit, UErrorCode& status) {
    int32_t start = pos;
    text.truncate(0);
    pos = parseSection(rule, pos, limit, text, UnicodeString(TRUE, ILLEGAL_TOP, -1), FALSE, status);

    if (cursorOffset > 0 && cursor != cursorOffsetPos) {
        return syntaxError(U_MISPLACED_CURSOR_OFFSET, rule, start, status);
    }
    
    return pos;
}
 























int32_t RuleHalf::parseSection(const UnicodeString& rule, int32_t pos, int32_t limit,
                               UnicodeString& buf,
                               const UnicodeString& illegal,
                               UBool isSegment, UErrorCode& status) {
    int32_t start = pos;
    ParsePosition pp;
    UnicodeString scratch;
    UBool done = FALSE;
    int32_t quoteStart = -1; 
    int32_t quoteLimit = -1;
    int32_t varStart = -1; 
    int32_t varLimit = -1;
    int32_t bufStart = buf.length();
    
    while (pos < limit && !done) {
        
        
        UChar c = rule.charAt(pos++);
        if (PatternProps::isWhiteSpace(c)) {
            
            
            
            continue;
        }
        if (u_strchr(HALF_ENDERS, c) != NULL) {
            if (isSegment) {
                
                return syntaxError(U_UNCLOSED_SEGMENT, rule, start, status);
            }
            break;
        }
        if (anchorEnd) {
            
            return syntaxError(U_MALFORMED_VARIABLE_REFERENCE, rule, start, status);
        }
        if (UnicodeSet::resemblesPattern(rule, pos-1)) {
            pp.setIndex(pos-1); 
            buf.append(parser.parseSet(rule, pp, status));
            if (U_FAILURE(status)) {
                return syntaxError(U_MALFORMED_SET, rule, start, status);
            }
            pos = pp.getIndex();                    
            continue;
        }
        
        if (c == ESCAPE) {
            if (pos == limit) {
                return syntaxError(U_TRAILING_BACKSLASH, rule, start, status);
            }
            UChar32 escaped = rule.unescapeAt(pos); 
            if (escaped == (UChar32) -1) {
                return syntaxError(U_MALFORMED_UNICODE_ESCAPE, rule, start, status);
            }
            if (!parser.checkVariableRange(escaped)) {
                return syntaxError(U_VARIABLE_RANGE_OVERLAP, rule, start, status);
            }
            buf.append(escaped);
            continue;
        }
        
        if (c == QUOTE) {
            int32_t iq = rule.indexOf(QUOTE, pos);
            if (iq == pos) {
                buf.append(c); 
                ++pos;
            } else {
                





                quoteStart = buf.length();
                for (;;) {
                    if (iq < 0) {
                        return syntaxError(U_UNTERMINATED_QUOTE, rule, start, status);
                    }
                    scratch.truncate(0);
                    rule.extractBetween(pos, iq, scratch);
                    buf.append(scratch);
                    pos = iq+1;
                    if (pos < limit && rule.charAt(pos) == QUOTE) {
                        
                        iq = rule.indexOf(QUOTE, pos+1);
                        
                    } else {
                        break;
                    }
                }
                quoteLimit = buf.length();

                for (iq=quoteStart; iq<quoteLimit; ++iq) {
                    if (!parser.checkVariableRange(buf.charAt(iq))) {
                        return syntaxError(U_VARIABLE_RANGE_OVERLAP, rule, start, status);
                    }
                }
            }
            continue;
        }

        if (!parser.checkVariableRange(c)) {
            return syntaxError(U_VARIABLE_RANGE_OVERLAP, rule, start, status);
        }

        if (illegal.indexOf(c) >= 0) {
            syntaxError(U_ILLEGAL_CHARACTER, rule, start, status);
        }

        switch (c) {
                    
        
        
        
        case ANCHOR_START:
            if (buf.length() == 0 && !anchorStart) {
                anchorStart = TRUE;
            } else {
              return syntaxError(U_MISPLACED_ANCHOR_START,
                                 rule, start, status);
            }
          break;
        case SEGMENT_OPEN:
            {
                
                
                int32_t bufSegStart = buf.length();
                
                
                
                
                int32_t segmentNumber = nextSegmentNumber++; 
                
                
                pos = parseSection(rule, pos, limit, buf, UnicodeString(TRUE, ILLEGAL_SEG, -1), TRUE, status);
                
                
                
                
                
                StringMatcher* m =
                    new StringMatcher(buf, bufSegStart, buf.length(),
                                      segmentNumber, *parser.curData);
                if (m == NULL) {
                    return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
                }
                
                
                parser.setSegmentObject(segmentNumber, m, status);
                buf.truncate(bufSegStart);
                buf.append(parser.getSegmentStandin(segmentNumber, status));
            }
            break;
        case FUNCTION:
        case ALT_FUNCTION:
            {
                int32_t iref = pos;
                TransliteratorIDParser::SingleID* single =
                    TransliteratorIDParser::parseFilterID(rule, iref);
                
                if (single == NULL ||
                    !ICU_Utility::parseChar(rule, iref, SEGMENT_OPEN)) {
                    return syntaxError(U_INVALID_FUNCTION, rule, start, status);
                }
                
                Transliterator *t = single->createInstance();
                delete single;
                if (t == NULL) {
                    return syntaxError(U_INVALID_FUNCTION, rule, start, status);
                }
                
                
                
                int32_t bufSegStart = buf.length();
                
                
                pos = parseSection(rule, iref, limit, buf, UnicodeString(TRUE, ILLEGAL_FUNC, -1), TRUE, status);
                
                
                
                UnicodeString output;
                buf.extractBetween(bufSegStart, buf.length(), output);
                FunctionReplacer *r =
                    new FunctionReplacer(t, new StringReplacer(output, parser.curData));
                if (r == NULL) {
                    return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
                }
                
                
                buf.truncate(bufSegStart);
                buf.append(parser.generateStandInFor(r, status));
            }
            break;
        case SymbolTable::SYMBOL_REF:
            
            {
                
                
                
                
                if (pos == limit) {
                    
                    
                    anchorEnd = TRUE;
                    break;
                }
                
                c = rule.charAt(pos);
                int32_t r = u_digit(c, 10);
                if (r >= 1 && r <= 9) {
                    r = ICU_Utility::parseNumber(rule, pos, 10);
                    if (r < 0) {
                        return syntaxError(U_UNDEFINED_SEGMENT_REFERENCE,
                                           rule, start, status);
                    }
                    buf.append(parser.getSegmentStandin(r, status));
                } else {
                    pp.setIndex(pos);
                    UnicodeString name = parser.parseData->
                                    parseReference(rule, pp, limit);
                    if (name.length() == 0) {
                        
                        
                        
                        
                        
                        anchorEnd = TRUE;
                        break;
                    }
                    pos = pp.getIndex();
                    
                    
                    
                    
                    varStart = buf.length();
                    parser.appendVariableDef(name, buf, status);
                    varLimit = buf.length();
                }
            }
            break;
        case DOT:
            buf.append(parser.getDotStandIn(status));
            break;
        case KLEENE_STAR:
        case ONE_OR_MORE:
        case ZERO_OR_ONE:
            
            
            
            
            
            
            {
                if (isSegment && buf.length() == bufStart) {
                    
                    return syntaxError(U_MISPLACED_QUANTIFIER, rule, start, status);
                }

                int32_t qstart, qlimit;
                
                
                if (buf.length() == quoteLimit) {
                    
                    qstart = quoteStart;
                    qlimit = quoteLimit;
                } else if (buf.length() == varLimit) {
                    
                    qstart = varStart;
                    qlimit = varLimit;
                } else {
                    
                    
                    qstart = buf.length() - 1;
                    qlimit = qstart + 1;
                }

                UnicodeFunctor *m =
                    new StringMatcher(buf, qstart, qlimit, 0, *parser.curData);
                if (m == NULL) {
                    return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
                }
                int32_t min = 0;
                int32_t max = Quantifier::MAX;
                switch (c) {
                case ONE_OR_MORE:
                    min = 1;
                    break;
                case ZERO_OR_ONE:
                    min = 0;
                    max = 1;
                    break;
                
                
                }
                m = new Quantifier(m, min, max);
                if (m == NULL) {
                    return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
                }
                buf.truncate(qstart);
                buf.append(parser.generateStandInFor(m, status));
            }
            break;

        
        
        
        case SEGMENT_CLOSE:
            
            
            done = TRUE;
            break;

        
        
        
        case CONTEXT_ANTE:
            if (ante >= 0) {
                return syntaxError(U_MULTIPLE_ANTE_CONTEXTS, rule, start, status);
            }
            ante = buf.length();
            break;
        case CONTEXT_POST:
            if (post >= 0) {
                return syntaxError(U_MULTIPLE_POST_CONTEXTS, rule, start, status);
            }
            post = buf.length();
            break;
        case CURSOR_POS:
            if (cursor >= 0) {
                return syntaxError(U_MULTIPLE_CURSORS, rule, start, status);
            }
            cursor = buf.length();
            break;
        case CURSOR_OFFSET:
            if (cursorOffset < 0) {
                if (buf.length() > 0) {
                    return syntaxError(U_MISPLACED_CURSOR_OFFSET, rule, start, status);
                }
                --cursorOffset;
            } else if (cursorOffset > 0) {
                if (buf.length() != cursorOffsetPos || cursor >= 0) {
                    return syntaxError(U_MISPLACED_CURSOR_OFFSET, rule, start, status);
                }
                ++cursorOffset;
            } else {
                if (cursor == 0 && buf.length() == 0) {
                    cursorOffset = -1;
                } else if (cursor < 0) {
                    cursorOffsetPos = buf.length();
                    cursorOffset = 1;
                } else {
                    return syntaxError(U_MISPLACED_CURSOR_OFFSET, rule, start, status);
                }
            }
            break;


        
        
        
        default:
            
            
            
            if (c >= 0x0021 && c <= 0x007E &&
                !((c >= 0x0030 && c <= 0x0039) ||
                  (c >= 0x0041 && c <= 0x005A) ||
                  (c >= 0x0061 && c <= 0x007A))) {
                return syntaxError(U_UNQUOTED_SPECIAL, rule, start, status);
            }
            buf.append(c);
            break;
        }
    }

    return pos;
}




void RuleHalf::removeContext() {
    
    
    if (post >= 0) {
        text.remove(post);
    }
    if (ante >= 0) {
        text.removeBetween(0, ante);
    }
    ante = post = -1;
    anchorStart = anchorEnd = FALSE;
}





UBool RuleHalf::isValidOutput(TransliteratorParser& transParser) {
    for (int32_t i=0; i<text.length(); ) {
        UChar32 c = text.char32At(i);
        i += U16_LENGTH(c);
        if (!transParser.parseData->isReplacer(c)) {
            return FALSE;
        }
    }
    return TRUE;
}





UBool RuleHalf::isValidInput(TransliteratorParser& transParser) {
    for (int32_t i=0; i<text.length(); ) {
        UChar32 c = text.char32At(i);
        i += U16_LENGTH(c);
        if (!transParser.parseData->isMatcher(c)) {
            return FALSE;
        }
    }
    return TRUE;
}








TransliteratorParser::TransliteratorParser(UErrorCode &statusReturn) :
dataVector(statusReturn),
idBlockVector(statusReturn),
variablesVector(statusReturn),
segmentObjects(statusReturn)
{
    idBlockVector.setDeleter(uprv_deleteUObject);
    curData = NULL;
    compoundFilter = NULL;
    parseData = NULL;
    variableNames.setValueDeleter(uprv_deleteUObject);
}




TransliteratorParser::~TransliteratorParser() {
    while (!dataVector.isEmpty())
        delete (TransliterationRuleData*)(dataVector.orphanElementAt(0));
    delete compoundFilter;
    delete parseData;
    while (!variablesVector.isEmpty())
        delete (UnicodeFunctor*)variablesVector.orphanElementAt(0);
}

void
TransliteratorParser::parse(const UnicodeString& rules,
                            UTransDirection transDirection,
                            UParseError& pe,
                            UErrorCode& ec) {
    if (U_SUCCESS(ec)) {
        parseRules(rules, transDirection, ec);
        pe = parseError;
    }
}



 
UnicodeSet* TransliteratorParser::orphanCompoundFilter() {
    UnicodeSet* f = compoundFilter;
    compoundFilter = NULL;
    return f;
}













void TransliteratorParser::parseRules(const UnicodeString& rule,
                                      UTransDirection theDirection,
                                      UErrorCode& status)
{
    
    uprv_memset(&parseError, 0, sizeof(parseError));
    parseError.line = parseError.offset = -1;

    UBool parsingIDs = TRUE;
    int32_t ruleCount = 0;
    
    while (!dataVector.isEmpty()) {
        delete (TransliterationRuleData*)(dataVector.orphanElementAt(0));
    }
    if (U_FAILURE(status)) {
        return;
    }

    idBlockVector.removeAllElements();
    curData = NULL;
    direction = theDirection;
    ruleCount = 0;

    delete compoundFilter;
    compoundFilter = NULL;

    while (!variablesVector.isEmpty()) {
        delete (UnicodeFunctor*)variablesVector.orphanElementAt(0);
    }
    variableNames.removeAll();
    parseData = new ParseData(0, &variablesVector, &variableNames);
    if (parseData == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    dotStandIn = (UChar) -1;

    UnicodeString *tempstr = NULL; 
    UnicodeString str; 
    UnicodeString idBlockResult;
    int32_t pos = 0;
    int32_t limit = rule.length();

    
    
    
    
    
    compoundFilter = NULL;
    int32_t compoundFilterOffset = -1;

    while (pos < limit && U_SUCCESS(status)) {
        UChar c = rule.charAt(pos++);
        if (PatternProps::isWhiteSpace(c)) {
            
            continue;
        }
        
        if (c == RULE_COMMENT_CHAR) {
            pos = rule.indexOf((UChar)0x000A , pos) + 1;
            if (pos == 0) {
                break; 
            }
            continue; 
        }

        
        if (c == END_OF_RULE)
            continue;

        
        ++ruleCount;
        
        
        
        --pos;
        
        
        if ((pos + ID_TOKEN_LEN + 1) <= limit &&
                rule.compare(pos, ID_TOKEN_LEN, ID_TOKEN) == 0) {
            pos += ID_TOKEN_LEN;
            c = rule.charAt(pos);
            while (PatternProps::isWhiteSpace(c) && pos < limit) {
                ++pos;
                c = rule.charAt(pos);
            }

            int32_t p = pos;
            
            if (!parsingIDs) {
                if (curData != NULL) {
                    if (direction == UTRANS_FORWARD)
                        dataVector.addElement(curData, status);
                    else
                        dataVector.insertElementAt(curData, 0, status);
                    curData = NULL;
                }
                parsingIDs = TRUE;
            }

            TransliteratorIDParser::SingleID* id =
                TransliteratorIDParser::parseSingleID(rule, p, direction, status);
            if (p != pos && ICU_Utility::parseChar(rule, p, END_OF_RULE)) {
                

                if (direction == UTRANS_FORWARD) {
                    idBlockResult.append(id->canonID).append(END_OF_RULE);
                } else {
                    idBlockResult.insert(0, END_OF_RULE);
                    idBlockResult.insert(0, id->canonID);
                }

            } else {
                
                int32_t withParens = -1;
                UnicodeSet* f = TransliteratorIDParser::parseGlobalFilter(rule, p, direction, withParens, NULL);
                if (f != NULL) {
                    if (ICU_Utility::parseChar(rule, p, END_OF_RULE)
                        && (direction == UTRANS_FORWARD) == (withParens == 0))
                    {
                        if (compoundFilter != NULL) {
                            
                            syntaxError(U_MULTIPLE_COMPOUND_FILTERS, rule, pos, status);
                            delete f;
                        } else {
                            compoundFilter = f;
                            compoundFilterOffset = ruleCount;
                        }
                    } else {
                        delete f;
                    }
                } else {
                    
                    
                    syntaxError(U_INVALID_ID, rule, pos, status);
                }
            }
            delete id;
            pos = p;
        } else {
            if (parsingIDs) {
                tempstr = new UnicodeString(idBlockResult);
                
                if (tempstr == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                if (direction == UTRANS_FORWARD)
                    idBlockVector.addElement(tempstr, status);
                else
                    idBlockVector.insertElementAt(tempstr, 0, status);
                idBlockResult.remove();
                parsingIDs = FALSE;
                curData = new TransliterationRuleData(status);
                
                if (curData == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                parseData->data = curData;

                
                
                
                
                setVariableRange(0xF000, 0xF8FF, status);
            }

            if (resemblesPragma(rule, pos, limit)) {
                int32_t ppp = parsePragma(rule, pos, limit, status);
                if (ppp < 0) {
                    syntaxError(U_MALFORMED_PRAGMA, rule, pos, status);
                }
                pos = ppp;
            
            } else {
                pos = parseRule(rule, pos, limit, status);
            }
        }
    }

    if (parsingIDs && idBlockResult.length() > 0) {
        tempstr = new UnicodeString(idBlockResult);
        
        if (tempstr == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if (direction == UTRANS_FORWARD)
            idBlockVector.addElement(tempstr, status);
        else
            idBlockVector.insertElementAt(tempstr, 0, status);
    }
    else if (!parsingIDs && curData != NULL) {
        if (direction == UTRANS_FORWARD)
            dataVector.addElement(curData, status);
        else
            dataVector.insertElementAt(curData, 0, status);
    }
    
    if (U_SUCCESS(status)) {
        
        int32_t i, dataVectorSize = dataVector.size();
        for (i = 0; i < dataVectorSize; i++) {
            TransliterationRuleData* data = (TransliterationRuleData*)dataVector.elementAt(i);
            data->variablesLength = variablesVector.size();
            if (data->variablesLength == 0) {
                data->variables = 0;
            } else {
                data->variables = (UnicodeFunctor**)uprv_malloc(data->variablesLength * sizeof(UnicodeFunctor*));
                
                if (data->variables == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                data->variablesAreOwned = (i == 0);
            }

            for (int32_t j = 0; j < data->variablesLength; j++) {
                data->variables[j] =
                    ((UnicodeSet*)variablesVector.elementAt(j));
            }
            
            data->variableNames.removeAll();
            int32_t pos = -1;
            const UHashElement* he = variableNames.nextElement(pos);
            while (he != NULL) {
                UnicodeString* tempus = (UnicodeString*)(((UnicodeString*)(he->value.pointer))->clone());
                if (tempus == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
                data->variableNames.put(*((UnicodeString*)(he->key.pointer)),
                    tempus, status);
                he = variableNames.nextElement(pos);
            }
        }
        variablesVector.removeAllElements();   

        
        if (compoundFilter != NULL) {
            if ((direction == UTRANS_FORWARD && compoundFilterOffset != 1) ||
                (direction == UTRANS_REVERSE && compoundFilterOffset != ruleCount)) {
                status = U_MISPLACED_COMPOUND_FILTER;
            }
        }        

        for (i = 0; i < dataVectorSize; i++) {
            TransliterationRuleData* data = (TransliterationRuleData*)dataVector.elementAt(i);
            data->ruleSet.freeze(parseError, status);
        }
        if (idBlockVector.size() == 1 && ((UnicodeString*)idBlockVector.elementAt(0))->isEmpty()) {
            idBlockVector.removeElementAt(0);
        }
    }
}




void TransliteratorParser::setVariableRange(int32_t start, int32_t end, UErrorCode& status) {
    if (start > end || start < 0 || end > 0xFFFF) {
        status = U_MALFORMED_PRAGMA;
        return;
    }
    
    curData->variablesBase = (UChar) start;
    if (dataVector.size() == 0) {
        variableNext = (UChar) start;
        variableLimit = (UChar) (end + 1);
    }
}






UBool TransliteratorParser::checkVariableRange(UChar32 ch) const {
    return !(ch >= curData->variablesBase && ch < variableLimit);
}





void TransliteratorParser::pragmaMaximumBackup(int32_t ) {
    
}





void TransliteratorParser::pragmaNormalizeRules(UNormalizationMode ) {
    
}

static const UChar PRAGMA_USE[] = {0x75,0x73,0x65,0x20,0}; 

static const UChar PRAGMA_VARIABLE_RANGE[] = {0x7E,0x76,0x61,0x72,0x69,0x61,0x62,0x6C,0x65,0x20,0x72,0x61,0x6E,0x67,0x65,0x20,0x23,0x20,0x23,0x7E,0x3B,0}; 

static const UChar PRAGMA_MAXIMUM_BACKUP[] = {0x7E,0x6D,0x61,0x78,0x69,0x6D,0x75,0x6D,0x20,0x62,0x61,0x63,0x6B,0x75,0x70,0x20,0x23,0x7E,0x3B,0}; 

static const UChar PRAGMA_NFD_RULES[] = {0x7E,0x6E,0x66,0x64,0x20,0x72,0x75,0x6C,0x65,0x73,0x7E,0x3B,0}; 

static const UChar PRAGMA_NFC_RULES[] = {0x7E,0x6E,0x66,0x63,0x20,0x72,0x75,0x6C,0x65,0x73,0x7E,0x3B,0}; 







UBool TransliteratorParser::resemblesPragma(const UnicodeString& rule, int32_t pos, int32_t limit) {
    
    return ICU_Utility::parsePattern(rule, pos, limit, UnicodeString(TRUE, PRAGMA_USE, 4), NULL) >= 0;
}










int32_t TransliteratorParser::parsePragma(const UnicodeString& rule, int32_t pos, int32_t limit, UErrorCode& status) {
    int32_t array[2];
    
    
    
    
    pos += 4;
    
    
    
    
    
    
    int p = ICU_Utility::parsePattern(rule, pos, limit, UnicodeString(TRUE, PRAGMA_VARIABLE_RANGE, -1), array);
    if (p >= 0) {
        setVariableRange(array[0], array[1], status);
        return p;
    }
    
    p = ICU_Utility::parsePattern(rule, pos, limit, UnicodeString(TRUE, PRAGMA_MAXIMUM_BACKUP, -1), array);
    if (p >= 0) {
        pragmaMaximumBackup(array[0]);
        return p;
    }
    
    p = ICU_Utility::parsePattern(rule, pos, limit, UnicodeString(TRUE, PRAGMA_NFD_RULES, -1), NULL);
    if (p >= 0) {
        pragmaNormalizeRules(UNORM_NFD);
        return p;
    }
    
    p = ICU_Utility::parsePattern(rule, pos, limit, UnicodeString(TRUE, PRAGMA_NFC_RULES, -1), NULL);
    if (p >= 0) {
        pragmaNormalizeRules(UNORM_NFC);
        return p;
    }
    
    
    return -1;
}














int32_t TransliteratorParser::parseRule(const UnicodeString& rule, int32_t pos, int32_t limit, UErrorCode& status) {
    
    int32_t start = pos;
    UChar op = 0;
    int32_t i;

    
    segmentStandins.truncate(0);
    segmentObjects.removeAllElements();

    
    RuleHalf _left(*this), _right(*this);
    RuleHalf* left = &_left;
    RuleHalf* right = &_right;

    undefinedVariableName.remove();
    pos = left->parse(rule, pos, limit, status);
    if (U_FAILURE(status)) {
        return start;
    }

    if (pos == limit || u_strchr(gOPERATORS, (op = rule.charAt(--pos))) == NULL) {
        return syntaxError(U_MISSING_OPERATOR, rule, start, status);
    }
    ++pos;

    
    if (op == REVERSE_RULE_OP &&
        (pos < limit && rule.charAt(pos) == FORWARD_RULE_OP)) {
        ++pos;
        op = FWDREV_RULE_OP;
    }

    
    switch (op) {
    case ALT_FORWARD_RULE_OP:
        op = FORWARD_RULE_OP;
        break;
    case ALT_REVERSE_RULE_OP:
        op = REVERSE_RULE_OP;
        break;
    case ALT_FWDREV_RULE_OP:
        op = FWDREV_RULE_OP;
        break;
    }

    pos = right->parse(rule, pos, limit, status);
    if (U_FAILURE(status)) {
        return start;
    }

    if (pos < limit) {
        if (rule.charAt(--pos) == END_OF_RULE) {
            ++pos;
        } else {
            
            return syntaxError(U_UNQUOTED_SPECIAL, rule, start, status);
        }
    }

    if (op == VARIABLE_DEF_OP) {
        
        
        
        

        
        
        if (undefinedVariableName.length() == 0) {
            
            return syntaxError(U_BAD_VARIABLE_DEFINITION, rule, start, status);
        }
        if (left->text.length() != 1 || left->text.charAt(0) != variableLimit) {
            
            return syntaxError(U_MALFORMED_VARIABLE_DEFINITION, rule, start, status);
        }
        if (left->anchorStart || left->anchorEnd ||
            right->anchorStart || right->anchorEnd) {
            return syntaxError(U_MALFORMED_VARIABLE_DEFINITION, rule, start, status);
        } 
        
        UnicodeString* value = new UnicodeString(right->text);
        
        if (value == NULL) {
            return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
        }
        variableNames.put(undefinedVariableName, value, status);
        ++variableLimit;
        return pos;
    }

    
    
    if (undefinedVariableName.length() != 0) {
        return syntaxError(
                    U_UNDEFINED_VARIABLE,
                    rule, start, status);
    }

    
    if (segmentStandins.length() > segmentObjects.size()) {
        syntaxError(U_UNDEFINED_SEGMENT_REFERENCE, rule, start, status);
    }
    for (i=0; i<segmentStandins.length(); ++i) {
        if (segmentStandins.charAt(i) == 0) {
            syntaxError(U_INTERNAL_TRANSLITERATOR_ERROR, rule, start, status); 
        }
    }
    for (i=0; i<segmentObjects.size(); ++i) {
        if (segmentObjects.elementAt(i) == NULL) {
            syntaxError(U_INTERNAL_TRANSLITERATOR_ERROR, rule, start, status); 
        }
    }
    
    
    
    if (op != FWDREV_RULE_OP &&
        ((direction == UTRANS_FORWARD) != (op == FORWARD_RULE_OP))) {
        return pos;
    }

    
    
    if (direction == UTRANS_REVERSE) {
        left = &_right;
        right = &_left;
    }

    
    
    
    if (op == FWDREV_RULE_OP) {
        right->removeContext();
        left->cursor = -1;
        left->cursorOffset = 0;
    }

    
    if (left->ante < 0) {
        left->ante = 0;
    }
    if (left->post < 0) {
        left->post = left->text.length();
    }

    
    
    
    
    
    
    if (right->ante >= 0 || right->post >= 0 || left->cursor >= 0 ||
        (right->cursorOffset != 0 && right->cursor < 0) ||
        
        
        
        
        
        
        right->anchorStart || right->anchorEnd ||
        !left->isValidInput(*this) || !right->isValidOutput(*this) ||
        left->ante > left->post) {

        return syntaxError(U_MALFORMED_RULE, rule, start, status);
    }

    
    UnicodeFunctor** segmentsArray = NULL;
    if (segmentObjects.size() > 0) {
        segmentsArray = (UnicodeFunctor **)uprv_malloc(segmentObjects.size() * sizeof(UnicodeFunctor *));
        
        if (segmentsArray == NULL) {
            return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
        }
        segmentObjects.toArray((void**) segmentsArray);
    }
    TransliterationRule* temptr = new TransliterationRule(
            left->text, left->ante, left->post,
            right->text, right->cursor, right->cursorOffset,
            segmentsArray,
            segmentObjects.size(),
            left->anchorStart, left->anchorEnd,
            curData,
            status);
    
    if (temptr == NULL) {
        uprv_free(segmentsArray);
        return syntaxError(U_MEMORY_ALLOCATION_ERROR, rule, start, status);
    }

    curData->ruleSet.addRule(temptr, status);

    return pos;
}










int32_t TransliteratorParser::syntaxError(UErrorCode parseErrorCode,
                                          const UnicodeString& rule,
                                          int32_t pos,
                                          UErrorCode& status)
{
    parseError.offset = pos;
    parseError.line = 0 ; 
    
    
    const int32_t LEN = U_PARSE_CONTEXT_LEN - 1;
    int32_t start = uprv_max(pos - LEN, 0);
    int32_t stop  = pos;
    
    rule.extract(start,stop-start,parseError.preContext);
    
    parseError.preContext[stop-start] = 0;
    
    
    start = pos;
    stop  = uprv_min(pos + LEN, rule.length());
    
    rule.extract(start,stop-start,parseError.postContext);
    
    parseError.postContext[stop-start]= 0;

    status = (UErrorCode)parseErrorCode;
    return pos;

}





UChar TransliteratorParser::parseSet(const UnicodeString& rule,
                                          ParsePosition& pos,
                                          UErrorCode& status) {
    UnicodeSet* set = new UnicodeSet(rule, pos, USET_IGNORE_SPACE, parseData, status);
    
    if (set == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return (UChar)0x0000; 
    }
    set->compact();
    return generateStandInFor(set, status);
}





UChar TransliteratorParser::generateStandInFor(UnicodeFunctor* adopted, UErrorCode& status) {
    
    
    
    
    for (int32_t i=0; i<variablesVector.size(); ++i) {
        if (variablesVector.elementAt(i) == adopted) { 
            return (UChar) (curData->variablesBase + i);
        }
    }
    
    if (variableNext >= variableLimit) {
        delete adopted;
        status = U_VARIABLE_RANGE_EXHAUSTED;
        return 0;
    }
    variablesVector.addElement(adopted, status);
    return variableNext++;
}




UChar TransliteratorParser::getSegmentStandin(int32_t seg, UErrorCode& status) {
    
    UChar empty = curData->variablesBase - 1;
    while (segmentStandins.length() < seg) {
        segmentStandins.append(empty);
    }
    UChar c = segmentStandins.charAt(seg-1);
    if (c == empty) {
        if (variableNext >= variableLimit) {
            status = U_VARIABLE_RANGE_EXHAUSTED;
            return 0;
        }
        c = variableNext++;
        
        
        
        variablesVector.addElement((void*) NULL, status);
        segmentStandins.setCharAt(seg-1, c);
    }
    return c;
}




void TransliteratorParser::setSegmentObject(int32_t seg, StringMatcher* adopted, UErrorCode& status) {
    
    
    
    
    if (segmentObjects.size() < seg) {
        segmentObjects.setSize(seg, status);
    }
    int32_t index = getSegmentStandin(seg, status) - curData->variablesBase;
    if (segmentObjects.elementAt(seg-1) != NULL ||
        variablesVector.elementAt(index) != NULL) {
        
        status = U_INTERNAL_TRANSLITERATOR_ERROR;
        return;
    }
    segmentObjects.setElementAt(adopted, seg-1);
    variablesVector.setElementAt(adopted, index);
}





UChar TransliteratorParser::getDotStandIn(UErrorCode& status) {
    if (dotStandIn == (UChar) -1) {
        UnicodeSet* tempus = new UnicodeSet(UnicodeString(TRUE, DOT_SET, -1), status);
        
        if (tempus == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return (UChar)0x0000;
        }
        dotStandIn = generateStandInFor(tempus, status);
    }
    return dotStandIn;
}





void TransliteratorParser::appendVariableDef(const UnicodeString& name,
                                                  UnicodeString& buf,
                                                  UErrorCode& status) {
    const UnicodeString* s = (const UnicodeString*) variableNames.get(name);
    if (s == NULL) {
        
        
        
        
        if (undefinedVariableName.length() == 0) {
            undefinedVariableName = name;
            if (variableNext >= variableLimit) {
                
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
            buf.append((UChar) --variableLimit);
        } else {
            
            
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
    } else {
        buf.append(*s);
    }
}








U_NAMESPACE_END

U_CAPI int32_t
utrans_stripRules(const UChar *source, int32_t sourceLen, UChar *target, UErrorCode *status) {
    U_NAMESPACE_USE

    
    const UChar *targetStart = target;
    const UChar *sourceLimit = source+sourceLen;
    UChar *targetLimit = target+sourceLen;
    UChar32 c = 0;
    UBool quoted = FALSE;
    int32_t index;

    uprv_memset(target, 0, sourceLen*U_SIZEOF_UCHAR);

    
    while (source < sourceLimit)
    {
        index=0;
        U16_NEXT_UNSAFE(source, index, c);
        source+=index;
        if(c == QUOTE) {
            quoted = (UBool)!quoted;
        }
        else if (!quoted) {
            if (c == RULE_COMMENT_CHAR) {
                
                while (targetStart < target && *(target - 1) == 0x0020) {
                    target--;
                }
                do {
                    c = *(source++);
                }
                while (c != CR && c != LF);
            }
            else if (c == ESCAPE) {
                UChar32   c2 = *source;
                if (c2 == CR || c2 == LF) {
                    
                    
                    source++;
                    continue;
                }
                if (c2 == 0x0075 && source+5 < sourceLimit) { 
                    int32_t escapeOffset = 0;
                    UnicodeString escapedStr(source, 5);
                    c2 = escapedStr.unescapeAt(escapeOffset);

                    if (c2 == (UChar32)0xFFFFFFFF || escapeOffset == 0)
                    {
                        *status = U_PARSE_ERROR;
                        return 0;
                    }
                    if (!PatternProps::isWhiteSpace(c2) && !u_iscntrl(c2) && !u_ispunct(c2)) {
                        
                        source+=5;
                        c = c2;
                    }
                }
                else if (c2 == QUOTE) {
                    
                    quoted = (UBool)!quoted;
                }
            }
        }
        if (c == CR || c == LF)
        {
            


            quoted = FALSE;
            while (source < sourceLimit) {
                c = *(source);
                if (c != CR && c != LF && c != 0x0020) {
                    break;
                }
                source++;
            }
            continue;
        }

        
        index=0;
        U16_APPEND_UNSAFE(target, index, c);
        target+=index;
    }
    if (target < targetLimit) {
        *target = 0;
    }
    return (int32_t)(target-targetStart);
}

#endif 
