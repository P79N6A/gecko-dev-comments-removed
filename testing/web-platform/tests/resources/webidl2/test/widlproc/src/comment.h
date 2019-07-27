












#ifndef comment_h
#define comment_h

struct tok;
struct node;

void addcomment(struct tok *tok);
void setcommentnode(struct node *node2);
void processcomments(struct node *root);
void outputdescriptive(struct node *node, unsigned int indent);

#endif 
