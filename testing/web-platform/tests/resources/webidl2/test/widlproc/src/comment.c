












#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "comment.h"
#include "entities.h"
#include "lex.h"
#include "misc.h"
#include "node.h"
#include "os.h"
#include "process.h"


struct cnode {
    struct cnode *next;
    struct cnode *children;
    struct cnode *parent;
    const struct cnodefuncs *funcs;
    const char *attrtext;
    const char *filename;
    unsigned int linenum;
};

struct cnodefuncs {
    int indesc; 

    int needpara; 

    int (*askend)(struct cnode *cnode, const struct cnodefuncs *type);
    void (*end)(struct cnode *cnode);
    void (*output)(struct cnode *cnode, unsigned int indent);
};

struct paramcnode {
    struct cnode cn;
    int inout;
    char name[1];
};


struct comment {
    struct comment *next;
    struct node *node;
    unsigned int type;
    const char *filename;
    unsigned int linenum;
    struct cnode root;
    int back; 
    char *text;
};


static struct node *lastidentifier;
static struct comment *comments;
static int incode, inhtmlblock;
static struct comment *curcomment;




#define HTMLEL_EMPTY 1
#define HTMLEL_INLINE 2
#define HTMLEL_BLOCK 4
#define HTMLEL_AUTOCLOSE 8
#define HTMLEL_LI 0x10
#define HTMLEL_DLCONTENTS 0x20
#define HTMLEL_TABLECONTENTS 0x40
#define HTMLEL_TRCONTENTS 0x80

#define HTMLEL_FLOW (HTMLEL_BLOCK | HTMLEL_INLINE)

struct htmleldesc {
    unsigned int namelen;
    const char *name;
    unsigned int flags;
    unsigned int content;
};

static const struct htmleldesc htmleldescs[] = {
    { 1, "a", HTMLEL_INLINE, 0 },
    { 1, "b", HTMLEL_INLINE, 0 },
    { 2, "br", HTMLEL_INLINE, HTMLEL_EMPTY },
    { 3, "img", HTMLEL_INLINE, HTMLEL_EMPTY },
    { 2, "dd", HTMLEL_DLCONTENTS, HTMLEL_FLOW },
    { 2, "dl", HTMLEL_BLOCK, HTMLEL_DLCONTENTS },
    { 2, "dt", HTMLEL_DLCONTENTS, HTMLEL_INLINE },
    { 2, "em", HTMLEL_INLINE, 0 },
    { 2, "li", HTMLEL_LI, HTMLEL_FLOW },
    { 2, "ol", HTMLEL_BLOCK, HTMLEL_LI },
    { 1, "p", HTMLEL_BLOCK, HTMLEL_INLINE },
    { 2, "td", HTMLEL_TRCONTENTS | HTMLEL_AUTOCLOSE, HTMLEL_FLOW },
    { 2, "th", HTMLEL_TRCONTENTS | HTMLEL_AUTOCLOSE, HTMLEL_FLOW },
    { 2, "tr", HTMLEL_TABLECONTENTS | HTMLEL_AUTOCLOSE, HTMLEL_TRCONTENTS },
    { 5, "table", HTMLEL_BLOCK, HTMLEL_TABLECONTENTS },
    { 2, "ul", HTMLEL_BLOCK, HTMLEL_LI },
    { 0, 0, 0, 0 }
};
#define HTMLELDESC_B (htmleldescs + 1)
#define HTMLELDESC_BR (htmleldescs + 2)






void
addcomment(struct tok *tok)
{
    if (tok->len >= 1 && (tok->start[0] == '!'
        || (tok->type == TOK_BLOCKCOMMENT && tok->start[0] == '*')
        || (tok->type == TOK_INLINECOMMENT && tok->start[0] == '/')))
    {
        struct comment *comment;
        comment = memalloc(sizeof(struct comment));
        comment->text = memalloc(tok->len + 1);
        memcpy(comment->text, tok->start, tok->len);
        comment->text[tok->len] = 0;
        comment->type = tok->type;
        comment->filename = tok->filename;
        comment->linenum = tok->linenum;
        comment->node = 0;
        comment->back = 0;
        if (comment->text[1] == '<') {
            comment->back = 1;
            if (!lastidentifier) {
                locerrorexit(comment->filename, comment->linenum,
                    "no identifier to attach doxygen comment to");
            }
            comment->node = lastidentifier;
        }
        comment->next = comments;
        comments = comment;
    }
}






void
setcommentnode(struct node *node2)
{
    struct comment *comment = comments;
    while (comment && !comment->node) {
        comment->node = node2;
        comment = comment->next;
    }
    lastidentifier = node2;
}











