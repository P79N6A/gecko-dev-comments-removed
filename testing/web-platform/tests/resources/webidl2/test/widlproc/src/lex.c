












#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "lex.h"
#include "misc.h"
#include "node.h"
#include "process.h"

struct file {
    struct file *next;
    const char *filename;
    char *buf;
    const char *pos, *end;
    unsigned int linenum;
};

const char keywords[] = KEYWORDS;

static struct file *file, *firstfile;
static struct tok tok;






void
readinput(const char *const *argv)
{
    struct file **pfile = &file;
    for (;;) {
        struct file *file;
        const char *filename = *argv++;
        char *buf = 0;
        int len = 0, thislen, isstdin;
        FILE *handle;
        if (!filename)
            break;
        
        isstdin = !strcmp(filename, "-");
        if (isstdin) {
            handle = stdin;
            filename = "<stdin>";
        } else {
            handle = fopen(filename, "rb");
            if (!handle)
                errorexit("%s: %s", filename, strerror(errno));
        }
        for (;;) {
            thislen = len ? len * 2 : 4096;
            buf = memrealloc(buf, len + thislen + 1);
            thislen = fread(buf + len, 1, thislen, handle);
            if (!thislen)
                break;
            len += thislen;
        }
        if (ferror(handle))
            errorexit("%s: I/O error", filename);
        if (!isstdin)
            fclose(handle);
        buf[len] = 0;
        buf = memrealloc(buf, len + 1);
        
        file = memalloc(sizeof(struct file));
        *pfile = file;
        pfile = &file->next;
        file->filename = filename;
        file->pos = file->buf = buf;
        file->end = buf + len;
        file->linenum = 1;
    }
    *pfile = 0;
    firstfile = file;
}




static void
lexerrorexit(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vlocerrorexit(file->filename, file->linenum, format, ap);
    va_end(ap);
}








static struct tok *
lexblockcomment(const char *start)
{
    const char *p = start + 1;
    tok.filename = file->filename;
    tok.linenum = file->linenum;
    for (;;) {
        int ch = *++p;
        if (!ch)
            lexerrorexit("unterminated block comment");
        if (ch != '*') {
            if (ch == '\n')
                file->linenum++;
            continue;
        }
        ch = p[1];
        if (!ch)
            lexerrorexit("unterminated block comment");
        if (ch == '/')
            break;
    }
    p += 2;
    file->pos = p;
    tok.type = TOK_BLOCKCOMMENT;
    tok.start = start + 2;
    tok.len = p - start - 4;
    return &tok;
}








static struct tok *
lexinlinecomment(const char *start)
{
    const char *p = start + 2;
    p = start + 1;
    for (;;) {
        int ch = *++p;
        if (!ch || ch == '\n')
            break;
    }
    p++;
    file->pos = p;
    tok.type = TOK_INLINECOMMENT;
    tok.start = start + 2;
    tok.len = p - start - 2;
    tok.filename = file->filename;
    tok.linenum = file->linenum++;
    return &tok;
}











static struct tok *
lexnumber(const char *start)
{
    for (;;) {
        const char *p = start;
        const char *octalend = start;
        int ch = *p;
        enum { STATE_START, STATE_INT, STATE_HEX, STATE_OCTAL, STATE_BADOCTAL,
                STATE_DP, STATE_EXPSTART, STATE_EXPSIGN, STATE_EXP
                } state = STATE_START;
        if (ch == '-') {
            ch = *++p;
	    if (ch == 'I') { 
	      char * infinity = "-Infinity";
              unsigned int len = strlen(infinity);
	      if (!memcmp(start, infinity, len)) {
                tok.type = TOK_minusinfinity;
		tok.start = start;
		tok.len = len;
		tok.filename = file->filename;
		tok.linenum = file->linenum;
		file->pos = start + len;
		return &tok;
	      }
	    }
	}
        if (ch == '0') {
            state = STATE_OCTAL;
            ch = *++p;
            if ((ch & ~0x20) == 'X') {
                state = STATE_HEX;
                ch = *++p;
            }
        }

        for (;;) {
            if ((unsigned)(ch - '0') >= 8) {
                if ((ch & -2) == '8') {
                    if (state == STATE_OCTAL) {
                        state = STATE_BADOCTAL;
                        octalend = p;
                    }
                } else if ((unsigned)((ch & ~0x20) - 'A') <= 'F' - 'A') {
                    if (state != STATE_HEX) {
                        if ((ch & ~0x20) != 'E')
                            break;
                        if (state == STATE_HEX || state >= STATE_EXPSTART || state == STATE_START)
                            break;
                        state = STATE_EXPSTART;
                    }
                } else if (ch == '.') {
                    if (state == STATE_HEX || state >= STATE_DP)
                        break;
                    state = STATE_DP;
                } else if (ch == '-') {
                    if (state != STATE_EXPSTART)
                        break;
                    state = STATE_EXPSIGN;
                } else
                    break;
            }
            ch = *++p;
            if (state == STATE_START)
                state = STATE_INT;
            else if (state == STATE_EXPSTART || state == STATE_EXPSIGN)
                state = STATE_EXP;
        }
        switch (state) {
        case STATE_START:
            
            tok.type = '-';
            p = start + 1;
            break;
        case STATE_BADOCTAL:
            p = octalend;
            
        case STATE_INT:
        case STATE_OCTAL:
            tok.type = TOK_INTEGER;
            break;
        case STATE_HEX:
            if (p - start == 2 || (p - start == 3 && *start == '-'))
                p = start + 1;
            tok.type = TOK_INTEGER;
            break;
        case STATE_EXP:
        case STATE_DP:
            tok.type = TOK_FLOAT;
            break;
        case STATE_EXPSIGN:
            p--;
            
        case STATE_EXPSTART:
            p--;
            tok.type = TOK_FLOAT;
            break;
        }
        tok.start = start;
        tok.len = p - start;
        tok.filename = file->filename;
        tok.linenum = file->linenum;
        file->pos = p;
        return &tok;
    }
}








