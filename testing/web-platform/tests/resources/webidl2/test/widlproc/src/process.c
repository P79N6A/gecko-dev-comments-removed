












#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "comment.h"
#include "lex.h"
#include "misc.h"
#include "node.h"
#include "os.h"
#include "parse.h"
#include "process.h"

#if 0
static const char ntnames[] = { NTNAMES };
#endif 











void
printtext(const char *s, unsigned int len, int escamp)
{
    const char *p = s, *end = s + len;
    unsigned int count = 0;
    while (p != end) {
        int ch = *p;
        char buf[9];
        const char *seq = 0;
        count++;
        switch (ch) {
        case '<':
            seq = "&lt;";
            break;
        case '&':
            seq = escamp ? "&amp;" : "&";
            break;
        case '"':
            seq = "&quot;";
            break;
        case '\n':
            p++;
            count = 0;
            continue;
        case '\t':
            seq = "        " + ((count - 1) & 7);
            count = 0;
            break;
        default:
            if ((unsigned char)ch >= 0x20) {
                p++;
                continue;
            }
            snprintf(buf, 9, "&#%i;", ch);
            seq = buf;
            break;
        }
        if (p - s != fwrite(s, 1, p - s, stdout))
            errorexit("write error");
        fputs(seq, stdout);
        s = ++p;
    }
    if (p - s != fwrite(s, 1, p - s, stdout))
        errorexit("write error");
}

#if 0








static int
outputnodeastext(struct node *node, int needspace)
{
    if (node->type >= NT_START) {
        struct node *child = node->children;
        while (child) {
            needspace = outputnodeastext(child, needspace);
            child = child->next;
        }
    } else {
        unsigned int len = strlen(node->name);
        if (len) {
            int ch = node->name[0];
            if (ch == '_' || ((unsigned)(ch - '0') < 10
                    || (unsigned)((ch & ~0x20) - 'A') < 26))
            {
                if (needspace) putchar(' ');
            }
            ch = node->name[len - 1];
            if (ch == '_' || ((unsigned)(ch - '0') < 10
                    || (unsigned)((ch & ~0x20) - 'A') < 26))
            {
                needspace = 1;
            }
            printtext(node->name, len, 1);
        }
    }
    return needspace;
}








static int
printfqid(struct node *node)
{
    int any = 0;
    struct node *identifier;
    if (node->parent) {
        any = printfqid(node->parent);
    }
    switch (node->type) {
    case NT_Module:
    case NT_Interface:
    case NT_Typedef:
    case NT_Operation:
    case NT_Attribute:
    case NT_Const:
        if (any)
            printf(":");
        
        identifier = node->children;
        while (identifier) {
            if (identifier->type == TOK_IDENTIFIER)
                break;
            if (identifier->type == NT_TypedefRest) {
                identifier = identifier->children;
                continue;
            }
            identifier = identifier->next;
        }
        if (identifier) {
            printtext(identifier->name, strlen(identifier->name), 1);
            any = 1;
        }
        break;
    }
    return any;
}









static void outputchildren(struct node *node, struct node *identifier, unsigned int indent);

static void
output(struct node *node, struct node *extendedattributelist,
       unsigned int indent)
{
    if (extendedattributelist) {
        node->wsstart = extendedattributelist->wsstart;
        node->start = extendedattributelist->start;
    }
    if (node->type == NT_ExtendedAttribute) {
        printf("%*s<ExtendedAttribute value=\"", indent, "");
        outputnodeastext(node, 0);
        printf("\"/>\n");
    } else if (node->type == NT_BooleanLiteral) {
        printf("%*s<BooleanLiteral value=\"%s\"/>", indent, "",
            node->children->name);
    } else if (node->type == NT_ReadOnly) {
        printf("%*s<ReadOnly/>\n", indent, "");
    } else if (node->type >= NT_START) {
        const char *ntname;
        
        struct node *identifier = node->children;
        while (identifier) {
            if (identifier->type == TOK_IDENTIFIER)
                break;
            identifier = identifier->next;
        }
        
        ntname = ntnames + 2;
        while (node->type - NT_START != ((unsigned char)ntname[-2] | (unsigned char)ntname[-1] << 8))
            ntname += strlen(ntname) + 3;
        
        printf("%*s<%s", indent, "", ntname);
        
        if (identifier) {
            printf(" identifier=\"");
            printtext(identifier->name, strlen(identifier->name), 1);
            printf("\"");
        }
        switch (node->type) {
        case NT_Module:
        case NT_Interface:
        case NT_Typedef:
        case NT_Const:
            
            printf(" fqid=\"");
            printfqid(node);
            printf("\"");
            break;
        }
        if (!identifier && !extendedattributelist && !node->children && !node->comments)
            printf("/>\n");
        else {
            printf(">\n");
            
            outputdescriptive(node, indent + 2);
            
            if (identifier)
                outputdescriptive(identifier, indent + 2);
            
            if (extendedattributelist)
                output(extendedattributelist, 0, indent + 2);
            
            outputchildren(node, identifier, indent + 2);
            printf("%*s</%s>\n", indent, "", ntname);
        }
    } else switch (node->type) {
    case TOK_DOMString:
    case TOK_any:
    case TOK_boolean:
    case TOK_octet:
    case TOK_float:
    case TOK_double:
    case TOK_Object:
    case TOK_unsigned:
    case TOK_short:
    case TOK_long:
    case TOK_void:
        printf("%*s<%s/>\n", indent, "", node->name);
        break;
    case TOK_INTEGER:
        printf("%*s<integer value=\"", indent, "");
        printtext(node->name, strlen(node->name), 1);
        printf("\"/>\n");
        break;
    case TOK_FLOAT:
        printf("%*s<Float value=\"", indent, "");
        printtext(node->name, strlen(node->name), 1);
        printf("\"/>\n");
        break;
    case TOK_STRING:
        printf("%*s<string value=\"", indent, "");
        printtext(node->name, strlen(node->name), 1);
        printf("\"/>\n");
        break;
    }
}








static void
outputchildren(struct node *node, struct node *identifier, unsigned int indent)
{
    struct node *extendedattributelist;
    struct node *child;
    child = node->children;
    extendedattributelist = 0;
    while (child) {
        if (child->type == NT_ExtendedAttributeList && node->type != NT_Argument)
            extendedattributelist = child;
        else {
            if (identifier != child)
                output(child, extendedattributelist, indent);
            extendedattributelist = 0;
        }
        child = child->next;
    }
}
#endif 






void
processfiles(const char *const *names, int dtdref)
{
    struct node *root;
    readinput(names);
    root = parse();
    processcomments(root);
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    if(dtdref)
        printf("<!DOCTYPE Definitions SYSTEM \"widlprocxml.dtd\">\n");
    outputnode(root, 0);
}