static struct comment *
joininlinecomments(struct comment *comments)
{
    struct comment **pcomment;
    pcomment = &comments;
    for (;;) {
        struct comment *comment;
        comment = *pcomment;
        if (!comment)
            break;
        if (comment->type != TOK_INLINECOMMENT) {
            
            pcomment = &comment->next;
        } else if (!comment->back && (!comment->next
                || comment->next->type != TOK_INLINECOMMENT
                || comment->next->filename != comment->filename
                || comment->next->linenum != comment->linenum + 1))
        {
            
            *pcomment = comment->next;

        } else {
            




            struct comment *newcomment = 0, *comment2;
            const char *filename = comment->filename;
            unsigned int linenum = comment->linenum;
            for (;;) {
                char *wp = newcomment->text;
                comment2 = comment;
                do {
                    unsigned int len = strlen(comment2->text);
                    if (newcomment)
                        memcpy(wp, comment2->text, len);
                    wp += len;
                    linenum--;
                    comment2 = comment2->next;
                } while (comment2 && comment2->filename == filename
                            && comment2->linenum == linenum
                            && comment2->node == comment->node);
                
                if (newcomment) {
                    *wp = 0;
                    break;
                }
                newcomment = memalloc(sizeof(struct comment)
                                + wp - newcomment->text);
                newcomment->node = comment->node;
                newcomment->type = comment->type;
                newcomment->filename = filename;
                newcomment->linenum = linenum + 1;
            }
            

            newcomment->next = comment2;
            *pcomment = newcomment;
            pcomment = &newcomment->next;
        }
    }
    return comments;
}









static void
outputchildren(struct cnode *cnode, unsigned int indent, int indesc)
{
    int curindesc = indesc;
    cnode = cnode->children;
    while (cnode) {
        if (curindesc != cnode->funcs->indesc) {
            assert(!indesc);
            printf("%*s<%sdescription>\n", indent + 1, "", curindesc ? "/" : "");
            curindesc = !curindesc;
        }
        (*cnode->funcs->output)(cnode, indent + 2);
        cnode = cnode->next;
    }
    if (curindesc != indesc)
        printf("%*s<%sdescription>\n", indent + 1, "", curindesc ? "/" : "");
}










static int
default_askend(struct cnode *cnode, const struct cnodefuncs *type)
{
    return 1;
}









static int
root_askend(struct cnode *cnode, const struct cnodefuncs *type)
{
    return 0;
}







static void
root_output(struct cnode *cnode, unsigned int indent)
{
    outputchildren(cnode, indent, 0);
}




static const struct cnodefuncs root_funcs = {
    0, 
    1, 
    &root_askend,
    0, 
    &root_output,
};








static struct cnode *
endcnode(struct cnode *cnode)
{
    if (cnode->funcs->end)
        (*cnode->funcs->end)(cnode);
    
    {
        struct cnode *child = cnode->children;
        cnode->children = 0;
        while (child) {
            struct cnode *next = child->next;
            child->next = cnode->children;
            cnode->children = child;
            child = next;
        }
    }
    return cnode->parent;
}










static struct cnode *
endspecificcnode(struct cnode *cnode, const struct cnodefuncs *type,
                 const char *filename, unsigned int linenum)
{
    while (cnode->funcs != type) {
        if (cnode->funcs == &root_funcs)
            locerrorexit(filename, linenum, "unmatched \\endcode");
        cnode = endcnode(cnode);
    }
    return cnode;
}









static struct cnode *
startcnode(struct cnode *cnode, struct cnode *newcnode)
{
    newcnode->parent = cnode;
    newcnode->next = cnode->children;
    cnode->children = newcnode;
    return newcnode;
}







static void
para_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<p>\n", indent, "");
    outputchildren(cnode, indent, 1);
    printf("%*s</p>\n", indent, "");
}






static void
para_end(struct cnode *cnode)
{
    
    if (!cnode->children)
        cnode->parent->children = cnode->next;
}




static const struct cnodefuncs para_funcs = {
    1, 
    0, 
    &default_askend,
    &para_end, 
    &para_output,
};







static void
brief_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<brief>\n", indent, "");
    outputchildren(cnode, indent, 1);
    printf("%*s</brief>\n", indent, "");
}




static const struct cnodefuncs brief_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &brief_output,
};







static void
return_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<description><p>\n", indent, "");
    outputchildren(cnode, indent, 1);
    printf("%*s</p></description>\n", indent, "");
}




static const struct cnodefuncs return_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &return_output,
};







static void
name_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<name>\n", indent, "");
    outputchildren(cnode, indent, 1);
    printf("%*s</name>\n", indent, "");
}




static const struct cnodefuncs name_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &name_output,
};







static void
author_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<author>\n", indent, "");
    outputchildren(cnode, indent, 1);
    printf("%*s</author>\n", indent, "");
}




static const struct cnodefuncs author_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &author_output,
};







static void
version_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<version>\n", indent, "");
    outputchildren(cnode, indent, 1);
    printf("%*s</version>\n", indent, "");
}




