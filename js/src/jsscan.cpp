










































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
#include "jsstdint.h"
#include "jsarena.h"
#include "jsbit.h"
#include "jsutil.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsemit.h"
#include "jsexn.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsparse.h"
#include "jsregexp.h"
#include "jsscan.h"
#include "jsscript.h"
#include "jsstaticcheck.h"
#include "jsvector.h"

#include "jsscriptinlines.h"

#if JS_HAS_XML_SUPPORT
#include "jsxml.h"
#endif

using namespace js;

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

namespace js {

const KeywordInfo *
FindKeyword(const jschar *s, size_t length)
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

} 

JSBool
js_IsIdentifier(JSLinearString *str)
{
    const jschar *chars = str->chars();
    size_t length = str->length();

    if (length == 0)
        return JS_FALSE;
    jschar c = *chars;
    if (!JS_ISIDSTART(c))
        return JS_FALSE;
    const jschar *end = chars + length;
    while (++chars != end) {
        c = *chars;
        if (!JS_ISIDENT(c))
            return JS_FALSE;
    }
    return JS_TRUE;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4351)
#endif


TokenStream::TokenStream(JSContext *cx)
  : cx(cx), tokens(), cursor(), lookahead(), flags(), listenerTSData(), tokenbuf(cx)
{}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

bool
TokenStream::init(const jschar *base, size_t length, const char *fn, uintN ln, JSVersion v)
{
    filename = fn;
    lineno = ln;
    version = v;
    xml = VersionHasXML(v);

    userbuf.init(base, length);
    linebase = base;
    prevLinebase = NULL;

    listener = cx->debugHooks->sourceHandler;
    listenerData = cx->debugHooks->sourceHandlerData;

    













    memset(oneCharTokens, 0, sizeof(oneCharTokens));
    oneCharTokens[';'] = TOK_SEMI;
    oneCharTokens[','] = TOK_COMMA;
    oneCharTokens['?'] = TOK_HOOK;
    oneCharTokens['['] = TOK_LB;
    oneCharTokens[']'] = TOK_RB;
    oneCharTokens['{'] = TOK_LC;
    oneCharTokens['}'] = TOK_RC;
    oneCharTokens['('] = TOK_LP;
    oneCharTokens[')'] = TOK_RP;

    
    memset(maybeEOL, 0, sizeof(maybeEOL));
    maybeEOL['\n'] = true;
    maybeEOL['\r'] = true;
    maybeEOL[LINE_SEPARATOR & 0xff] = true;
    maybeEOL[PARA_SEPARATOR & 0xff] = true;

    
    memset(maybeStrSpecial, 0, sizeof(maybeStrSpecial));
    maybeStrSpecial['"'] = true;
    maybeStrSpecial['\''] = true;
    maybeStrSpecial['\\'] = true;
    maybeStrSpecial['\n'] = true;
    maybeStrSpecial['\r'] = true;
    maybeStrSpecial[LINE_SEPARATOR & 0xff] = true;
    maybeStrSpecial[PARA_SEPARATOR & 0xff] = true;
    maybeStrSpecial[EOF & 0xff] = true;
    return true;
}

TokenStream::~TokenStream()
{
    if (flags & TSF_OWNFILENAME)
        cx->free_((void *) filename);
}


#if defined(HAVE_GETC_UNLOCKED)
# define fast_getc getc_unlocked
#elif defined(HAVE__GETC_NOLOCK)
# define fast_getc _getc_nolock
#else
# define fast_getc getc
#endif

