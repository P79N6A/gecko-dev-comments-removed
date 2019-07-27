












#ifndef node_h
#define node_h


enum { NODE_ELEMENT, NODE_ATTR, NODE_ATTRLIST };
struct node {
    int type;
    struct node *next;
    struct node *parent;
    struct node *children;
    struct comment *comments; 
    

    const char *wsstart;
    


    const char *start;
    const char *end;
    const char *id;
};

struct element {
    struct node n;
    const char *name;
};

struct attr {
    struct node n;
    const char *name;
    const char *value;
};

struct attrlist {
    struct node n;
};

struct node *newelement(const char *name);
struct node *newattr(const char *name, const char *val);
struct node *newattrlist(void);
void addnode(struct node *parent, struct node *child);
void reversechildren(struct node *node);
int nodeisempty(struct node *node);
const char *getattr(struct node *node, const char *name);
struct node *nodewalk(struct node *node);
struct node *findreturntype(struct node *node);
struct node *findparamidentifier(struct node *node, const char *name);
struct node *findthrowidentifier(struct node *node, const char *name);
void outputnode(struct node *node, unsigned int indent);

#endif 