static const struct cnodefuncs version_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &version_output,
};









static void
code_end(struct cnode *cnode)
{
    if (incode) {
        

        locerrorexit(cnode->filename, cnode->linenum, "mismatched \\code");
    }
}







static void
code_output(struct cnode *cnode, unsigned int indent)
{
    
    if(cnode->attrtext)
	    printf("%*s<Code %s>", indent, "", cnode->attrtext);
	else
	    printf("%*s<Code>", indent, "");
    outputchildren(cnode, indent, 1);
    printf("</Code>\n");
}

static const struct cnodefuncs code_funcs = {
    0, 
    0, 
    &default_askend,
    &code_end, 
    &code_output,
};









static struct cnode *
startpara(struct cnode *cnode, const struct cnodefuncs *type)
{
    struct cnode *newcnode;
    while ((*cnode->funcs->askend)(cnode, type))
        cnode = endcnode(cnode);
    newcnode = memalloc(sizeof(struct cnode));
    newcnode->funcs = type;
    return startcnode(cnode, newcnode);
}




struct textcnode {
    struct cnode cn;
    unsigned char *data;
    unsigned int len;
    unsigned int max;
};






static void
text_end(struct cnode *cnode)
{
    struct textcnode *textcnode = (void *)cnode;
    textcnode->data[textcnode->len] = 0;
    textcnode->max = textcnode->len + 1;
    textcnode->data = memrealloc(textcnode->data, textcnode->max);
}







static void
text_output(struct cnode *cnode, unsigned int indent)
{
    
    struct textcnode *textcnode = (void *)cnode;
    unsigned int len = textcnode->len;
    unsigned const char *p = textcnode->data;
    while (len) {
        unsigned int thislen;
        const char *thisptr;
        thislen = p[0];
        

        memcpy((void *)&thisptr, p + 1, sizeof(void *));
        p += 1 + sizeof(void *);
        len -= 1 + sizeof(void *);
        printtext(thisptr, thislen, 0);
    }
}

static const struct cnodefuncs text_funcs = {
    1, 
    0, 
    &default_askend,
    &text_end, 
    &text_output,
};




struct htmlcnode {
    struct cnode cn;
    const struct htmleldesc *desc;
    char attrs[1];
};






static void
html_end(struct cnode *cnode)
{
    if (((struct htmlcnode *)cnode)->desc->flags & HTMLEL_BLOCK)
        inhtmlblock--;
}







static void
html_output(struct cnode *cnode, unsigned int indent)
{
    struct htmlcnode *htmlcnode = (void *)cnode;
    if (!(htmlcnode->desc->flags & HTMLEL_INLINE))
        printf("%*s", indent, "");
    if (htmlcnode->cn.children) {
        printf("<%s%s>", htmlcnode->desc->name, htmlcnode->attrs);
        if (!(htmlcnode->desc->flags & HTMLEL_INLINE))
            putchar('\n');
        outputchildren(&htmlcnode->cn, indent, 1);
        if (!(htmlcnode->desc->flags & HTMLEL_INLINE))
            printf("%*s", indent, "");
        printf("</%s>", htmlcnode->desc->name);
    } else
        printf("<%s%s/>", htmlcnode->desc->name, htmlcnode->attrs);
    if (!(htmlcnode->desc->flags & HTMLEL_INLINE))
        putchar('\n');
}

static const struct cnodefuncs html_funcs = {
    1, 
    0, 
    &default_askend,
    &html_end, 
    &html_output,
};













static struct cnode *
starthtmlcnode(struct cnode *cnode, const struct htmleldesc *htmleldesc,
               const char *attrs, unsigned int attrslen,
               const char *filename, unsigned int linenum)
{
    struct htmlcnode *htmlcnode;
    

    for (;;) {
        if (cnode->funcs != &html_funcs) {
            



            if (!(htmleldesc->flags & HTMLEL_INLINE)) {
                if (!(htmleldesc->flags & HTMLEL_BLOCK))
                    locerrorexit(filename, linenum, "<%s> not valid here", htmleldesc->name);
                while ((*cnode->funcs->askend)(cnode, 0))
                    cnode = endcnode(cnode);
            } else {
                while (cnode->funcs == &text_funcs)
                    cnode = endcnode(cnode);
            }
            break;
        }
        htmlcnode = (struct htmlcnode *)cnode;
        if (!(htmleldesc->flags & htmlcnode->desc->content))
            locerrorexit(filename, linenum, "<%s> not valid here", htmleldesc->name);
        break;
    }
    if (htmleldesc->flags & HTMLEL_BLOCK)
        inhtmlblock++;
    
    htmlcnode = memalloc(sizeof(struct htmlcnode) + attrslen);
    htmlcnode->desc = htmleldesc;
    htmlcnode->cn.funcs = &html_funcs;
    htmlcnode->cn.filename = filename;
    htmlcnode->cn.linenum = linenum;
    memcpy(htmlcnode->attrs, attrs, attrslen);
    htmlcnode->attrs[attrslen] = 0;
    
    cnode = startcnode(cnode, &htmlcnode->cn);
    return cnode;
}