static struct tok *
lexstring(const char *start)
{
    for (;;) {
        const char *p = start + 1;
        int ch = *p;
        for (;;) {
            if (!ch || ch == '\n')
                lexerrorexit("unterminated string");
            if (ch == '"') {
                tok.type = TOK_STRING;
                tok.start = start + 1;
                tok.len = p - start - 1;
                tok.filename = file->filename;
                tok.linenum = file->linenum;
                file->pos = p + 1;
                return &tok;
            }
            

            ch = *++p;
        }
    }
}








static struct tok *
lexidentifier(const char *start)
{
    const char *p = start + 1;
    for (;;) {
        int ch = *p;
        if (ch != '_' && (unsigned)(ch - '0') >= 10
                && (unsigned)((ch & ~0x20) - 'A') > 'Z' - 'A')
        {
            break;
        }
        p++;
    }
    tok.type = TOK_IDENTIFIER;
    tok.start = start;
    tok.len = p - start;
    tok.filename = file->filename;
    tok.linenum = file->linenum;
    file->pos = p;
    
    {
        unsigned int type = TOK_DOMString;
        p = keywords;
        for (;;) {
            unsigned int len = strlen(p);
            if (!len)
                break;
            if (len == tok.len && !memcmp(start, p, len)) {
                tok.type = type;
                break;
            }
            p += len + 1;
            type++;
        }
    }
    return &tok;
}






struct tok *
lex(void)
{
    const char *p;
    int ch;
    for (;;) {
        if (!file) {
            tok.type = TOK_EOF;
            tok.start = "end of file";
            tok.len = strlen(tok.start);
            return &tok;
        }
        tok.prestart = p = file->pos;
        
        for (;;) {
            ch = *p++;
            switch (ch) {
            case ' ':
            case '\t':
            case '\r':
                continue;
            case '\n':
                ++file->linenum;
                tok.prestart = p;
                continue;
            }
            break;
        }
        p--;
        if (ch)
            break;
        if (p != file->end)
            lexerrorexit("\\0 byte not allowed");
        file = file->next;
    }
    
    tok.start = p;
    if (ch == '/') {
        switch (*++p) {
        case '*':
            return lexblockcomment(p - 1);
        case '/':
            return lexinlinecomment(p - 1);
        }
        tok.type = '/';
    } else {
        

        if (ch == '-' || (unsigned)(ch - '0') < 10)
            return lexnumber(p);
        
        if (ch == '"')
            return lexstring(p);
        
        if (ch == '_' || (unsigned)((ch & ~0x20) - 'A') <= 'Z' - 'A')
            return lexidentifier(p);
        
        if (ch == '.') {
            tok.type = '.';
            if (*++p == '.' && p[1] == '.') {
                tok.type = TOK_ELLIPSIS;
                p += 2;
            }
            goto done;
        }
        if (ch == '[') {
            tok.type = '[';
            if (*++p == ']') {
                tok.type = TOK_DOUBLEBRACKET;
                p++;
            }
            goto done;
        }
    }
    
    tok.type = ch;
    p++;
done:
    tok.filename = file->filename;
    tok.linenum = file->linenum;
    tok.len = p - tok.start;
    file->pos = p;
    return &tok;
}






void
outputwidl(struct node *node)
{
    const char *start = node->wsstart, *end = node->end;
    
    struct file *file = firstfile;
    while (start < file->buf || start >= file->end) {
        file = file->next;
        assert(file);
    }
    

    while (node && !node->start)
        node = nodewalk(node);
    

    for (;;) {
        int final = end >= file->buf && end <= file->end;
        const char *thisend = final ? end : file->end;
        
        while (start != end) {
            const char *p, *p2, *comment, *endcomment;
            int ch;
            if (node && start == node->start) {
                

                fputs("<ref>", stdout);
                printtext(node->start, node->end - node->start, 1);
                fputs("</ref>", stdout);
                start = node->end;
                
                do
                    node = nodewalk(node);
                while (node && !node->start);
                continue;
            }
            p2 = thisend;
            if (node && node->start >= file->buf && node->start < p2)
                p2 = node->start;
            p = memchr(start, '/', p2 - start);
            if (!p) {
                printtext(start, p2 - start, 1);
                if (p2 != thisend) {
                    start = p2;
                    continue;
                }
                break;
            }
            
            comment = 0;
            if (p + 1 != thisend) {
                switch (p[1]) {
                case '*':
                    
                    comment = p;
                    p++;
                    do 
                        p = memchr(p + 1, '*', thisend - p - 1);
                    while (p[1] != '/');
                    endcomment = p + 2;
                    break;
                case '/':
                    
                    comment = p;
                    p = memchr(p, '\n', thisend - p);
                    if (!p)
                        p = thisend;
                    endcomment = p;
                    break;
                }
            }
            if (!comment) {
                
                p++;
                printtext(start, p - start, 1);
                start = p;
                assert(start <= end);
                continue;
            }
            

            p = comment;
            while (p != start && ((ch = p[-1]) == ' ' || ch == '\t'))
                p--;
            if (p == start || p[-1] == '\n') {
                comment = p;
                


                p = endcomment;
                while (p != thisend && ((ch = *p) == ' ' || ch == '\t'))
                    p++;
                if (p != thisend && *p == '\n')
                    p++;
                endcomment = p;
            }
            printtext(start, comment - start, 1);
            start = endcomment;
            if (start > thisend)
                start = thisend;
        }
        if (final)
            break;
        file = file->next;
        assert(file);
        start = file->buf;
    }
}