JS_FRIEND_API(int)
js_fgets(char *buf, int size, FILE *file)
{
    int n, i, c;
    JSBool crflag;

    n = size - 1;
    if (n < 0)
        return -1;

    crflag = JS_FALSE;
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


int32
TokenStream::getChar()
{
    int32 c;
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
    prevLinebase = linebase;
    linebase = userbuf.addressOfNextRawChar();
    lineno++;
    return '\n';
}








int32
TokenStream::getCharIgnoreEOL()
{
    if (JS_LIKELY(userbuf.hasRawChars()))
        return userbuf.getRawChar();

    flags |= TSF_EOF;
    return EOF;
}

void
TokenStream::ungetChar(int32 c)
{
    if (c == EOF)
        return;
    JS_ASSERT(!userbuf.atStart());
    userbuf.ungetRawChar();
    if (c == '\n') {
#ifdef DEBUG
        int32 c2 = userbuf.peekRawChar();
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
TokenStream::ungetCharIgnoreEOL(int32 c)
{
    if (c == EOF)
        return;
    JS_ASSERT(!userbuf.atStart());
    userbuf.ungetRawChar();
}






JSBool
TokenStream::peekChars(intN n, jschar *cp)
{
    intN i, j;
    int32 c;

    for (i = 0; i < n; i++) {
        c = getChar();
        if (c == EOF)
            break;
        if (c == '\n') {
            ungetChar(c);
            break;
        }
        cp[i] = (jschar)c;
    }
    for (j = i - 1; j >= 0; j--)
        ungetChar(cp[j]);
    return i == n;
}

const jschar *
TokenStream::TokenBuf::findEOL()
{
    const jschar *tmp = ptr;
#ifdef DEBUG
    




    if (!tmp)
        tmp = ptrWhenPoisoned;
#endif

    while (true) {
        if (tmp >= limit)
            break;
        if (TokenBuf::isRawEOLChar(*tmp++))
            break;
    }
    return tmp;
}

bool
TokenStream::reportCompileErrorNumberVA(JSParseNode *pn, uintN flags, uintN errorNumber,
                                        va_list ap)
{
    JSErrorReport report;
    char *message;
    jschar *linechars;
    char *linebytes;
    bool warning;
    JSBool ok;
    const TokenPos *tp;
    uintN i;

    if (JSREPORT_IS_STRICT(flags) && !cx->hasStrictOption())
        return JS_TRUE;

    warning = JSREPORT_IS_WARNING(flags);
    if (warning && cx->hasWErrorOption()) {
        flags &= ~JSREPORT_WARNING;
        warning = false;
    }

    PodZero(&report);
    report.flags = flags;
    report.errorNumber = errorNumber;
    message = NULL;
    linechars = NULL;
    linebytes = NULL;

    MUST_FLOW_THROUGH("out");
    ok = js_ExpandErrorArguments(cx, js_GetErrorMessage, NULL,
                                 errorNumber, &message, &report,
                                 !(flags & JSREPORT_UC), ap);
    if (!ok) {
        warning = false;
        goto out;
    }

    report.filename = filename;

    tp = pn ? &pn->pn_pos : &currentToken().pos;
    report.lineno = tp->begin.lineno;

    









    if (report.lineno == lineno) {
        size_t linelength = userbuf.findEOL() - linebase;

        linechars = (jschar *)cx->malloc_((linelength + 1) * sizeof(jschar));
        if (!linechars) {
            warning = false;
            goto out;
        }
        memcpy(linechars, linebase, linelength * sizeof(jschar));
        linechars[linelength] = 0;
        linebytes = js_DeflateString(cx, linechars, linelength);
        if (!linebytes) {
            warning = false;
            goto out;
        }

        
        report.linebuf = linebytes;
        report.uclinebuf = linechars;

        
        JS_ASSERT(tp->begin.lineno == tp->end.lineno);
        report.tokenptr = report.linebuf + tp->begin.index;
        report.uctokenptr = report.uclinebuf + tp->begin.index;
    }

    












    if (!js_ErrorToException(cx, message, &report, NULL, NULL)) {
        



        bool reportError = true;
        if (JSDebugErrorHook hook = cx->debugHooks->debugErrorHook)
            reportError = hook(cx, message, &report, cx->debugHooks->debugErrorHookData);

        
        if (reportError && cx->errorReporter)
            cx->errorReporter(cx, message, &report);
    }

  out:
    if (linebytes)
        cx->free_(linebytes);
    if (linechars)
        cx->free_(linechars);
    if (message)
        cx->free_(message);
    if (report.ucmessage)
        cx->free_((void *)report.ucmessage);

    if (report.messageArgs) {
        if (!(flags & JSREPORT_UC)) {
            i = 0;
            while (report.messageArgs[i])
                cx->free_((void *)report.messageArgs[i++]);
        }
        cx->free_((void *)report.messageArgs);
    }

    return warning;
}

bool
js::ReportStrictModeError(JSContext *cx, TokenStream *ts, JSTreeContext *tc, JSParseNode *pn,
                          uintN errorNumber, ...)
{
    JS_ASSERT(ts || tc);
    JS_ASSERT(cx == ts->getContext());

    
    uintN flags;
    if ((ts && ts->isStrictMode()) || (tc && (tc->flags & TCF_STRICT_MODE_CODE))) {
        flags = JSREPORT_ERROR;
    } else {
        if (!cx->hasStrictOption())
            return true;
        flags = JSREPORT_WARNING;
    }

    va_list ap;
    va_start(ap, errorNumber);
    bool result = ts->reportCompileErrorNumberVA(pn, flags, errorNumber, ap);
    va_end(ap);

    return result;
}

bool
js::ReportCompileErrorNumber(JSContext *cx, TokenStream *ts, JSParseNode *pn,
                             uintN flags, uintN errorNumber, ...)
{
    va_list ap;

    




    JS_ASSERT(!(flags & JSREPORT_STRICT_MODE_ERROR));

    va_start(ap, errorNumber);
    JS_ASSERT(cx == ts->getContext());
    bool result = ts->reportCompileErrorNumberVA(pn, flags, errorNumber, ap);
    va_end(ap);

    return result;
}

#if JS_HAS_XML_SUPPORT

JSBool
TokenStream::getXMLEntity()
{
    ptrdiff_t offset, length, i;
    int c, d;
    JSBool ispair;
    jschar *bp, digit;
    char *bytes;
    JSErrNum msg;

    CharBuffer &tb = tokenbuf;

    
    offset = tb.length();
    if (!tb.append('&'))
        return JS_FALSE;
    while ((c = getChar()) != ';') {
        if (c == EOF || c == '\n') {
            ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_END_OF_XML_ENTITY);
            return JS_FALSE;
        }
        if (!tb.append(c))
            return JS_FALSE;
    }

    
    length = tb.length() - offset;
    bp = tb.begin() + offset;
    c = d = 0;
    ispair = JS_FALSE;
    if (length > 2 && bp[1] == '#') {
        
        i = 2;
        if (length > 3 && JS_TOLOWER(bp[i]) == 'x') {
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
            ispair = JS_TRUE;
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
    return JS_TRUE;

  badncr:
    msg = JSMSG_BAD_XML_NCR;
  bad:
    
    JS_ASSERT((tb.end() - bp) >= 1);
    bytes = js_DeflateString(cx, bp + 1, (tb.end() - bp) - 1);
    if (bytes) {
        ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, msg, bytes);
        cx->free_(bytes);
    }
    return JS_FALSE;
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
TokenStream::matchUnicodeEscapeIdStart(int32 *cp)
{
    if (peekUnicodeEscape(cp) && JS_ISIDSTART(*cp)) {
        skipChars(5);
        return true;
    }
    return false;
}

bool
TokenStream::matchUnicodeEscapeIdent(int32 *cp)
{
    if (peekUnicodeEscape(cp) && JS_ISIDENT(*cp)) {
        skipChars(5);
        return true;
    }
    return false;
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
    return js_AtomizeChars(cx, cb.begin(), cb.length(), 0);
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
    int32 c, qc;
    const jschar *tmp = userbuf.addressOfNextRawChar(); 
    userbuf.setAddressOfNextRawChar(identStart);

    tokenbuf.clear();
    for (;;) {
        c = getCharIgnoreEOL();
        if (!JS_ISIDENT(c)) {
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

TokenKind
TokenStream::getTokenInternal()
{
    TokenKind tt;
    int c, qc;
    Token *tp;
    JSAtom *atom;
    bool hadUnicodeEscape;
    const jschar *numStart;
#if JS_HAS_XML_SUPPORT
    JSBool inTarget;
    size_t targetLength;
    ptrdiff_t contentIndex;
#endif

#if JS_HAS_XML_SUPPORT
    


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

            if (!JS_ISXMLSPACE(c))
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
        tp->t_op = JSOP_STRING;
        tp->t_atom = atom;
        goto out;
    }

    


    if (flags & TSF_XMLTAGMODE) {
        tp = newToken(0);
        c = getChar();
        if (JS_ISXMLSPACE(c)) {
            do {
                c = getChar();
            } while (JS_ISXMLSPACE(c));
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
        if (JS_ISXMLNSSTART(c)) {
            JSBool sawColon = JS_FALSE;

            if (!tokenbuf.append(c))
                goto error;
            while ((c = getChar()) != EOF && JS_ISXMLNAME(c)) {
                if (c == ':') {
                    int nextc;

                    if (sawColon ||
                        (nextc = peekChar(),
                         ((flags & TSF_XMLONLYMODE) || nextc != '{') &&
                         !JS_ISXMLNAME(nextc))) {
                        ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                                 JSMSG_BAD_XML_QNAME);
                        goto error;
                    }
                    sawColon = JS_TRUE;
                }

                if (!tokenbuf.append(c))
                    goto error;
            }

            ungetChar(c);
            atom = atomize(cx, tokenbuf);
            if (!atom)
                goto error;
            tp->t_op = JSOP_STRING;
            tp->t_atom = atom;
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
                    ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                             JSMSG_UNTERMINATED_STRING);
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
            tp->t_op = JSOP_STRING;
            tp->t_atom = atom;
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
            ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_BAD_XML_CHARACTER);
            goto error;
        }
        
    }
#endif 

  retry:
    


    do {
        c = getChar();
        if (c == '\n') {
            flags &= ~TSF_DIRTYLINE;
            flags |= TSF_EOL;
            continue;
        }
    } while (JS_ISSPACE_OR_BOM((jschar)c));

    if (c == EOF) {
        tp = newToken(0);   
        tt = TOK_EOF;
        goto out;
    }
    tp = newToken(-1);

    


    if (c < 128) {
        tt = (TokenKind)oneCharTokens[c];
        if (tt != 0)
            goto out;
    }

    


    hadUnicodeEscape = false;
    if (JS_ISIDSTART(c) ||
        (c == '\\' && (hadUnicodeEscape = matchUnicodeEscapeIdStart(&qc))))
    {
        const jschar *identStart = userbuf.addressOfNextRawChar() - 1;
        if (hadUnicodeEscape) {
            c = qc;
            identStart -= 5;    
        }
        for (;;) {
            c = getCharIgnoreEOL();
            if (!JS_ISIDENT(c)) {
                if (c != '\\' || !matchUnicodeEscapeIdent(&qc))
                    break;
                hadUnicodeEscape = true;
            }
        }
        ungetCharIgnoreEOL(c);

        



        const KeywordInfo *kw;
        if (!hadUnicodeEscape &&
            !(flags & TSF_KEYWORD_IS_NAME) &&
            (kw = FindKeyword(identStart, userbuf.addressOfNextRawChar() - identStart))) {
            if (kw->tokentype == TOK_RESERVED) {
                if (!ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                              JSMSG_RESERVED_ID, kw->chars)) {
                    goto error;
                }
            } else if (kw->tokentype == TOK_STRICT_RESERVED) {
                if (isStrictMode()
                    ? !ReportStrictModeError(cx, this, NULL, NULL, JSMSG_RESERVED_ID, kw->chars)
                    : !ReportCompileErrorNumber(cx, this, NULL,
                                                JSREPORT_STRICT | JSREPORT_WARNING,
                                                JSMSG_RESERVED_ID, kw->chars)) {
                    goto error;
                }
            } else {
                if (kw->version <= versionNumber()) {
                    tt = kw->tokentype;
                    tp->t_op = (JSOp) kw->op;
                    goto out;
                }

                




                if ((kw->tokentype == TOK_LET || kw->tokentype == TOK_YIELD) &&
                    !ReportStrictModeError(cx, this, NULL, NULL, JSMSG_RESERVED_ID, kw->chars))
                {
                    goto error;
                }
            }
        }

        




        if (!hadUnicodeEscape)
            atom = js_AtomizeChars(cx, identStart, userbuf.addressOfNextRawChar() - identStart, 0);
        else if (putIdentInTokenbuf(identStart))
            atom = atomize(cx, tokenbuf);
        else
            atom = NULL;
        if (!atom)
            goto error;
        tp->t_op = JSOP_NAME;
        tp->t_atom = atom;
        tt = TOK_NAME;
        goto out;
    }

    


    if (JS7_ISDECNZ(c) || (c == '.' && JS7_ISDEC(peekChar()))) {
        numStart = userbuf.addressOfNextRawChar() - 1;

      decimal:
        bool hasFracOrExp = false;
        while (JS7_ISDEC(c))
            c = getCharIgnoreEOL();

        if (c == '.') {
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
                ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                         JSMSG_MISSING_EXPONENT);
                goto error;
            }
            do {
                c = getCharIgnoreEOL();
            } while (JS7_ISDEC(c));
        }
        ungetCharIgnoreEOL(c);

        if (JS_ISIDSTART(c)) {
            ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_IDSTART_AFTER_NUMBER);
            goto error;
        }

        




        jsdouble dval;
        const jschar *dummy;
        if (!hasFracOrExp) {
            if (!GetPrefixInteger(cx, numStart, userbuf.addressOfNextRawChar(), 10, &dummy, &dval))
                goto error;
        } else {
            if (!js_strtod(cx, numStart, userbuf.addressOfNextRawChar(), &dummy, &dval))
                goto error;
        }
        tp->t_dval = dval;
        tt = TOK_NUMBER;
        goto out;
    }

    


    if (c == '0') {
        int radix;
        c = getCharIgnoreEOL();
        if (c == 'x' || c == 'X') {
            radix = 16;
            c = getCharIgnoreEOL();
            if (!JS7_ISHEX(c)) {
                ungetCharIgnoreEOL(c);
                ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_MISSING_HEXDIGITS);
                goto error;
            }
            numStart = userbuf.addressOfNextRawChar() - 1;
            while (JS7_ISHEX(c))
                c = getCharIgnoreEOL();
        } else if (JS7_ISDEC(c)) {
            radix = 8;
            numStart = userbuf.addressOfNextRawChar() - 1;
            while (JS7_ISDEC(c)) {
                
                if (!ReportStrictModeError(cx, this, NULL, NULL, JSMSG_DEPRECATED_OCTAL))
                    goto error;

                





                if (c >= '8') {
                    if (!ReportCompileErrorNumber(cx, this, NULL, JSREPORT_WARNING,
                                                  JSMSG_BAD_OCTAL, c == '8' ? "08" : "09")) {
                        goto error;
                    }
                    goto decimal;   
                }
                c = getCharIgnoreEOL();
            }
        } else {
            
            radix = 10;
            numStart = userbuf.addressOfNextRawChar() - 1;
            goto decimal;
        }
        ungetCharIgnoreEOL(c);

        if (JS_ISIDSTART(c)) {
            ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_IDSTART_AFTER_NUMBER);
            goto error;
        }

        jsdouble dval;
        const jschar *dummy;
        if (!GetPrefixInteger(cx, numStart, userbuf.addressOfNextRawChar(), radix, &dummy, &dval))
            goto error;
        tp->t_dval = dval;
        tt = TOK_NUMBER;
        goto out;
    }

    


    if (c == '"' || c == '\'') {
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
                            int32 val = JS7_UNDEC(c);

                            c = peekChar();
                            
                            if (val != 0 || JS7_ISDEC(c)) {
                                if (!ReportStrictModeError(cx, this, NULL, NULL,
                                                           JSMSG_DEPRECATED_OCTAL)) {
                                    goto error;
                                }
                                setOctalCharacterEscape();
                            }
                            if ('0' <= c && c < '8') {
                                val = 8 * val + JS7_UNDEC(c);
                                getChar();
                                c = peekChar();
                                if ('0' <= c && c < '8') {
                                    int32 save = val;
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
                            }
                        } else if (c == 'x') {
                            jschar cp[2];
                            if (peekChars(2, cp) &&
                                JS7_ISHEX(cp[0]) && JS7_ISHEX(cp[1])) {
                                c = (JS7_UNHEX(cp[0]) << 4) + JS7_UNHEX(cp[1]);
                                skipChars(2);
                            }
                        } else if (c == '\n') {
                            
                            continue;
                        }
                        break;
                    }
                } else if (TokenBuf::isRawEOLChar(c) || c == EOF) {
                    ungetCharIgnoreEOL(c);
                    ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                             JSMSG_UNTERMINATED_STRING);
                    goto error;
                }
            }
            if (!tokenbuf.append(c))
                goto error;
        }
        atom = atomize(cx, tokenbuf);
        if (!atom)
            goto error;
        tp->pos.end.lineno = lineno;
        tp->t_op = JSOP_STRING;
        tp->t_atom = atom;
        tt = TOK_STRING;
        goto out;
    }

    


    switch (c) {
      case '.':
#if JS_HAS_XML_SUPPORT
        if (matchChar(c))
            tt = TOK_DBLDOT;
        else
#endif
            tt = TOK_DOT;
        break;

      case ':':
#if JS_HAS_XML_SUPPORT
        if (matchChar(c)) {
            tt = TOK_DBLCOLON;
            break;
        }
#endif
        



        tp->t_op = JSOP_NOP;
        tt = TOK_COLON;
        break;

      case '|':
        if (matchChar(c)) {
            tt = TOK_OR;
        } else if (matchChar('=')) {
            tp->t_op = JSOP_BITOR;
            tt = TOK_ASSIGN;
        } else {
            tt = TOK_BITOR;
        }
        break;

      case '^':
        if (matchChar('=')) {
            tp->t_op = JSOP_BITXOR;
            tt = TOK_ASSIGN;
        } else {
            tt = TOK_BITXOR;
        }
        break;

      case '&':
        if (matchChar(c)) {
            tt = TOK_AND;
        } else if (matchChar('=')) {
            tp->t_op = JSOP_BITAND;
            tt = TOK_ASSIGN;
        } else {
            tt = TOK_BITAND;
        }
        break;

      case '=':
        if (matchChar(c)) {
            tp->t_op = matchChar(c) ? JSOP_STRICTEQ : JSOP_EQ;
            tt = TOK_EQOP;
        } else {
            tp->t_op = JSOP_NOP;
            tt = TOK_ASSIGN;
        }
        break;

      case '!':
        if (matchChar('=')) {
            tp->t_op = matchChar('=') ? JSOP_STRICTNE : JSOP_NE;
            tt = TOK_EQOP;
        } else {
            tp->t_op = JSOP_NOT;
            tt = TOK_UNARYOP;
        }
        break;

#if JS_HAS_XML_SUPPORT
      case '@':
        tt = TOK_AT;
        break;
#endif

      case '<':
#if JS_HAS_XML_SUPPORT
        




























        if ((flags & TSF_OPERAND) && (hasXML() || peekChar() != '!')) {
            
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
                    tt = TOK_XMLCOMMENT;
                    tp->t_op = JSOP_XMLCOMMENT;
                    goto finish_xml_markup;
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
                        getChar();            
                        tt = TOK_XMLCDATA;
                        tp->t_op = JSOP_XMLCDATA;
                        goto finish_xml_markup;
                    }
                    goto bad_xml_markup;
                }
            }

            
            if (matchChar('?')) {
                inTarget = JS_TRUE;
                targetLength = 0;
                contentIndex = -1;

                tokenbuf.clear();
                while ((c = getChar()) != '?' || peekChar() != '>') {
                    if (c == EOF)
                        goto bad_xml_markup;
                    if (inTarget) {
                        if (JS_ISXMLSPACE(c)) {
                            if (tokenbuf.empty())
                                goto bad_xml_markup;
                            inTarget = JS_FALSE;
                        } else {
                            if (!(tokenbuf.empty()
                                  ? JS_ISXMLNSSTART(c)
                                  : JS_ISXMLNS(c))) {
                                goto bad_xml_markup;
                            }
                            ++targetLength;
                        }
                    } else {
                        if (contentIndex < 0 && !JS_ISXMLSPACE(c))
                            contentIndex = tokenbuf.length();
                    }
                    if (!tokenbuf.append(c))
                        goto error;
                }
                if (targetLength == 0)
                    goto bad_xml_markup;
                if (contentIndex < 0) {
                    atom = cx->runtime->atomState.emptyAtom;
                } else {
                    atom = js_AtomizeChars(cx, tokenbuf.begin() + contentIndex,
                                           tokenbuf.length() - contentIndex, 0);
                    if (!atom)
                        goto error;
                }
                tokenbuf.shrinkBy(tokenbuf.length() - targetLength);
                tp->t_atom2 = atom;
                tt = TOK_XMLPI;

        finish_xml_markup:
                if (!matchChar('>'))
                    goto bad_xml_markup;
                atom = atomize(cx, tokenbuf);
                if (!atom)
                    goto error;
                tp->t_atom = atom;
                tp->pos.end.lineno = lineno;
                goto out;
            }

            
            tt = matchChar('/') ? TOK_XMLETAGO : TOK_XMLSTAGO;
            goto out;

        bad_xml_markup:
            ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_BAD_XML_MARKUP);
            goto error;
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
        if (matchChar(c)) {
            tp->t_op = JSOP_LSH;
            tt = matchChar('=') ? TOK_ASSIGN : TOK_SHOP;
        } else {
            tp->t_op = matchChar('=') ? JSOP_LE : JSOP_LT;
            tt = TOK_RELOP;
        }
        break;

      case '>':
        if (matchChar(c)) {
            tp->t_op = matchChar(c) ? JSOP_URSH : JSOP_RSH;
            tt = matchChar('=') ? TOK_ASSIGN : TOK_SHOP;
        } else {
            tp->t_op = matchChar('=') ? JSOP_GE : JSOP_GT;
            tt = TOK_RELOP;
        }
        break;

      case '*':
        tp->t_op = JSOP_MUL;
        tt = matchChar('=') ? TOK_ASSIGN : TOK_STAR;
        break;

      case '/':
        if (matchChar('/')) {
            




            if (cx->hasAtLineOption()) {
                jschar cp[5];
                uintN i, line, temp;
                char filenameBuf[1024];

                if (peekChars(5, cp) &&
                    cp[0] == '@' &&
                    cp[1] == 'l' &&
                    cp[2] == 'i' &&
                    cp[3] == 'n' &&
                    cp[4] == 'e') {
                    skipChars(5);
                    while ((c = getChar()) != '\n' && JS_ISSPACE_OR_BOM((jschar)c))
                        continue;
                    if (JS7_ISDEC(c)) {
                        line = JS7_UNDEC(c);
                        while ((c = getChar()) != EOF && JS7_ISDEC(c)) {
                            temp = 10 * line + JS7_UNDEC(c);
                            if (temp < line) {
                                
                                goto skipline;
                            }
                            line = temp;
                        }
                        while (c != '\n' && JS_ISSPACE_OR_BOM((jschar)c))
                            c = getChar();
                        i = 0;
                        if (c == '"') {
                            while ((c = getChar()) != EOF && c != '"') {
                                if (c == '\n') {
                                    ungetChar(c);
                                    goto skipline;
                                }
                                if ((c >> 8) != 0 || i >= sizeof filenameBuf - 1)
                                    goto skipline;
                                filenameBuf[i++] = (char) c;
                            }
                            if (c == '"') {
                                while ((c = getChar()) != '\n' &&
                                       JS_ISSPACE_OR_BOM((jschar)c)) {
                                    continue;
                                }
                            }
                        }
                        filenameBuf[i] = '\0';
                        if (c == EOF || c == '\n') {
                            if (i > 0) {
                                if (flags & TSF_OWNFILENAME)
                                    cx->free_((void *) filename);
                                filename = JS_strdup(cx, filenameBuf);
                                if (!filename)
                                    goto error;
                                flags |= TSF_OWNFILENAME;
                            }
                            lineno = line;
                        }
                    }
                    ungetChar(c);
                }
            }

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
            uintN linenoBefore = lineno;
            while ((c = getChar()) != EOF &&
                   !(c == '*' && matchChar('/'))) {
                
            }
            if (c == EOF) {
                ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                         JSMSG_UNTERMINATED_COMMENT);
                goto error;
            }
            if (linenoBefore != lineno) {
                flags &= ~TSF_DIRTYLINE;
                flags |= TSF_EOL;
            }
            cursor = (cursor - 1) & ntokensMask;
            goto retry;
        }

        if (flags & TSF_OPERAND) {
            uintN reflags, length;
            JSBool inCharClass = JS_FALSE;

            tokenbuf.clear();
            for (;;) {
                c = getChar();
                if (c == '\\') {
                    if (!tokenbuf.append(c))
                        goto error;
                    c = getChar();
                } else if (c == '[') {
                    inCharClass = JS_TRUE;
                } else if (c == ']') {
                    inCharClass = JS_FALSE;
                } else if (c == '/' && !inCharClass) {
                    
                    break;
                }
                if (c == '\n' || c == EOF) {
                    ungetChar(c);
                    ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR,
                                             JSMSG_UNTERMINATED_REGEXP);
                    goto error;
                }
                if (!tokenbuf.append(c))
                    goto error;
            }
            for (reflags = 0, length = tokenbuf.length() + 1; ; length++) {
                c = peekChar();
                if (c == 'g' && !(reflags & JSREG_GLOB))
                    reflags |= JSREG_GLOB;
                else if (c == 'i' && !(reflags & JSREG_FOLD))
                    reflags |= JSREG_FOLD;
                else if (c == 'm' && !(reflags & JSREG_MULTILINE))
                    reflags |= JSREG_MULTILINE;
                else if (c == 'y' && !(reflags & JSREG_STICKY))
                    reflags |= JSREG_STICKY;
                else
                    break;
                getChar();
            }
            c = peekChar();
            if (JS7_ISLET(c)) {
                char buf[2] = { '\0' };
                tp->pos.begin.index += length + 1;
                buf[0] = (char)c;
                ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_BAD_REGEXP_FLAG,
                                         buf);
                (void) getChar();
                goto error;
            }
            tp->t_reflags = reflags;
            tt = TOK_REGEXP;
            break;
        }

        tp->t_op = JSOP_DIV;
        tt = matchChar('=') ? TOK_ASSIGN : TOK_DIVOP;
        break;

      case '%':
        tp->t_op = JSOP_MOD;
        tt = matchChar('=') ? TOK_ASSIGN : TOK_DIVOP;
        break;

      case '~':
        tp->t_op = JSOP_BITNOT;
        tt = TOK_UNARYOP;
        break;

      case '+':
        if (matchChar('=')) {
            tp->t_op = JSOP_ADD;
            tt = TOK_ASSIGN;
        } else if (matchChar(c)) {
            tt = TOK_INC;
        } else {
            tp->t_op = JSOP_POS;
            tt = TOK_PLUS;
        }
        break;

      case '-':
        if (matchChar('=')) {
            tp->t_op = JSOP_SUB;
            tt = TOK_ASSIGN;
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

#if JS_HAS_SHARP_VARS
      case '#':
      {
        uint32 n;

        c = getChar();
        if (!JS7_ISDEC(c)) {
            ungetChar(c);
            goto badchar;
        }
        n = (uint32)JS7_UNDEC(c);
        for (;;) {
            c = getChar();
            if (!JS7_ISDEC(c))
                break;
            n = 10 * n + JS7_UNDEC(c);
            if (n >= UINT16_LIMIT) {
                ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_SHARPVAR_TOO_BIG);
                goto error;
            }
        }
        tp->t_dval = (jsdouble) n;
        if (cx->hasStrictOption() &&
            (c == '=' || c == '#')) {
            char buf[20];
            JS_snprintf(buf, sizeof buf, "#%u%c", n, c);
            if (!ReportCompileErrorNumber(cx, this, NULL, JSREPORT_WARNING | JSREPORT_STRICT,
                                          JSMSG_DEPRECATED_USAGE, buf)) {
                goto error;
            }
        }
        if (c == '=')
            tt = TOK_DEFSHARP;
        else if (c == '#')
            tt = TOK_USESHARP;
        else
            goto badchar;
        break;
      }
#endif 

#if JS_HAS_SHARP_VARS || JS_HAS_XML_SUPPORT
      badchar:
#endif

      default:
        ReportCompileErrorNumber(cx, this, NULL, JSREPORT_ERROR, JSMSG_ILLEGAL_CHARACTER);
        goto error;
    }

  out:
    flags |= TSF_DIRTYLINE;
    tp->pos.end.index = userbuf.addressOfNextRawChar() - linebase;
    tp->type = tt;
    JS_ASSERT(IsTokenSane(tp));
    return tt;

  error:
    JS_ASSERT(cx->isExceptionPending());

    





    flags |= TSF_DIRTYLINE;
    tp->pos.end.index = tp->pos.begin.index + 1;
    tp->type = TOK_ERROR;
    JS_ASSERT(IsTokenSane(tp));
#ifdef DEBUG
    








    userbuf.poison();
#endif
    return TOK_ERROR;
}