static void
param_output(struct cnode *cnode, unsigned int indent)
{
    struct paramcnode *paramcnode = (void *)cnode;
    printf("%*s<param identifier=\"%s\">\n", indent, "", paramcnode->name);
    outputchildren(&paramcnode->cn, indent, 1);
    printf("%*s</param>\n", indent, "");
}




static const struct cnodefuncs param_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &param_output,
};




static const struct cnodefuncs throw_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &return_output,
};












static struct cnode *
startparamcnode(struct cnode *cnode, const char *word, unsigned int wordlen,
                int inout, const struct cnodefuncs *funcs)
{
    struct paramcnode *paramcnode;
    paramcnode = memalloc(sizeof(struct paramcnode) + wordlen);
    paramcnode->cn.funcs = funcs;
    memcpy(paramcnode->name, word, wordlen);
    paramcnode->name[wordlen] = 0;
    paramcnode->inout = inout;
    return startcnode(cnode, &paramcnode->cn);
}







static void
api_feature_output(struct cnode *cnode, unsigned int indent)
{
    struct paramcnode *paramcnode = (void *)cnode;
    printf("%*s<api-feature identifier=\"%s\">\n", indent, "", paramcnode->name);
    outputchildren(cnode, indent, 1);
    printf("%*s</api-feature>\n", indent, "");
}




static const struct cnodefuncs api_feature_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &api_feature_output,
};







static void
device_cap_output(struct cnode *cnode, unsigned int indent)
{
    struct paramcnode *paramcnode = (void *)cnode;
    printf("%*s<device-cap identifier=\"%s\">\n", indent, "", paramcnode->name);
    outputchildren(cnode, indent, 1);
    printf("%*s</device-cap>\n", indent, "");
}




static const struct cnodefuncs device_cap_funcs = {
    0, 
    0, 
    &default_askend,
    0, 
    &device_cap_output,
};









static int
def_api_feature_askend(struct cnode *cnode, const struct cnodefuncs *type)
{
    

    if (!type || type == &para_funcs || type == &device_cap_funcs || type == &brief_funcs)
        return 0;
    return 1;
}







static void
def_api_feature_output(struct cnode *cnode, unsigned int indent)
{
    struct paramcnode *paramcnode = (void *)cnode;
    printf("%*s<def-api-feature identifier=\"%s\">\n", indent, "", paramcnode->name);
    printf("%*s<descriptive>\n", indent + 2, "");
    outputchildren(cnode, indent + 2, 0);
    printf("%*s</descriptive>\n", indent + 2, "");
    printf("%*s</def-api-feature>\n", indent, "");
}




static const struct cnodefuncs def_api_feature_funcs = {
    0, 
    1, 
    &def_api_feature_askend,
    0, 
    &def_api_feature_output,
};









static int
def_api_feature_set_askend(struct cnode *cnode, const struct cnodefuncs *type)
{
    

    if (!type || type == &para_funcs || type == &api_feature_funcs || type == &brief_funcs)
        return 0;
    return 1;
}







static void
def_api_feature_set_output(struct cnode *cnode, unsigned int indent)
{
    struct paramcnode *paramcnode = (void *)cnode;
    printf("%*s<def-api-feature-set identifier=\"%s\">\n", indent, "", paramcnode->name);
    printf("%*s<descriptive>\n", indent + 2, "");
    outputchildren(cnode, indent + 2, 0);
    printf("%*s</descriptive>\n", indent + 2, "");
    printf("%*s</def-api-feature-set>\n", indent, "");
}




static const struct cnodefuncs def_api_feature_set_funcs = {
    0, 
    1, 
    &def_api_feature_set_askend,
    0, 
    &def_api_feature_set_output,
};









static int
def_instantiated_askend(struct cnode *cnode, const struct cnodefuncs *type)
{
    

    if (!type || type == &para_funcs || type == &api_feature_funcs || type == &brief_funcs)
        return 0;
    return 1;
}







static void
def_instantiated_output(struct cnode *cnode, unsigned int indent)
{
    printf("%*s<def-instantiated>\n", indent, "");
    printf("%*s<descriptive>\n", indent + 2, "");
    outputchildren(cnode, indent + 2, 0);
    printf("%*s</descriptive>\n", indent + 2, "");
    printf("%*s</def-instantiated>\n", indent, "");
}




static const struct cnodefuncs def_instantiated_funcs = {
    0, 
    1, 
    &def_instantiated_askend,
    0, 
    &def_instantiated_output,
};









