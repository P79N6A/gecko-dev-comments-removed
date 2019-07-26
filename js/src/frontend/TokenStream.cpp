









#include <stdio.h>      
#include <errno.h>
#include <limits.h>
#include <math.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsexn.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "frontend/Parser.h"
#include "frontend/TokenStream.h"
#include "vm/RegExpObject.h"
#include "vm/StringBuffer.h"

#include "jsscriptinlines.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

using namespace js;
using namespace js::frontend;
using namespace js::unicode;

#define JS_KEYWORD(keyword, type, op, version) \
    const char js_##keyword##_str[] = #keyword;
#include "jskeyword.tbl"
#undef JS_KEYWORD

static const KeywordInfo keywords[] = {
#define JS_KEYWORD(keyword, type, op, version) \
    {js_##keyword##_str, type, op, version},
#include "jskeyword.tbl"
#undef JS_KEYWORD
};

const KeywordInfo *
frontend::FindKeyword(const jschar *s, size_t length)
{
    JS_ASSERT(length != 0);

    register size_t i;
    const struct KeywordInfo *kw;
    const char *chars;

#define JSKW_LENGTH()           length
#define JSKW_AT(column)         s[column]
#define JSKW_GOT_MATCH(index)   i = (index); goto got_match;
#define JSKW_TEST_GUESS(index)  i = (index); goto test_guess;
#define JSKW_NO_MATCH()         goto no_match;
#include "jsautokw.h"
#undef JSKW_NO_MATCH
#undef JSKW_TEST_GUESS
#undef JSKW_GOT_MATCH
#undef JSKW_AT
#undef JSKW_LENGTH

  got_match:
    return &keywords[i];

  test_guess:
    kw = &keywords[i];
    chars = kw->chars;
    do {
        if (*s++ != (unsigned char)(*chars++))
            goto no_match;
    } while (--length != 0);
    return kw;

  no_match:
    return NULL;
}

bool
frontend::IsIdentifier(JSLinearString *str)
{
    const jschar *chars = str->chars();
    size_t length = str->length();

    if (length == 0)
        return false;
    jschar c = *chars;
    if (!IsIdentifierStart(c))
        return false;
    const jschar *end = chars + length;
    while (++chars != end) {
        c = *chars;
        if (!IsIdentifierPart(c))
            return false;
    }
    return true;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4351)
#endif


TokenStream::TokenStream(JSContext *cx, const CompileOptions &options,
                         const jschar *base, size_t length, StrictModeGetter *smg)
  : tokens(),
    tokensRoot(cx, &tokens),
    cursor(),
    lookahead(),
    lineno(options.lineno),
    flags(),
    linebase(base),
    prevLinebase(NULL),
    linebaseRoot(cx, &linebase),
    prevLinebaseRoot(cx, &prevLinebase),
    userbuf(base, length),
    userbufRoot(cx, &userbuf),
    filename(options.filename),
    sourceMap(NULL),
    listenerTSData(),
    tokenbuf(cx),
    version(options.version),
    allowXML(VersionHasAllowXML(options.version)),
    moarXML(VersionHasMoarXML(options.version)),
    cx(cx),
    originPrincipals(JSScript::normalizeOriginPrincipals(options.principals,
                                                         options.originPrincipals)),
    strictModeGetter(smg)
{
    if (originPrincipals)
        JS_HoldPrincipals(originPrincipals);

    JSSourceHandler listener = cx->runtime->debugHooks.sourceHandler;
    void *listenerData = cx->runtime->debugHooks.sourceHandlerData;

    if (listener)
        listener(options.filename, options.lineno, base, length, &listenerTSData, listenerData);

    













    memset(oneCharTokens, 0, sizeof(oneCharTokens));
    oneCharTokens[unsigned(';')] = TOK_SEMI;
    oneCharTokens[unsigned(',')] = TOK_COMMA;
    oneCharTokens[unsigned('?')] = TOK_HOOK;
    oneCharTokens[unsigned('[')] = TOK_LB;
    oneCharTokens[unsigned(']')] = TOK_RB;
    oneCharTokens[unsigned('{')] = TOK_LC;
    oneCharTokens[unsigned('}')] = TOK_RC;
    oneCharTokens[unsigned('(')] = TOK_LP;
    oneCharTokens[unsigned(')')] = TOK_RP;

    
    memset(maybeEOL, 0, sizeof(maybeEOL));
    maybeEOL[unsigned('\n')] = true;
    maybeEOL[unsigned('\r')] = true;
    maybeEOL[unsigned(LINE_SEPARATOR & 0xff)] = true;
    maybeEOL[unsigned(PARA_SEPARATOR & 0xff)] = true;

    
    memset(maybeStrSpecial, 0, sizeof(maybeStrSpecial));
    maybeStrSpecial[unsigned('"')] = true;
    maybeStrSpecial[unsigned('\'')] = true;
    maybeStrSpecial[unsigned('\\')] = true;
    maybeStrSpecial[unsigned('\n')] = true;
    maybeStrSpecial[unsigned('\r')] = true;
    maybeStrSpecial[unsigned(LINE_SEPARATOR & 0xff)] = true;
    maybeStrSpecial[unsigned(PARA_SEPARATOR & 0xff)] = true;
    maybeStrSpecial[unsigned(EOF & 0xff)] = true;

    









    tokens[0].pos.begin.lineno = tokens[0].pos.end.lineno = options.lineno;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

TokenStream::~TokenStream()
{
    if (flags & TSF_OWNFILENAME)
        js_free((void *) filename);
    if (sourceMap)
        js_free(sourceMap);
    if (originPrincipals)
        JS_DropPrincipals(cx->runtime, originPrincipals);
}


#if defined(HAVE_GETC_UNLOCKED)
# define fast_getc getc_unlocked
#elif defined(HAVE__GETC_NOLOCK)
# define fast_getc _getc_nolock
#else
# define fast_getc getc
#endif

JS_ALWAYS_INLINE void
TokenStream::updateLineInfoForEOL()
{
    prevLinebase = linebase;
    linebase = userbuf.addressOfNextRawChar();
    lineno++;
}

JS_ALWAYS_INLINE void
TokenStream::updateFlagsForEOL()
{
    flags &= ~TSF_DIRTYLINE;
    flags |= TSF_EOL;
}


int32_t
TokenStream::getChar()
{
    int32_t c;
    if (JS_LIKELY(userbuf.hasRawChars())) {
        c = userbuf.getRawChar();

        














        if (JS_UNLIKELY(maybeEOL[c & 0xff])) {
            if (c == '\n')
                goto eol;
            if (c == '\r') {
                
                if (userbuf.hasRawChars())
                    userbuf.matchRawChar('\n');
                goto eol;
            }
            if (c == LINE_SEPARATOR || c == PARA_SEPARATOR)
                goto eol;
        }
        return c;
    }

    flags |= TSF_EOF;
    return EOF;

  eol:
    updateLineInfoForEOL();
    return '\n';
}








int32_t
TokenStream::getCharIgnoreEOL()
{
    if (JS_LIKELY(userbuf.hasRawChars()))
        return userbuf.getRawChar();

    flags |= TSF_EOF;
    return EOF;
}

void
TokenStream::ungetChar(int32_t c)
{
    if (c == EOF)
        return;
    JS_ASSERT(!userbuf.atStart());
    userbuf.ungetRawChar();
    if (c == '\n') {
#ifdef DEBUG
        int32_t c2 = userbuf.peekRawChar();
        JS_ASSERT(TokenBuf::isRawEOLChar(c2));
#endif

        
        if (!userbuf.atStart())
            userbuf.matchRawCharBackwards('\r');

        JS_ASSERT(prevLinebase);    
        linebase = prevLinebase;
        prevLinebase = NULL;
        lineno--;
    } else {
        JS_ASSERT(userbuf.peekRawChar() == c);
    }
}

void
TokenStream::ungetCharIgnoreEOL(int32_t c)
{
    if (c == EOF)
        return;
    JS_ASSERT(!userbuf.atStart());
    userbuf.ungetRawChar();
}







bool
TokenStream::peekChars(int n, jschar *cp)
{
    int i, j;
    int32_t c;

    for (i = 0; i < n; i++) {
        c = getCharIgnoreEOL();
        if (c == EOF)
            break;
        if (c == '\n') {
            ungetCharIgnoreEOL(c);
            break;
        }
        cp[i] = (jschar)c;
    }
    for (j = i - 1; j >= 0; j--)
        ungetCharIgnoreEOL(cp[j]);
    return i == n;
}

const jschar *
TokenStream::TokenBuf::findEOLMax(const jschar *p, size_t max)
{
    JS_ASSERT(base_ <= p && p <= limit_);

    size_t n = 0;
    while (true) {
        if (p >= limit_)
            break;
        if (n >= max)
            break;
        if (TokenBuf::isRawEOLChar(*p++))
            break;
        n++;
    }
    return p;
}

bool
TokenStream::reportStrictModeErrorNumberVA(ParseNode *pn, unsigned errorNumber, va_list args)
{
    
    unsigned flags = JSREPORT_STRICT;
    if (strictModeState() != StrictMode::NOTSTRICT)
        flags |= JSREPORT_ERROR;
    else if (cx->hasStrictOption())
        flags |= JSREPORT_WARNING;
    else
        return true;
 
    return reportCompileErrorNumberVA(pn, flags, errorNumber, args);
}

void
CompileError::throwError()
{
    












    if (!js_ErrorToException(cx, message, &report, NULL, NULL)) {
        



        bool reportError = true;
        if (JSDebugErrorHook hook = cx->runtime->debugHooks.debugErrorHook) {
            reportError = hook(cx, message, &report, cx->runtime->debugHooks.debugErrorHookData);
        }

        
        if (reportError && cx->errorReporter)
            cx->errorReporter(cx, message, &report);
    }
}

CompileError::~CompileError()
{
    js_free((void*)report.uclinebuf);
    js_free((void*)report.linebuf);
    js_free((void*)report.ucmessage);
    js_free(message);
    message = NULL;

    if (report.messageArgs) {
        if (hasCharArgs) {
            unsigned i = 0;
            while (report.messageArgs[i])
                js_free((void*)report.messageArgs[i++]);
        }
        js_free(report.messageArgs);
    }

    PodZero(&report);
}

bool
TokenStream::reportCompileErrorNumberVA(ParseNode *pn, unsigned flags, unsigned errorNumber,
                                        va_list args)
{
    bool strict = JSREPORT_IS_STRICT(flags);
    bool warning = JSREPORT_IS_WARNING(flags);

    
    
    if (strict && warning && (!cx->hasStrictOption() || errorNumber == JSMSG_STRICT_CODE_WITH))
        return true;

    if (warning && cx->hasWErrorOption()) {
        flags &= ~JSREPORT_WARNING;
        warning = false;
    }

    CompileError normalError(cx);
    CompileError *err = &normalError;
    if (strict && !warning && strictModeState() == StrictMode::UNKNOWN) {
        if (strictModeGetter->queuedStrictModeError()) {
            
            
            if (cx->hasStrictOption() && errorNumber != JSMSG_STRICT_CODE_WITH) {
                flags |= JSREPORT_WARNING;
                warning = true;
            } else {
                return true;
            }
        } else {
            err = cx->new_<CompileError, JSContext *>(cx);
            if (!err)
                return false;
            strictModeGetter->setQueuedStrictModeError(err);
        }
    }

    const TokenPos *const tp = pn ? &pn->pn_pos : &currentToken().pos;

    err->report.flags = flags;
    err->report.errorNumber = errorNumber;
    err->report.filename = filename;
    err->report.originPrincipals = originPrincipals;
    err->report.lineno = tp->begin.lineno;

    err->hasCharArgs = !(flags & JSREPORT_UC);

    if (!js_ExpandErrorArguments(cx, js_GetErrorMessage, NULL, errorNumber, &err->message, &err->report,
                                 err->hasCharArgs, args)) {
        return false;
    }

    









    if (err->report.lineno == lineno) {
        const jschar *tokptr = linebase + tp->begin.index;

        
        
        
        
        
        static const size_t windowRadius = 60;

        
        const jschar *windowBase = (linebase + windowRadius < tokptr)
                                 ? tokptr - windowRadius
                                 : linebase;
        size_t nTrunc = windowBase - linebase;
        uint32_t windowIndex = tp->begin.index - nTrunc;

        
        const jschar *windowLimit = userbuf.findEOLMax(tokptr, windowRadius);
        size_t windowLength = windowLimit - windowBase;
        JS_ASSERT(windowLength <= windowRadius * 2);

        
        StringBuffer windowBuf(cx);
        if (!windowBuf.append(windowBase, windowLength) || !windowBuf.append((jschar)0))
            return false;

        
        
        err->report.uclinebuf = windowBuf.extractWellSized();
        if (!err->report.uclinebuf)
            return false;
        err->report.linebuf = DeflateString(cx, err->report.uclinebuf, windowLength);
        if (!err->report.linebuf)
            return false;

        
        JS_ASSERT(tp->begin.lineno == tp->end.lineno);
        err->report.tokenptr = err->report.linebuf + windowIndex;
        err->report.uctokenptr = err->report.uclinebuf + windowIndex;
    }

    if (err == &normalError)
        err->throwError();
    else
        return true;

    return warning;
}

bool
TokenStream::reportStrictModeError(unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = reportStrictModeErrorNumberVA(NULL, errorNumber, args);
    va_end(args);
    return result;
}

bool
TokenStream::reportError(unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = reportCompileErrorNumberVA(NULL, JSREPORT_ERROR, errorNumber, args);
    va_end(args);
    return result;
}

bool
TokenStream::reportWarning(unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = reportCompileErrorNumberVA(NULL, JSREPORT_WARNING, errorNumber, args);
    va_end(args);
    return result;
}

bool
TokenStream::reportStrictWarning(unsigned errorNumber, ...)
{
    va_list args;
    va_start(args, errorNumber);
    bool result = reportCompileErrorNumberVA(NULL, JSREPORT_STRICT | JSREPORT_WARNING,
                                             errorNumber, args);
    va_end(args);
    return result;
}

#if JS_HAS_XML_SUPPORT

bool
TokenStream::getXMLEntity()
{
    ptrdiff_t offset, length, i;
    int c, d;
    bool ispair;
    jschar *bp, digit;
    char *bytes;
    JSErrNum msg;

    CharBuffer &tb = tokenbuf;

    
    offset = tb.length();
    if (!tb.append('&'))
        return false;
    while ((c = getChar()) != ';') {
        if (c == EOF || c == '\n') {
            reportError(JSMSG_END_OF_XML_ENTITY);
            return false;
        }
        if (!tb.append(c))
            return false;
    }

    
    length = tb.length() - offset;
    bp = tb.begin() + offset;
    c = d = 0;
    ispair = false;
    if (length > 2 && bp[1] == '#') {
        
        i = 2;
        if (length > 3 && (bp[i] == 'x' || bp[i] == 'X')) {
            if (length > 9)     
                goto badncr;
            while (++i < length) {
                digit = bp[i];
                if (!JS7_ISHEX(digit))
                    goto badncr;
                c = (c << 4) + JS7_UNHEX(digit);
            }
        } else {
            while (i < length) {
                digit = bp[i++];
                if (!JS7_ISDEC(digit))
                    goto badncr;
                c = (c * 10) + JS7_UNDEC(digit);
                if (c < 0)
                    goto badncr;
            }
        }

        if (0x10000 <= c && c <= 0x10FFFF) {
            
            d = 0xDC00 + (c & 0x3FF);
            c = 0xD7C0 + (c >> 10);
            ispair = true;
        } else {
            
            if (c != 0x9 && c != 0xA && c != 0xD &&
                !(0x20 <= c && c <= 0xD7FF) &&
                !(0xE000 <= c && c <= 0xFFFD)) {
                goto badncr;
            }
        }
    } else {
        
        switch (length) {
          case 3:
            if (bp[2] == 't') {
                if (bp[1] == 'l')
                    c = '<';
                else if (bp[1] == 'g')
                    c = '>';
            }
            break;
          case 4:
            if (bp[1] == 'a' && bp[2] == 'm' && bp[3] == 'p')
                c = '&';
            break;
          case 5:
            if (bp[3] == 'o') {
                if (bp[1] == 'a' && bp[2] == 'p' && bp[4] == 's')
                    c = '\'';
                else if (bp[1] == 'q' && bp[2] == 'u' && bp[4] == 't')
                    c = '"';
            }
            break;
        }
        if (c == 0) {
            msg = JSMSG_UNKNOWN_XML_ENTITY;
            goto bad;
        }
    }

    
    *bp++ = (jschar) c;
    if (ispair)
        *bp++ = (jschar) d;
    tb.shrinkBy(tb.end() - bp);
    return true;

  badncr:
    msg = JSMSG_BAD_XML_NCR;
  bad:
    
    JS_ASSERT((tb.end() - bp) >= 1);
    bytes = DeflateString(cx, bp + 1, (tb.end() - bp) - 1);
    if (bytes) {
        reportError(msg, bytes);
        js_free(bytes);
    }
    return false;
}

bool
TokenStream::getXMLTextOrTag(TokenKind *ttp, Token **tpp)
{
    TokenKind tt;
    int c, qc;
    Token *tp;
    JSAtom *atom;

    


    if (flags & TSF_XMLTEXTMODE) {
        tt = TOK_XMLSPACE;      
        tp = newToken(0);
        tokenbuf.clear();
        qc = (flags & TSF_XMLONLYMODE) ? '<' : '{';

        while ((c = getChar()) != qc && c != '<' && c != EOF) {
            if (c == '&' && qc == '<') {
                if (!getXMLEntity())
                    goto error;
                tt = TOK_XMLTEXT;
                continue;
            }

            if (!IsXMLSpace(c))
                tt = TOK_XMLTEXT;
            if (!tokenbuf.append(c))
                goto error;
        }
        ungetChar(c);

        if (tokenbuf.empty()) {
            atom = NULL;
        } else {
            atom = atomize(cx, tokenbuf);
            if (!atom)
                goto error;
        }
        tp->pos.end.lineno = lineno;
        tp->setAtom(JSOP_STRING, atom);
        goto out;
    }

    


    else {
        JS_ASSERT(flags & TSF_XMLTAGMODE);
        tp = newToken(0);
        c = getChar();
        if (c != EOF && IsXMLSpace(c)) {
            do {
                c = getChar();
                if (c == EOF)
                    break;
            } while (IsXMLSpace(c));
            ungetChar(c);
            tp->pos.end.lineno = lineno;
            tt = TOK_XMLSPACE;
            goto out;
        }

        if (c == EOF) {
            tt = TOK_EOF;
            goto out;
        }

        tokenbuf.clear();
        if (IsXMLNamespaceStart(c)) {
            bool sawColon = false;

            if (!tokenbuf.append(c))
                goto error;
            while ((c = getChar()) != EOF && IsXMLNamePart(c)) {
                if (c == ':') {
                    int nextc;

                    if (sawColon ||
                        (nextc = peekChar(),
                         ((flags & TSF_XMLONLYMODE) || nextc != '{') &&
                         !IsXMLNamePart(nextc))) {
                        reportError(JSMSG_BAD_XML_QNAME);
                        goto error;
                    }
                    sawColon = true;
                }

                if (!tokenbuf.append(c))
                    goto error;
            }

            ungetChar(c);
            atom = atomize(cx, tokenbuf);
            if (!atom)
                goto error;
            tp->setAtom(JSOP_STRING, atom);
            tt = TOK_XMLNAME;
            goto out;
        }

        switch (c) {
          case '{':
            if (flags & TSF_XMLONLYMODE)
                goto bad_xml_char;
            tt = TOK_LC;
            goto out;

          case '=':
            tt = TOK_ASSIGN;
            goto out;

          case '"':
          case '\'':
            qc = c;
            while ((c = getChar()) != qc) {
                if (c == EOF) {
                    reportError(JSMSG_UNTERMINATED_STRING);
                    goto error;
                }

                




                if (c == '"' && !(flags & TSF_XMLONLYMODE)) {
                    JS_ASSERT(qc == '\'');
                    if (!tokenbuf.append(js_quot_entity_str,
                                     strlen(js_quot_entity_str)))
                        goto error;
                    continue;
                }

                if (c == '&' && (flags & TSF_XMLONLYMODE)) {
                    if (!getXMLEntity())
                        goto error;
                    continue;
                }

                if (!tokenbuf.append(c))
                    goto error;
            }
            atom = atomize(cx, tokenbuf);
            if (!atom)
                goto error;
            tp->pos.end.lineno = lineno;
            tp->setAtom(JSOP_STRING, atom);
            tt = TOK_XMLATTR;
            goto out;

          case '>':
            tt = TOK_XMLTAGC;
            goto out;

          case '/':
            if (matchChar('>')) {
                tt = TOK_XMLPTAGC;
                goto out;
            }
            

          bad_xml_char:
          default:
            reportError(JSMSG_BAD_XML_CHARACTER);
            goto error;
        }
        JS_NOT_REACHED("getXMLTextOrTag 1");
    }
    JS_NOT_REACHED("getXMLTextOrTag 2");

  out:
    *ttp = tt;
    *tpp = tp;
    return true;

  error:
    *ttp = TOK_ERROR;
    *tpp = tp;
    return false;
}






























bool
TokenStream::getXMLMarkup(TokenKind *ttp, Token **tpp)
{
    TokenKind tt;
    int c;
    Token *tp = *tpp;

    
    if (matchChar('!')) {
        tokenbuf.clear();

        
        if (matchChar('-')) {
            if (!matchChar('-'))
                goto bad_xml_markup;
            while ((c = getChar()) != '-' || !matchChar('-')) {
                if (c == EOF)
                    goto bad_xml_markup;
                if (!tokenbuf.append(c))
                    goto error;
            }
            if (!matchChar('>'))
                goto bad_xml_markup;

            JSAtom *commentText = atomize(cx, tokenbuf);
            if (!commentText)
                goto error;
            tp->setAtom(JSOP_XMLCOMMENT, commentText);
            tp->pos.end.lineno = lineno;
            tt = TOK_XMLCOMMENT;
            goto out;
        }

        
        if (matchChar('[')) {
            jschar cp[6];
            if (peekChars(6, cp) &&
                cp[0] == 'C' &&
                cp[1] == 'D' &&
                cp[2] == 'A' &&
                cp[3] == 'T' &&
                cp[4] == 'A' &&
                cp[5] == '[') {
                skipChars(6);
                while ((c = getChar()) != ']' ||
                       !peekChars(2, cp) ||
                       cp[0] != ']' ||
                       cp[1] != '>') {
                    if (c == EOF)
                        goto bad_xml_markup;
                    if (!tokenbuf.append(c))
                        goto error;
                }
                consumeKnownChar(']');
                consumeKnownChar('>');

                JSAtom *cdataContent = atomize(cx, tokenbuf);
                if (!cdataContent)
                    goto error;

                tp->setAtom(JSOP_XMLCDATA, cdataContent);
                tp->pos.end.lineno = lineno;
                tt = TOK_XMLCDATA;
                goto out;
            }
            goto bad_xml_markup;
        }
    }

    
    if (matchChar('?')) {
        bool inTarget = true;
        size_t targetLength = 0;
        ptrdiff_t contentIndex = -1;

        tokenbuf.clear();
        while ((c = getChar()) != '?' || peekChar() != '>') {
            if (c == EOF)
                goto bad_xml_markup;
            if (inTarget) {
                if (IsXMLSpace(c)) {
                    if (tokenbuf.empty())
                        goto bad_xml_markup;
                    inTarget = false;
                } else {
                    if (!(tokenbuf.empty()
                          ? IsXMLNamespaceStart(c)
                          : IsXMLNamespacePart(c))) {
                        goto bad_xml_markup;
                    }
                    ++targetLength;
                }
            } else {
                if (contentIndex < 0 && !IsXMLSpace(c))
                    contentIndex = tokenbuf.length();
            }
            if (!tokenbuf.append(c))
                goto error;
        }
        if (targetLength == 0)
            goto bad_xml_markup;

        JSAtom *data;
        if (contentIndex < 0) {
            data = cx->runtime->atomState.emptyAtom;
        } else {
            data = AtomizeChars(cx, tokenbuf.begin() + contentIndex,
                                tokenbuf.length() - contentIndex);
            if (!data)
                goto error;
        }
        tokenbuf.shrinkBy(tokenbuf.length() - targetLength);
        consumeKnownChar('>');
        JSAtom *target = atomize(cx, tokenbuf);
        if (!target)
            goto error;
        tp->setProcessingInstruction(target->asPropertyName(), data);
        tp->pos.end.lineno = lineno;
        tt = TOK_XMLPI;
        goto out;
    }

    
    tt = matchChar('/') ? TOK_XMLETAGO : TOK_XMLSTAGO;

  out:
    *ttp = tt;
    *tpp = tp;
    return true;

  bad_xml_markup:
    reportError(JSMSG_BAD_XML_MARKUP);
  error:
    *ttp = TOK_ERROR;
    *tpp = tp;
    return false;
}
#endif 







bool
TokenStream::peekUnicodeEscape(int *result)
{
    jschar cp[5];

    if (peekChars(5, cp) && cp[0] == 'u' &&
        JS7_ISHEX(cp[1]) && JS7_ISHEX(cp[2]) &&
        JS7_ISHEX(cp[3]) && JS7_ISHEX(cp[4]))
    {
        *result = (((((JS7_UNHEX(cp[1]) << 4)
                + JS7_UNHEX(cp[2])) << 4)
              + JS7_UNHEX(cp[3])) << 4)
            + JS7_UNHEX(cp[4]);
        return true;
    }
    return false;
}

bool
TokenStream::matchUnicodeEscapeIdStart(int32_t *cp)
{
    if (peekUnicodeEscape(cp) && IsIdentifierStart(*cp)) {
        skipChars(5);
        return true;
    }
    return false;
}

bool
TokenStream::matchUnicodeEscapeIdent(int32_t *cp)
{
    if (peekUnicodeEscape(cp) && IsIdentifierPart(*cp)) {
        skipChars(5);
        return true;
    }
    return false;
}

size_t
TokenStream::endOffset(const Token &tok)
{
    uint32_t lineno = tok.pos.begin.lineno;
    JS_ASSERT(lineno <= tok.pos.end.lineno);
    const jschar *end;
    if (lineno < tok.pos.end.lineno) {
        TokenBuf buf(tok.ptr, userbuf.addressOfNextRawChar() - userbuf.base());
        for (; lineno < tok.pos.end.lineno; lineno++) {
            jschar c;
            do {
                JS_ASSERT(buf.hasRawChars());
                c = buf.getRawChar();
            } while (!TokenBuf::isRawEOLChar(c));
            if (c == '\r' && buf.hasRawChars())
                buf.matchRawChar('\n');
        }
        end = buf.addressOfNextRawChar() + tok.pos.end.index;
    } else {
        end = tok.ptr + (tok.pos.end.index - tok.pos.begin.index);
    }
    JS_ASSERT(end <= userbuf.addressOfNextRawChar());
    return end - userbuf.base();
}





static bool
CharsMatch(const jschar *p, const char *q) {
    while (*q) {
        if (*p++ != *q++)
            return false;
    }
    return true;
}

bool
TokenStream::getAtLine()
{
    int c;
    jschar cp[5];
    unsigned i, line, temp;
    char filenameBuf[1024];

    




    if (peekChars(5, cp) && CharsMatch(cp, "@line")) {
        skipChars(5);
        while ((c = getChar()) != '\n' && c != EOF && IsSpaceOrBOM2(c))
            continue;
        if (JS7_ISDEC(c)) {
            line = JS7_UNDEC(c);
            while ((c = getChar()) != EOF && JS7_ISDEC(c)) {
                temp = 10 * line + JS7_UNDEC(c);
                if (temp < line) {
                    
                    return true;
                }
                line = temp;
            }
            while (c != '\n' && c != EOF && IsSpaceOrBOM2(c))
                c = getChar();
            i = 0;
            if (c == '"') {
                while ((c = getChar()) != EOF && c != '"') {
                    if (c == '\n') {
                        ungetChar(c);
                        return true;
                    }
                    if ((c >> 8) != 0 || i >= sizeof filenameBuf - 1)
                        return true;
                    filenameBuf[i++] = (char) c;
                }
                if (c == '"') {
                    while ((c = getChar()) != '\n' && c != EOF && IsSpaceOrBOM2(c))
                        continue;
                }
            }
            filenameBuf[i] = '\0';
            if (c == EOF || c == '\n') {
                if (i > 0) {
                    if (flags & TSF_OWNFILENAME)
                        js_free((void *) filename);
                    filename = JS_strdup(cx, filenameBuf);
                    if (!filename)
                        return false;
                    flags |= TSF_OWNFILENAME;
                }
                lineno = line;
            }
        }
        ungetChar(c);
    }
    return true;
}

bool
TokenStream::getAtSourceMappingURL()
{
    

    jschar peeked[19];
    int32_t c;

    if (peekChars(19, peeked) && CharsMatch(peeked, "@ sourceMappingURL=")) {
        skipChars(19);
        tokenbuf.clear();

        while ((c = peekChar()) && c != EOF && !IsSpaceOrBOM2(c)) {
            getChar();
            tokenbuf.append(c);
        }

        if (tokenbuf.empty())
            

            return true;

        size_t sourceMapLength = tokenbuf.length();

        if (sourceMap)
            js_free(sourceMap);
        sourceMap = cx->pod_malloc<jschar>(sourceMapLength + 1);
        if (!sourceMap)
            return false;

        PodCopy(sourceMap, tokenbuf.begin(), sourceMapLength);
        sourceMap[sourceMapLength] = '\0';
    }
    return true;
}

Token *
TokenStream::newToken(ptrdiff_t adjust)
{
    cursor = (cursor + 1) & ntokensMask;
    Token *tp = &tokens[cursor];
    tp->ptr = userbuf.addressOfNextRawChar() + adjust;
    tp->pos.begin.index = tp->ptr - linebase;
    tp->pos.begin.lineno = tp->pos.end.lineno = lineno;
    return tp;
}

JS_ALWAYS_INLINE JSAtom *
TokenStream::atomize(JSContext *cx, CharBuffer &cb)
{
    return AtomizeChars(cx, cb.begin(), cb.length());
}

#ifdef DEBUG
bool
IsTokenSane(Token *tp)
{
    



    if (tp->type < TOK_ERROR || tp->type >= TOK_LIMIT || tp->type == TOK_EOL)
        return false;

    if (tp->pos.begin.lineno == tp->pos.end.lineno) {
        if (tp->pos.begin.index > tp->pos.end.index)
            return false;
    } else {
        
        switch (tp->type) {
          case TOK_STRING:
          case TOK_XMLATTR:
          case TOK_XMLSPACE:
          case TOK_XMLTEXT:
          case TOK_XMLCOMMENT:
          case TOK_XMLCDATA:
          case TOK_XMLPI:
            break;
          default:
            return false;
        }
    }
    return true;
}
#endif

bool
TokenStream::putIdentInTokenbuf(const jschar *identStart)
{
    int32_t c, qc;
    const jschar *tmp = userbuf.addressOfNextRawChar();
    userbuf.setAddressOfNextRawChar(identStart);

    tokenbuf.clear();
    for (;;) {
        c = getCharIgnoreEOL();
        if (!IsIdentifierPart(c)) {
            if (c != '\\' || !matchUnicodeEscapeIdent(&qc))
                break;
            c = qc;
        }
        if (!tokenbuf.append(c)) {
            userbuf.setAddressOfNextRawChar(tmp);
            return false;
        }
    }
    userbuf.setAddressOfNextRawChar(tmp);
    return true;
}

bool
TokenStream::checkForKeyword(const jschar *s, size_t length, TokenKind *ttp, JSOp *topp)
{
    JS_ASSERT(!ttp == !topp);

    const KeywordInfo *kw = FindKeyword(s, length);
    if (!kw)
        return true;

    if (kw->tokentype == TOK_RESERVED)
        return reportError(JSMSG_RESERVED_ID, kw->chars);

    if (kw->tokentype != TOK_STRICT_RESERVED) {
        if (kw->version <= versionNumber()) {
            
            if (ttp) {
                *ttp = kw->tokentype;
                *topp = (JSOp) kw->op;
                return true;
            }
            return reportError(JSMSG_RESERVED_ID, kw->chars);
        }

        




        if (kw->tokentype != TOK_LET && kw->tokentype != TOK_YIELD)
            return true;
    }

    
    return reportStrictModeError(JSMSG_RESERVED_ID, kw->chars);
}

enum FirstCharKind {
    Other,
    OneChar,
    Ident,
    Dot,
    Equals,
    String,
    Dec,
    Colon,
    Plus,
    HexOct,

    
    Space,
    EOL
};

#define _______ Other














static const uint8_t firstCharKinds[] = {

 _______, _______, _______, _______, _______, _______, _______, _______, _______,   Space,
     EOL,   Space,   Space,     EOL, _______, _______, _______, _______, _______, _______,
 _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
 _______, _______,   Space, _______,  String, _______,   Ident, _______, _______,  String,
 OneChar, OneChar, _______,    Plus, OneChar, _______,     Dot, _______,  HexOct,     Dec,
     Dec,     Dec,     Dec,     Dec,     Dec,     Dec,     Dec,     Dec,   Colon, OneChar,
 _______,  Equals, _______, OneChar, _______,   Ident,   Ident,   Ident,   Ident,   Ident,
   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,
   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,
   Ident, OneChar, _______, OneChar, _______,   Ident, _______,   Ident,   Ident,   Ident,
   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,
   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,   Ident,
   Ident,   Ident,   Ident, OneChar, _______, OneChar, _______, _______
};

#undef _______

TokenKind
TokenStream::getTokenInternal()
{
    TokenKind tt;
    int c, qc;
    Token *tp;
    FirstCharKind c1kind;
    const jschar *numStart;
    bool hasFracOrExp;
    const jschar *identStart;
    bool hadUnicodeEscape;

    SkipRoot skipNum(cx, &numStart), skipIdent(cx, &identStart);

#if JS_HAS_XML_SUPPORT
    


    if (flags & (TSF_XMLTEXTMODE|TSF_XMLTAGMODE)) {
        if (!getXMLTextOrTag(&tt, &tp))
            goto error;
        goto out;
    }
#endif

  retry:
    if (JS_UNLIKELY(!userbuf.hasRawChars())) {
        tp = newToken(0);
        tt = TOK_EOF;
        flags |= TSF_EOF;
        goto out;
    }

    c = userbuf.getRawChar();
    JS_ASSERT(c != EOF);

    



    if (JS_UNLIKELY(c >= 128)) {
        if (IsSpaceOrBOM2(c)) {
            if (c == LINE_SEPARATOR || c == PARA_SEPARATOR) {
                updateLineInfoForEOL();
                updateFlagsForEOL();
            }

            goto retry;
        }

        tp = newToken(-1);

        
        JS_STATIC_ASSERT('$' < 128 && '_' < 128);
        if (IsLetter(c)) {
            identStart = userbuf.addressOfNextRawChar() - 1;
            hadUnicodeEscape = false;
            goto identifier;
        }

        goto badchar;
    }

    







    c1kind = FirstCharKind(firstCharKinds[c]);

    




    if (c1kind >= Space) {
        if (c1kind == EOL) {
            
            if (c == '\r' && userbuf.hasRawChars())
                userbuf.matchRawChar('\n');
            updateLineInfoForEOL();
            updateFlagsForEOL();
        }
        goto retry;
    }

    tp = newToken(-1);

    


    if (c1kind == OneChar) {
        tt = (TokenKind)oneCharTokens[c];
        goto out;
    }

    


    if (c1kind == Ident) {
        identStart = userbuf.addressOfNextRawChar() - 1;
        hadUnicodeEscape = false;

      identifier:
        for (;;) {
            c = getCharIgnoreEOL();
            if (c == EOF)
                break;
            if (!IsIdentifierPart(c)) {
                if (c != '\\' || !matchUnicodeEscapeIdent(&qc))
                    break;
                hadUnicodeEscape = true;
            }
        }
        ungetCharIgnoreEOL(c);

        
        if (hadUnicodeEscape && !putIdentInTokenbuf(identStart))
            goto error;

        
        if (!(flags & TSF_KEYWORD_IS_NAME)) {
            const jschar *chars;
            size_t length;
            if (hadUnicodeEscape) {
                chars = tokenbuf.begin();
                length = tokenbuf.length();
            } else {
                chars = identStart;
                length = userbuf.addressOfNextRawChar() - identStart;
            }
            tt = TOK_NAME;
            if (!checkForKeyword(chars, length, &tt, &tp->t_op))
                goto error;
            if (tt != TOK_NAME)
                goto out;
        }

        




        JSAtom *atom;
        if (!hadUnicodeEscape)
            atom = AtomizeChars(cx, identStart, userbuf.addressOfNextRawChar() - identStart);
        else
            atom = atomize(cx, tokenbuf);
        if (!atom)
            goto error;
        tp->setName(JSOP_NAME, atom->asPropertyName());
        tt = TOK_NAME;
        goto out;
    }

    if (c1kind == Dot) {
        c = getCharIgnoreEOL();
        if (JS7_ISDEC(c)) {
            numStart = userbuf.addressOfNextRawChar() - 2;
            goto decimal_dot;
        }
        if (c == '.') {
            qc = getCharIgnoreEOL();
            if (qc == '.') {
                tt = TOK_TRIPLEDOT;
                goto out;
            }
            ungetCharIgnoreEOL(qc);
#if JS_HAS_XML_SUPPORT
            tt = TOK_DBLDOT;
            goto out;
#endif
        }
        ungetCharIgnoreEOL(c);
        tt = TOK_DOT;
        goto out;
    }

    if (c1kind == Equals) {
        if (matchChar('=')) {
            if (matchChar('=')) {
                tp->t_op = JSOP_STRICTEQ;
                tt = TOK_STRICTEQ;
            } else {
                tp->t_op = JSOP_EQ;
                tt = TOK_EQ;
            }
        } else {
            tp->t_op = JSOP_NOP;
            tt = TOK_ASSIGN;
        }
        goto out;
    }

    


    if (c1kind == String) {
        qc = c;
        tokenbuf.clear();
        while (true) {
            






            c = getCharIgnoreEOL();
            if (maybeStrSpecial[c & 0xff]) {
                if (c == qc)
                    break;
                if (c == '\\') {
                    switch (c = getChar()) {
                      case 'b': c = '\b'; break;
                      case 'f': c = '\f'; break;
                      case 'n': c = '\n'; break;
                      case 'r': c = '\r'; break;
                      case 't': c = '\t'; break;
                      case 'v': c = '\v'; break;

                      default:
                        if ('0' <= c && c < '8') {
                            int32_t val = JS7_UNDEC(c);

                            c = peekChar();
                            
                            if (val != 0 || JS7_ISDEC(c)) {
                                if (!reportStrictModeError(JSMSG_DEPRECATED_OCTAL))
                                    goto error;
                            }
                            if ('0' <= c && c < '8') {
                                val = 8 * val + JS7_UNDEC(c);
                                getChar();
                                c = peekChar();
                                if ('0' <= c && c < '8') {
                                    int32_t save = val;
                                    val = 8 * val + JS7_UNDEC(c);
                                    if (val <= 0377)
                                        getChar();
                                    else
                                        val = save;
                                }
                            }

                            c = (jschar)val;
                        } else if (c == 'u') {
                            jschar cp[4];
                            if (peekChars(4, cp) &&
                                JS7_ISHEX(cp[0]) && JS7_ISHEX(cp[1]) &&
                                JS7_ISHEX(cp[2]) && JS7_ISHEX(cp[3])) {
                                c = (((((JS7_UNHEX(cp[0]) << 4)
                                        + JS7_UNHEX(cp[1])) << 4)
                                      + JS7_UNHEX(cp[2])) << 4)
                                    + JS7_UNHEX(cp[3]);
                                skipChars(4);
                            } else {
                                reportError(JSMSG_MALFORMED_ESCAPE, "Unicode");
                                goto error;
                            }
                        } else if (c == 'x') {
                            jschar cp[2];
                            if (peekChars(2, cp) &&
                                JS7_ISHEX(cp[0]) && JS7_ISHEX(cp[1])) {
                                c = (JS7_UNHEX(cp[0]) << 4) + JS7_UNHEX(cp[1]);
                                skipChars(2);
                            } else {
                                reportError(JSMSG_MALFORMED_ESCAPE, "hexadecimal");
                                goto error;
                            }
                        } else if (c == '\n') {
                            



                            continue;
                        }
                        break;
                    }
                } else if (TokenBuf::isRawEOLChar(c) || c == EOF) {
                    ungetCharIgnoreEOL(c);
                    reportError(JSMSG_UNTERMINATED_STRING);
                    goto error;
                }
            }
            if (!tokenbuf.append(c))
                goto error;
        }
        JSAtom *atom = atomize(cx, tokenbuf);
        if (!atom)
            goto error;
        tp->pos.end.lineno = lineno;
        tp->setAtom(JSOP_STRING, atom);
        tt = TOK_STRING;
        goto out;
    }

    


    if (c1kind == Dec) {
        numStart = userbuf.addressOfNextRawChar() - 1;

      decimal:
        hasFracOrExp = false;
        while (JS7_ISDEC(c))
            c = getCharIgnoreEOL();

        if (c == '.') {
          decimal_dot:
            hasFracOrExp = true;
            do {
                c = getCharIgnoreEOL();
            } while (JS7_ISDEC(c));
        }
        if (c == 'e' || c == 'E') {
            hasFracOrExp = true;
            c = getCharIgnoreEOL();
            if (c == '+' || c == '-')
                c = getCharIgnoreEOL();
            if (!JS7_ISDEC(c)) {
                ungetCharIgnoreEOL(c);
                reportError(JSMSG_MISSING_EXPONENT);
                goto error;
            }
            do {
                c = getCharIgnoreEOL();
            } while (JS7_ISDEC(c));
        }
        ungetCharIgnoreEOL(c);

        if (c != EOF && IsIdentifierStart(c)) {
            reportError(JSMSG_IDSTART_AFTER_NUMBER);
            goto error;
        }

        




        double dval;
        const jschar *dummy;
        if (!hasFracOrExp) {
            if (!GetPrefixInteger(cx, numStart, userbuf.addressOfNextRawChar(), 10, &dummy, &dval))
                goto error;
        } else {
            if (!js_strtod(cx, numStart, userbuf.addressOfNextRawChar(), &dummy, &dval))
                goto error;
        }
        tp->setNumber(dval);
        tt = TOK_NUMBER;
        goto out;
    }

    if (c1kind == Colon) {
#if JS_HAS_XML_SUPPORT
        if (matchChar(':')) {
            tt = TOK_DBLCOLON;
            goto out;
        }
#endif
        tp->t_op = JSOP_NOP;
        tt = TOK_COLON;
        goto out;
    }

    if (c1kind == Plus) {
        if (matchChar('=')) {
            tp->t_op = JSOP_ADD;
            tt = TOK_ADDASSIGN;
        } else if (matchChar('+')) {
            tt = TOK_INC;
        } else {
            tp->t_op = JSOP_POS;
            tt = TOK_PLUS;
        }
        goto out;
    }

    


    if (c1kind == HexOct) {
        int radix;
        c = getCharIgnoreEOL();
        if (c == 'x' || c == 'X') {
            radix = 16;
            c = getCharIgnoreEOL();
            if (!JS7_ISHEX(c)) {
                ungetCharIgnoreEOL(c);
                reportError(JSMSG_MISSING_HEXDIGITS);
                goto error;
            }
            numStart = userbuf.addressOfNextRawChar() - 1;  
            while (JS7_ISHEX(c))
                c = getCharIgnoreEOL();
        } else if (JS7_ISDEC(c)) {
            radix = 8;
            numStart = userbuf.addressOfNextRawChar() - 1;  
            while (JS7_ISDEC(c)) {
                
                if (!reportStrictModeError(JSMSG_DEPRECATED_OCTAL))
                    goto error;

                





                if (c >= '8') {
                    if (!reportWarning(JSMSG_BAD_OCTAL, c == '8' ? "08" : "09")) {
                        goto error;
                    }
                    goto decimal;   
                }
                c = getCharIgnoreEOL();
            }
        } else {
            
            numStart = userbuf.addressOfNextRawChar() - 1;
            goto decimal;
        }
        ungetCharIgnoreEOL(c);

        if (c != EOF && IsIdentifierStart(c)) {
            reportError(JSMSG_IDSTART_AFTER_NUMBER);
            goto error;
        }

        double dval;
        const jschar *dummy;
        if (!GetPrefixInteger(cx, numStart, userbuf.addressOfNextRawChar(), radix, &dummy, &dval))
            goto error;
        tp->setNumber(dval);
        tt = TOK_NUMBER;
        goto out;
    }

    


    JS_ASSERT(c1kind == Other);
    switch (c) {
      case '\\':
        hadUnicodeEscape = matchUnicodeEscapeIdStart(&qc);
        if (hadUnicodeEscape) {
            identStart = userbuf.addressOfNextRawChar() - 6;
            goto identifier;
        }
        goto badchar;

      case '|':
        if (matchChar(c)) {
            tt = TOK_OR;
        } else if (matchChar('=')) {
            tp->t_op = JSOP_BITOR;
            tt = TOK_BITORASSIGN;
        } else {
            tt = TOK_BITOR;
        }
        break;

      case '^':
        if (matchChar('=')) {
            tp->t_op = JSOP_BITXOR;
            tt = TOK_BITXORASSIGN;
        } else {
            tt = TOK_BITXOR;
        }
        break;

      case '&':
        if (matchChar('&')) {
            tt = TOK_AND;
        } else if (matchChar('=')) {
            tp->t_op = JSOP_BITAND;
            tt = TOK_BITANDASSIGN;
        } else {
            tt = TOK_BITAND;
        }
        break;

      case '!':
        if (matchChar('=')) {
            if (matchChar('=')) {
                tp->t_op = JSOP_STRICTNE;
                tt = TOK_STRICTNE;
            } else {
                tp->t_op = JSOP_NE;
                tt = TOK_NE;
            }
        } else {
            tp->t_op = JSOP_NOT;
            tt = TOK_NOT;
        }
        break;

#if JS_HAS_XML_SUPPORT
      case '@':
        tt = TOK_AT;
        break;
#endif

      case '<':
#if JS_HAS_XML_SUPPORT
        if ((flags & TSF_OPERAND) && allowsXML() && (hasMoarXML() || peekChar() != '!')) {
            if (!getXMLMarkup(&tt, &tp))
                goto error;
            goto out;
        }
#endif

        
        if (matchChar('!')) {
            if (matchChar('-')) {
                if (matchChar('-')) {
                    flags |= TSF_IN_HTML_COMMENT;
                    goto skipline;
                }
                ungetChar('-');
            }
            ungetChar('!');
        }
        if (matchChar('<')) {
            tp->t_op = JSOP_LSH;
            tt = matchChar('=') ? TOK_LSHASSIGN : TOK_LSH;
        } else {
            if (matchChar('=')) {
                tp->t_op = JSOP_LE;
                tt = TOK_LE;
            } else {
                tp->t_op = JSOP_LT;
                tt = TOK_LT;
            }
        }
        break;

      case '>':
        if (matchChar('>')) {
            if (matchChar('>')) {
                tp->t_op = JSOP_URSH;
                tt = matchChar('=') ? TOK_URSHASSIGN : TOK_URSH;
            } else {
                tp->t_op = JSOP_RSH;
                tt = matchChar('=') ? TOK_RSHASSIGN : TOK_RSH;
            }
        } else {
            if (matchChar('=')) {
                tp->t_op = JSOP_GE;
                tt = TOK_GE;
            } else {
                tp->t_op = JSOP_GT;
                tt = TOK_GT;
            }
        }
        break;

      case '*':
        tp->t_op = JSOP_MUL;
        tt = matchChar('=') ? TOK_MULASSIGN : TOK_STAR;
        break;

      case '/':
        


        if (matchChar('/')) {
            if (cx->hasAtLineOption() && !getAtLine())
                goto error;

            if (!getAtSourceMappingURL())
                goto error;

  skipline:
            
            if (flags & TSF_IN_HTML_COMMENT) {
                while ((c = getChar()) != EOF && c != '\n') {
                    if (c == '-' && matchChar('-') && matchChar('>'))
                        flags &= ~TSF_IN_HTML_COMMENT;
                }
            } else {
                while ((c = getChar()) != EOF && c != '\n')
                    continue;
            }
            ungetChar(c);
            cursor = (cursor - 1) & ntokensMask;
            goto retry;
        }

        


        if (matchChar('*')) {
            unsigned linenoBefore = lineno;
            while ((c = getChar()) != EOF &&
                   !(c == '*' && matchChar('/'))) {
                
            }
            if (c == EOF) {
                reportError(JSMSG_UNTERMINATED_COMMENT);
                goto error;
            }
            if (linenoBefore != lineno)
                updateFlagsForEOL();
            cursor = (cursor - 1) & ntokensMask;
            goto retry;
        }

        


        if (flags & TSF_OPERAND) {
            tokenbuf.clear();

            bool inCharClass = false;
            for (;;) {
                c = getChar();
                if (c == '\\') {
                    if (!tokenbuf.append(c))
                        goto error;
                    c = getChar();
                } else if (c == '[') {
                    inCharClass = true;
                } else if (c == ']') {
                    inCharClass = false;
                } else if (c == '/' && !inCharClass) {
                    
                    break;
                }
                if (c == '\n' || c == EOF) {
                    ungetChar(c);
                    reportError(JSMSG_UNTERMINATED_REGEXP);
                    goto error;
                }
                if (!tokenbuf.append(c))
                    goto error;
            }

            RegExpFlag reflags = NoFlags;
            unsigned length = tokenbuf.length() + 1;
            while (true) {
                c = peekChar();
                if (c == 'g' && !(reflags & GlobalFlag))
                    reflags = RegExpFlag(reflags | GlobalFlag);
                else if (c == 'i' && !(reflags & IgnoreCaseFlag))
                    reflags = RegExpFlag(reflags | IgnoreCaseFlag);
                else if (c == 'm' && !(reflags & MultilineFlag))
                    reflags = RegExpFlag(reflags | MultilineFlag);
                else if (c == 'y' && !(reflags & StickyFlag))
                    reflags = RegExpFlag(reflags | StickyFlag);
                else
                    break;
                getChar();
                length++;
            }

            c = peekChar();
            if (JS7_ISLET(c)) {
                char buf[2] = { '\0', '\0' };
                tp->pos.begin.index += length + 1;
                buf[0] = char(c);
                reportError(JSMSG_BAD_REGEXP_FLAG, buf);
                (void) getChar();
                goto error;
            }
            tp->setRegExpFlags(reflags);
            tt = TOK_REGEXP;
            break;
        }

        tp->t_op = JSOP_DIV;
        tt = matchChar('=') ? TOK_DIVASSIGN : TOK_DIV;
        break;

      case '%':
        tp->t_op = JSOP_MOD;
        tt = matchChar('=') ? TOK_MODASSIGN : TOK_MOD;
        break;

      case '~':
        tp->t_op = JSOP_BITNOT;
        tt = TOK_BITNOT;
        break;

      case '-':
        if (matchChar('=')) {
            tp->t_op = JSOP_SUB;
            tt = TOK_SUBASSIGN;
        } else if (matchChar(c)) {
            if (peekChar() == '>' && !(flags & TSF_DIRTYLINE)) {
                flags &= ~TSF_IN_HTML_COMMENT;
                goto skipline;
            }
            tt = TOK_DEC;
        } else {
            tp->t_op = JSOP_NEG;
            tt = TOK_MINUS;
        }
        break;

      badchar:
      default:
        reportError(JSMSG_ILLEGAL_CHARACTER);
        goto error;
    }

  out:
    flags |= TSF_DIRTYLINE;
    tp->pos.end.index = userbuf.addressOfNextRawChar() - linebase;
    tp->type = tt;
    JS_ASSERT(IsTokenSane(tp));
    return tt;

  error:
    





    flags |= TSF_DIRTYLINE;
    tp->pos.end.index = tp->pos.begin.index + 1;
    tp->type = TOK_ERROR;
    JS_ASSERT(IsTokenSane(tp));
    onError();
    return TOK_ERROR;
}

void
TokenStream::onError()
{
    flags |= TSF_HAD_ERROR;
#ifdef DEBUG
    








    userbuf.poison();
#endif
}

JS_FRIEND_API(int)
js_fgets(char *buf, int size, FILE *file)
{
    int n, i, c;
    bool crflag;

    n = size - 1;
    if (n < 0)
        return -1;

    crflag = false;
    for (i = 0; i < n && (c = fast_getc(file)) != EOF; i++) {
        buf[i] = c;
        if (c == '\n') {        
            i++;                
            break;
        }
        if (crflag) {           
            ungetc(c, file);
            break;              
        }
        crflag = (c == '\r');
    }

    buf[i] = '\0';
    return i;
}

#ifdef DEBUG
const char *
TokenKindToString(TokenKind tt)
{
    switch (tt) {
      case TOK_ERROR:           return "TOK_ERROR";
      case TOK_EOF:             return "TOK_EOF";
      case TOK_EOL:             return "TOK_EOL";
      case TOK_SEMI:            return "TOK_SEMI";
      case TOK_COMMA:           return "TOK_COMMA";
      case TOK_HOOK:            return "TOK_HOOK";
      case TOK_COLON:           return "TOK_COLON";
      case TOK_OR:              return "TOK_OR";
      case TOK_AND:             return "TOK_AND";
      case TOK_BITOR:           return "TOK_BITOR";
      case TOK_BITXOR:          return "TOK_BITXOR";
      case TOK_BITAND:          return "TOK_BITAND";
      case TOK_PLUS:            return "TOK_PLUS";
      case TOK_MINUS:           return "TOK_MINUS";
      case TOK_STAR:            return "TOK_STAR";
      case TOK_DIV:             return "TOK_DIV";
      case TOK_MOD:             return "TOK_MOD";
      case TOK_INC:             return "TOK_INC";
      case TOK_DEC:             return "TOK_DEC";
      case TOK_DOT:             return "TOK_DOT";
      case TOK_TRIPLEDOT:       return "TOK_TRIPLEDOT";
      case TOK_LB:              return "TOK_LB";
      case TOK_RB:              return "TOK_RB";
      case TOK_LC:              return "TOK_LC";
      case TOK_RC:              return "TOK_RC";
      case TOK_LP:              return "TOK_LP";
      case TOK_RP:              return "TOK_RP";
      case TOK_NAME:            return "TOK_NAME";
      case TOK_NUMBER:          return "TOK_NUMBER";
      case TOK_STRING:          return "TOK_STRING";
      case TOK_REGEXP:          return "TOK_REGEXP";
      case TOK_TRUE:            return "TOK_TRUE";
      case TOK_FALSE:           return "TOK_FALSE";
      case TOK_NULL:            return "TOK_NULL";
      case TOK_THIS:            return "TOK_THIS";
      case TOK_FUNCTION:        return "TOK_FUNCTION";
      case TOK_IF:              return "TOK_IF";
      case TOK_ELSE:            return "TOK_ELSE";
      case TOK_SWITCH:          return "TOK_SWITCH";
      case TOK_CASE:            return "TOK_CASE";
      case TOK_DEFAULT:         return "TOK_DEFAULT";
      case TOK_WHILE:           return "TOK_WHILE";
      case TOK_DO:              return "TOK_DO";
      case TOK_FOR:             return "TOK_FOR";
      case TOK_BREAK:           return "TOK_BREAK";
      case TOK_CONTINUE:        return "TOK_CONTINUE";
      case TOK_IN:              return "TOK_IN";
      case TOK_VAR:             return "TOK_VAR";
      case TOK_CONST:           return "TOK_CONST";
      case TOK_WITH:            return "TOK_WITH";
      case TOK_RETURN:          return "TOK_RETURN";
      case TOK_NEW:             return "TOK_NEW";
      case TOK_DELETE:          return "TOK_DELETE";
      case TOK_TRY:             return "TOK_TRY";
      case TOK_CATCH:           return "TOK_CATCH";
      case TOK_FINALLY:         return "TOK_FINALLY";
      case TOK_THROW:           return "TOK_THROW";
      case TOK_INSTANCEOF:      return "TOK_INSTANCEOF";
      case TOK_DEBUGGER:        return "TOK_DEBUGGER";
      case TOK_XMLSTAGO:        return "TOK_XMLSTAGO";
      case TOK_XMLETAGO:        return "TOK_XMLETAGO";
      case TOK_XMLPTAGC:        return "TOK_XMLPTAGC";
      case TOK_XMLTAGC:         return "TOK_XMLTAGC";
      case TOK_XMLNAME:         return "TOK_XMLNAME";
      case TOK_XMLATTR:         return "TOK_XMLATTR";
      case TOK_XMLSPACE:        return "TOK_XMLSPACE";
      case TOK_XMLTEXT:         return "TOK_XMLTEXT";
      case TOK_XMLCOMMENT:      return "TOK_XMLCOMMENT";
      case TOK_XMLCDATA:        return "TOK_XMLCDATA";
      case TOK_XMLPI:           return "TOK_XMLPI";
      case TOK_AT:              return "TOK_AT";
      case TOK_DBLCOLON:        return "TOK_DBLCOLON";
      case TOK_DBLDOT:          return "TOK_DBLDOT";
      case TOK_FILTER:          return "TOK_FILTER";
      case TOK_XMLELEM:         return "TOK_XMLELEM";
      case TOK_XMLLIST:         return "TOK_XMLLIST";
      case TOK_YIELD:           return "TOK_YIELD";
      case TOK_LEXICALSCOPE:    return "TOK_LEXICALSCOPE";
      case TOK_LET:             return "TOK_LET";
      case TOK_RESERVED:        return "TOK_RESERVED";
      case TOK_STRICT_RESERVED: return "TOK_STRICT_RESERVED";
      case TOK_STRICTEQ:        return "TOK_STRICTEQ";
      case TOK_EQ:              return "TOK_EQ";
      case TOK_STRICTNE:        return "TOK_STRICTNE";
      case TOK_NE:              return "TOK_NE";
      case TOK_TYPEOF:          return "TOK_TYPEOF";
      case TOK_VOID:            return "TOK_VOID";
      case TOK_NOT:             return "TOK_NOT";
      case TOK_BITNOT:          return "TOK_BITNOT";
      case TOK_LT:              return "TOK_LT";
      case TOK_LE:              return "TOK_LE";
      case TOK_GT:              return "TOK_GT";
      case TOK_GE:              return "TOK_GE";
      case TOK_LSH:             return "TOK_LSH";
      case TOK_RSH:             return "TOK_RSH";
      case TOK_URSH:            return "TOK_URSH";
      case TOK_ASSIGN:          return "TOK_ASSIGN";
      case TOK_ADDASSIGN:       return "TOK_ADDASSIGN";
      case TOK_SUBASSIGN:       return "TOK_SUBASSIGN";
      case TOK_BITORASSIGN:     return "TOK_BITORASSIGN";
      case TOK_BITXORASSIGN:    return "TOK_BITXORASSIGN";
      case TOK_BITANDASSIGN:    return "TOK_BITANDASSIGN";
      case TOK_LSHASSIGN:       return "TOK_LSHASSIGN";
      case TOK_RSHASSIGN:       return "TOK_RSHASSIGN";
      case TOK_URSHASSIGN:      return "TOK_URSHASSIGN";
      case TOK_MULASSIGN:       return "TOK_MULASSIGN";
      case TOK_DIVASSIGN:       return "TOK_DIVASSIGN";
      case TOK_MODASSIGN:       return "TOK_MODASSIGN";
      case TOK_EXPORT:          return "TOK_EXPORT";
      case TOK_IMPORT:          return "TOK_IMPORT";
      case TOK_LIMIT:           break;
    }

    return "<bad TokenKind>";
}
#endif
