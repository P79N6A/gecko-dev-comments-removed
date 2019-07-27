














#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "comment.h"
#include "lex.h"
#include "misc.h"
#include "node.h"
#include "process.h"

struct node *
newelement(const char *name)
{
    struct element *element = memalloc(sizeof(struct element));
    element->n.type = NODE_ELEMENT;
    element->name = name;
    return &element->n;
}

struct node *
newattr(const char *name, const char *val)
{
    struct attr *attr = memalloc(sizeof(struct attr));
    attr->n.type = NODE_ATTR;
    attr->name = name;
    attr->value = val;
    return &attr->n;
}

struct node *
newattrlist(void)
{
    struct attrlist *attrlist = memalloc(sizeof(struct attrlist));
    attrlist->n.type = NODE_ATTRLIST;
    return &attrlist->n;
}













void
addnode(struct node *parent, struct node *child)
{
    if (!child)
        return;
    if (child->type == NODE_ATTRLIST) {
        
        struct node *child2;
        reversechildren(child);
        child2 = child->children;
        memfree(child);
        while (child2) {
            struct node *next = child2->next;
            addnode(parent, child2);
            child2 = next;
        }
    } else {
        child->next = parent->children;
        parent->children = child;
        child->parent = parent;
    }
}






void
reversechildren(struct node *node)
{
    struct node *newlist = 0;
    struct node *child = node->children;
    while (child) {
        struct node *next = child->next;
        child->parent = node;
        child->next = newlist;
        newlist = child;
        reversechildren(child);
        child = next;
    }
    node->children = newlist;
}




int
nodeisempty(struct node *node)
{
    return !node->children;
}






struct node *
nodewalk(struct node *node)
{
    if (node->children)
        return node->children;
    if (node->next)
        return node->next;
    do {
        node = node->parent;
        if (!node)
            return 0;
    } while (!node->next);
    return node->next;
}









static struct node *
findchildelement(struct node *node, const char *name)
{
    node = node->children;
    while (node) {
        if (node->type == NODE_ELEMENT) {
            struct element *element = (void *)node;
            if (!strcmp(element->name, name))
                break;
        }
        node = node->next;
    }
    return node;
}









const char *
getattr(struct node *node, const char *name)
{
    node = node->children;
    while (node) {
        if (node->type == NODE_ATTR) {
            struct attr *attr = (void *)node;
            if (!strcmp(attr->name, name))
                return attr->value;
        }
        node = node->next;
    }
    return 0;
}










static struct node *
findchildelementwithnameattr(struct node *node, const char *name)
{
    node = node->children;
    while (node) {
        if (node->type == NODE_ELEMENT) {
            const char *s = getattr(node, "name");
            if (s && !strcmp(s, name))
                break;
        }
        node = node->next;
    }
    return node;
}








struct node *
findreturntype(struct node *node)
{
    return findchildelement(node, "Type");
}









struct node *
findparamidentifier(struct node *node, const char *name)
{
    node = findchildelement(node, "ArgumentList");
    if (node)
        node = findchildelementwithnameattr(node, name);
    return node;
}










struct node *
findthrowidentifier(struct node *node, const char *name)
{
    struct node *node2 = findchildelement(node, "Raises");
    if (node2)
        node2 = findchildelementwithnameattr(node2, name);
    if (!node2) {
        node2 = findchildelement(node, "SetRaises");
        if (node2)
            node2 = findchildelementwithnameattr(node2, name);
    }
    return node2;
}




static void
outputid(struct node *node)
{
    if (node->parent)
        outputid(node->parent);
    if (node->id) {
        fputs("::", stdout);
        printtext(node->id, strlen(node->id), 1);
    }
}







void
outputnode(struct node *node, unsigned int indent)
{
    struct element *element = (void *)node;
    struct node *child;
    int empty = 1;
    printf("%*s<%s", indent, "", element->name);
    child = element->n.children;
    while (child) {
        switch(child->type) {
        case NODE_ELEMENT:
            empty = 0;
            break;
        case NODE_ATTR:
            {
                struct attr *attr = (void *)child;
                printf(" %s=\"", attr->name);
                printtext(attr->value, strlen(attr->value), 1);
                printf("\"");
            }
            break;
        }
        child = child->next;
    }
    if (node->id) {
        printf(" id=\"");
        outputid(node);
        printf("\"");
    }
    if (!empty || node->comments || node->wsstart) {
        printf(">\n");
        if (node->wsstart) {
            printf("%*s  <webidl>", indent, "");
            outputwidl(node);
            printf("</webidl>\n");
        }
        outputdescriptive(node, indent + 2);
        child = element->n.children;
        while (child) {
            switch(child->type) {
            case NODE_ELEMENT:
                outputnode(child, indent + 2);
                break;
            }
            child = child->next;
        }
        printf("%*s</%s>\n", indent, "", element->name);
    } else
        printf("/>\n");
}