static int
def_device_cap_askend(struct cnode *cnode, const struct cnodefuncs *type)
{
    

    if (!type || type == &para_funcs || type == &param_funcs || type == &brief_funcs)
        return 0;
    return 1;
}







static void
def_device_cap_output(struct cnode *cnode, unsigned int indent)
{
    struct paramcnode *paramcnode = (void *)cnode;
    printf("%*s<def-device-cap identifier=\"%s\">\n", indent, "", paramcnode->name);
    printf("%*s<descriptive>\n", indent + 2, "");
    outputchildren(cnode, indent + 2, 0);
    printf("%*s</descriptive>\n", indent + 2, "");
    printf("%*s</def-device-cap>\n", indent, "");
}




static const struct cnodefuncs def_device_cap_funcs = {
    0, 
    1, 
    &def_device_cap_askend,
    0, 
    &def_device_cap_output,
};










static struct cnode *
addtext(struct cnode *cnode, const char *text, unsigned int len)
{
    struct textcnode *textcnode;
    if (!len)
        return cnode;
    if (cnode->funcs != &text_funcs) {
        
        textcnode = memalloc(sizeof(struct textcnode));
        textcnode->cn.funcs = &text_funcs;
        cnode = startcnode(cnode, &textcnode->cn);
    }
    textcnode = (void *)cnode;
    do {
        unsigned char buf[1 + sizeof(void *)];
        unsigned int thislen = len;
        if (thislen > 255)
            thislen = 255;
        
        buf[0] = thislen;
        memcpy(buf + 1, &text, sizeof(void *));
        
        if (textcnode->len + sizeof(buf) >= textcnode->max) {
            
            textcnode->max = textcnode->max ? 2 * textcnode->max : 1024;
            textcnode->data = memrealloc(textcnode->data, textcnode->max);
        }
        memcpy(textcnode->data + textcnode->len, buf, sizeof(buf));
        textcnode->len += sizeof(buf);
        text += thislen;
        len -= thislen;
    } while (len);
    return &textcnode->cn;
}








static inline int
iswhitespace(int ch)
{
    unsigned int i = ch - 1;
    if (i >= 32)
        return 0;
    return 0x80001100 >> i & 1;
}








static const char *
parseword(const char **pp)
{
    const char *p = *pp, *word = 0;
    int ch = *p;
    while (iswhitespace(ch))
        ch = *++p;
    word = p;
    while ((unsigned)((ch & ~0x20) - 'A') <= 'Z' - 'A'
            || (unsigned)(ch - '0') < 10 || ch == '_' || ch == '.'
            || ch == ':' || ch == '/' || ch == '-')
    {
        ch = *++p;
    }
    if (p == word)
        return 0;
    *pp = p;
    return word;
}

















static const char *
dox_b(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
      const char *filename, unsigned int linenum, const char *cmdname)
{
    struct cnode *cnode = *pcnode;
    const char *word = parseword(&p);
    
    if (word) {
        struct cnode *mycnode;
        mycnode = cnode = starthtmlcnode(cnode, HTMLELDESC_B, 0, 0, filename, linenum);
        cnode = addtext(cnode, word, p - word);
        while (cnode != mycnode)
            cnode = endcnode(cnode);
        cnode = endcnode(cnode);
    }
    *pcnode = cnode;
    return p;
}




static const char *
dox_n(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
      const char *filename, unsigned int linenum, const char *cmdname)
{
    struct cnode *cnode = *pcnode;
    cnode = starthtmlcnode(cnode, HTMLELDESC_BR, 0, 0, filename, linenum);
    cnode = endcnode(cnode);
    *pcnode = cnode;
    return p;
}




static const char *
dox_code(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
         const char *filename, unsigned int linenum, const char *cmdname)
{
    *pcnode = startpara(*pcnode, &code_funcs);
    (*pcnode)->filename = filename;
    (*pcnode)->linenum = linenum; 
    incode = 1;
    return p;
}




static const char *
dox_endcode(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
            const char *filename, unsigned int linenum, const char *cmdname)
{
    incode = 0;
    *pcnode = endspecificcnode(*pcnode, &code_funcs, filename, linenum);
    return p;
}




static const char *
dox_param(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
          const char *filename, unsigned int linenum, const char *cmdname)
{
    struct cnode *cnode = *pcnode;
    unsigned int inout = 0;
    const char *word;
    
    if (*p == '[') {
        for (;;) {
            p++;
            if (!memcmp(p, "in", 2)) {
                inout |= 1;
                p += 2;
            } else if (!memcmp(p, "out", 3)) {
                inout |= 2;
                p += 3;
            } else
                break;
            if (*p != ',')
                break;
        }
        if (*p != ']')
            locerrorexit(filename, linenum, "bad attributes on \\param");
        p++;
    }
    
    word = parseword(&p);
    if (!word)
        locerrorexit(filename, linenum, "expected word after \\param");
    
    while ((*cnode->funcs->askend)(cnode, type))
        cnode = endcnode(cnode);
    
    cnode = startparamcnode(cnode, word, p - word, inout, type);
    cnode->filename = filename;
    cnode->linenum = linenum;
    *pcnode = cnode;
    return p;
}




static const char *
dox_para(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
         const char *filename, unsigned int linenum, const char *cmdname)
{
    *pcnode = startpara(*pcnode, type);
    return p;
}




static const char *
dox_throw(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
          const char *filename, unsigned int linenum, const char *cmdname)
{
    struct cnode *cnode = *pcnode;
    const char *word;
    
    word = parseword(&p);
    if (!word)
        locerrorexit(filename, linenum, "expected word after \\throw");
    
    while ((*cnode->funcs->askend)(cnode, type))
        cnode = endcnode(cnode);
    
    cnode = startparamcnode(cnode, word, p - word, 0, type);
    cnode->filename = filename;
    cnode->linenum = linenum;
    *pcnode = cnode;
    return p;
}




static const char *
dox_attr(const char *p, struct cnode **pcnode, const struct cnodefuncs *type,
          const char *filename, unsigned int linenum, const char *cmdname)
{
  struct cnode *cnode = *pcnode;
    const char *word;
    int len, wordlen, offset = 0;
	char *attrtext;
    
    word = parseword(&p);
    if (!word)
        locerrorexit(filename, linenum, "expected word after \\%s", cmdname);

	len = strlen(cmdname) + (wordlen = p-word) + 4; 
	if(cnode->attrtext)
	  len += (offset = strlen(cnode->attrtext)) + 1; 
	attrtext = memalloc(len);
	if(offset) {
		memcpy(attrtext, cnode->attrtext, offset);
		attrtext[offset++] = ' ';
		memfree(((void*)cnode->attrtext));
	}
	offset += sprintf(&attrtext[offset], "%s=\"", cmdname);
	memcpy(&attrtext[offset], word, wordlen);
	strcpy(&attrtext[offset + wordlen], "\"");
	cnode->attrtext = attrtext;
	
	if(incode && iswhitespace(*p)) ++p;
    return p;
}




struct command {
    const char *(*func)(const char *p, struct cnode **pcnode, const struct cnodefuncs *type, const char *filename, unsigned int linenum, const char *cmdname);
    const struct cnodefuncs *type;
    unsigned int namelen;
    const char *name;
};
static const struct command commands[] = {
    { &dox_throw, &def_api_feature_funcs, 15, "def-api-feature" },
    { &dox_throw, &def_api_feature_set_funcs, 19, "def-api-feature-set" },
    { &dox_para, &def_instantiated_funcs, 16, "def-instantiated" },
    { &dox_para, &name_funcs, 4, "name" },
    { &dox_para, &author_funcs, 6, "author" },
    { &dox_b, 0, 1, "b" },
    { &dox_para, &brief_funcs, 5, "brief" },
    { &dox_code, 0, 4, "code" },
    { &dox_throw, &def_device_cap_funcs, 14, "def-device-cap" },
    { &dox_attr, 0, 4, "lang" },
    { &dox_endcode, 0, 7, "endcode" },
    { &dox_n, 0, 1, "n" },
    { &dox_param, &param_funcs, 5, "param" },
    { &dox_para, &return_funcs, 6, "return" },
    { &dox_throw, &throw_funcs, 5, "throw" },
    { &dox_throw, &api_feature_funcs, 11, "api-feature" },
    { &dox_throw, &device_cap_funcs, 10, "device-cap" },
    { &dox_para, &version_funcs, 7, "version" },
    { 0, 0, 0 }
};












static const char *
parsehtmltag(const char *start, struct cnode **pcnode,
             const char *filename, unsigned int *plinenum)
{
    struct cnode *cnode = *pcnode;
    const char *end = start + 1, *endname = 0, *name = end;
    int ch = *end;
    int quote = 0;
    int close = 0;
    unsigned int linenum = *plinenum;
    const struct htmleldesc *htmleldesc;
    if (ch == '/') {
        close = 1;
        ch = *++end;
        name = end;
    }
    
    for (;;) {
        if (!ch)
            locerrorexit(filename, *plinenum, "unterminated HTML tag");
        if (ch == '\n')
            linenum++;
        else if (iswhitespace(ch) || ch == '/') {
            if (!endname)
                endname = end;
        } else if (!quote) {
            if (ch == '"' || ch == '\'')
                quote = ch;
            else if (ch == '>')
                break;
        } else {
            if (ch == quote)
                quote = 0;
        }
        ch = *++end;
    }
    if (!endname)
        endname = end;
    end++;
    
    if (!close && endname != name && end[-2] == '/')
        close = 2;
    
    htmleldesc = htmleldescs;
    for (;;) {
        if (!htmleldesc->namelen) {
            locerrorexit(filename, *plinenum, "unrecognized HTML tag %.*s",
                    end - start, start);
        }
        if (htmleldesc->namelen == endname - name
                && !strncasecmp(htmleldesc->name, name, endname - name))
        {
            break;
        }
        htmleldesc++;
    }
    if (close == 1) {
        
        for (;;) {
            struct htmlcnode *htmlcnode;
            if (cnode->funcs != &text_funcs) {
                if (cnode->funcs != &html_funcs) {
                    locerrorexit(filename, *plinenum, "mismatched %.*s",
                            end - start, start);
                }
                htmlcnode = (struct htmlcnode *)cnode;
                if (htmlcnode->desc == htmleldesc)
                    break;
                if (!(htmlcnode->desc->flags & HTMLEL_AUTOCLOSE)) {
                    locerrorexit(filename, htmlcnode->cn.linenum,
                            "mismatched <%.*s>",
                            htmlcnode->desc->namelen, htmlcnode->desc->name);
                }
            }
            cnode = endcnode(cnode);
        }
        cnode = endcnode(cnode);
    } else {
        
      if (close !=2)
	   cnode = starthtmlcnode(cnode, htmleldesc, endname, end - 1 - endname, filename, *plinenum);
      else 
	   cnode = starthtmlcnode(cnode, htmleldesc, endname, end - 2 - endname, filename, *plinenum);
      if (close == 2 || (htmleldesc->content & HTMLEL_EMPTY)) {
	
	cnode = endcnode(cnode);
      }
    }
    *pcnode = cnode;
    *plinenum = linenum;
    return end;
}






static void
parsecomment(struct comment *comment)
{
    struct cnode *cnode = &comment->root;
    const char *p = comment->text + comment->back;
    unsigned int linenum = comment->linenum - 1;
    int ch;
    curcomment = comment;
    incode = 0;
    inhtmlblock = 0;
    cnode->funcs = &root_funcs;
    for (;;) {
        
        const char *starttext;
        ch = *p;
        linenum++;
        {
            
            const char *p2 = p;
            int ch2 = ch;
            while (iswhitespace(ch2))
                ch2 = *++p2;
            if (comment->type == TOK_BLOCKCOMMENT && ch2 == '*') {
                
                ch2 = *++p2;
                ch = ch2;
                p = p2;
                if (ch == '*')
                    goto checkforlineofstars;
                while (iswhitespace(ch2))
                    ch2 = *++p2;
            }
            if (comment->type == TOK_INLINECOMMENT && ch2 == '/') {
checkforlineofstars:
                if (!incode) {
                    

                    const char *p3 = p2;
                    int ch3;
                    do ch3 = *++p3; while (ch3 == ch2);
                    while (iswhitespace(ch3)) ch3 = *++p3;
                    if (!ch3 || ch3 == '\n') {
                        

                        ch2 = ch3;
                        p2 = p3;
                    }
                }
            }
            if (!incode) {
                

                ch = ch2;
                p = p2;
            }
        }
        if (!ch) {
            
            break;
        }
        if (!incode && !inhtmlblock && ch == '\n') {
            

            while ((*cnode->funcs->askend)(cnode, 0))
                cnode = endcnode(cnode);
            p++;
            continue;
        }
        
        if (cnode->funcs->needpara)
            cnode = startpara(cnode, &para_funcs);
        
        starttext = p;
        while (ch && ch != '\n') {
            if (ch != '\\' && ch != '<'  && ch != '$'
                    && ch != '&' && ch != '\r')
            {
                ch = *++p;
                continue;
            }
            
            if (p - starttext)
                cnode = addtext(cnode, starttext, p - starttext);
	    
	    if (ch == '\r') {
	        ch = *++p;
  	        starttext = p;
		continue;
	    }
            if (ch == '$')
                locerrorexit(comment->filename, linenum, "use \\$ instead of $");
            
            if (ch == '&' && p[1] != '#') {
                const char *entity = ENTITIES;
                

                const char *semicolon = strchr(p, ';');
                unsigned int len;
                if (!semicolon)
                    locerrorexit(comment->filename, linenum, "unterminated HTML entity");
                p++;
                for (;;) {
                    len = strlen(entity);
                    if (!len)
                        locerrorexit(comment->filename, linenum, "unrecognised HTML entity &%.*s;", semicolon - p, p);
                    if (len == semicolon - p && !memcmp(p, entity, len))
                        break;
                    entity += len + 1;
                    entity += strlen(entity) + 1;
                }
                entity += len + 1;
                cnode = addtext(cnode, entity, strlen(entity));
                p = semicolon + 1;
                ch = *p;
                starttext = p;
                continue;
            }
            
            else if (ch == '\\') {
                const char *match = "\\@&$#<>%";
                const char *pos;
                ch = p[1];
                pos = strchr(match, ch);
                if (pos) {
                    
                    const char *text = 
                        "\\\0    @\0    &amp;\0$\0    #\0    &lt;\0 >\0    %"
                        + 6 * (pos - match);
                    cnode = addtext(cnode, text, strlen(text));
                    p += 2;
                    ch = *p;
                    starttext = p;
                    continue;
                }
            } else if (ch == '<') {
                if (incode) {
                    ch = *++p;
                    starttext = p;
                    continue;
                }
                
                p = parsehtmltag(p, &cnode, comment->filename, &linenum);
                ch = *p;
                starttext = p;
                continue;
            }
            {
                
                const char *start = ++p;
                unsigned int cmdlen;
                const struct command *command;
                ch = *p;
                while ((unsigned)((ch & ~0x20) - 'A') <= 'Z' - 'A'
                        || (unsigned)(ch - '0') < 10 || ch == '_' || ch == '-')
                {
                    ch = *++p;
                }
                cmdlen = p - start;
                if (!cmdlen)
                    locerrorexit(comment->filename, linenum, "\\ or @ without Doxygen command");
                
                command = commands;
                for (;;) {
                    if (!command->namelen) {
                        locerrorexit(comment->filename, linenum, "unrecognized Doxygen command '%.*s'",
                                cmdlen + 1, start - 1);
                    }
                    if (command->namelen == cmdlen
                            && !memcmp(command->name, start, cmdlen))
                    {
                        break;
                    }
                    command++;
                }
                p = (*command->func)(p, &cnode, command->type,
                        comment->filename, linenum, command->name);
                ch = *p;
                starttext = p;
            }
        }
        if (p - starttext) {
            
            if (cnode->funcs->needpara)
                cnode = startpara(cnode, &para_funcs);
            cnode = addtext(cnode, starttext, p - starttext);
        }
        if (!ch)
            break;
        if (cnode->funcs == &text_funcs)
            addtext(cnode, "\n", 1);
        p++;
    }
    
    do
        cnode = endcnode(cnode);
    while (cnode);
    assert(!incode);
    assert(!inhtmlblock);
}






static void
parsecomments(struct comment *comment)
{
    while (comment) {
        parsecomment(comment);
        comment = comment->next;
    }
}







static void
attachcommenttonode(struct node *node, struct comment *comment)
{
    comment->next = node->comments;
    node->comments = comment;
}







static void
attachcomments(struct comment *comment, struct node *root)
{
    while (comment) {
        struct comment *next = comment->next;
        




        struct cnode **pcnode = &comment->root.children;
        for (;;) {
            struct cnode *cnode = *pcnode;
            if (!cnode)
                break;
            if (cnode->funcs == &param_funcs || cnode->funcs == &return_funcs
                    || cnode->funcs == &throw_funcs)
            {
                

                struct node *node;
                struct comment *newcomment;
                if (cnode->funcs == &param_funcs) {
                    node = findparamidentifier(comment->node,
                        ((struct paramcnode *)cnode)->name);
                    if (!node)
                        locerrorexit(comment->filename, cnode->linenum, "no parameter '%s' found", ((struct paramcnode *)cnode)->name);
                } else if (cnode->funcs == &return_funcs) {
                    node = findreturntype(comment->node);
                    if (!node)
                        locerrorexit(comment->filename, cnode->linenum, "no return type found");
                } else {
                    node = findthrowidentifier(comment->node,
                        ((struct paramcnode *)cnode)->name);
                    if (!node)
                        locerrorexit(comment->filename, cnode->linenum, "no exception '%s' found", ((struct paramcnode *)cnode)->name);
                }
                
                *pcnode = cnode->next;
                
                newcomment = memalloc(sizeof(struct comment));
                newcomment->root.funcs = &root_funcs;
                newcomment->linenum = cnode->linenum;
                
                newcomment->root.children = cnode;
                cnode->parent = &newcomment->root;
                cnode->next = 0;
                

                cnode->funcs = &return_funcs;
                
                attachcommenttonode(node, newcomment);
            } else {
                pcnode = &cnode->next;
            }
        }
        
        {
            struct node *node = comment->node;
            if (!node)
                node = root;
            attachcommenttonode(node, comment);
        }
        comment = next;
    }
}






void
processcomments(struct node *root)
{
    comments = joininlinecomments(comments);
    parsecomments(comments);
    attachcomments(comments, root);
}







void
outputdescriptive(struct node *node, unsigned int indent)
{
    struct comment *comment = node->comments;
    int indescriptive = 0;
    while (comment) {
        struct cnode *root = &comment->root;
        if (!indescriptive)
            printf("%*s<descriptive>\n", indent, "");
        indescriptive = 1;
        (*root->funcs->output)(root, indent + 2);
        comment = comment->next;
    }
    if (indescriptive)
        printf("%*s</descriptive>\n", indent, "");
}
