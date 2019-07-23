




































































#include <string.h>
#include <stdlib.h>


#include "patricia.h"

#include "nsAEDefs.h"       






typedef struct TNodeTag {
	unsigned char		*key;			
	short			bit;				
	long				nodeID;			
	struct TNodeTag	*left;			
	struct TNodeTag	*right;			
	void				*data;			
} TNode;


typedef struct {
	TNode*			headNode;			
	long				numKeyBits;		
	long				keySize;			
	long				numNodes;		
} TPatriciaTree;




#define TestKeyBit(key, bit)				( (key[bit >> 3] & (1 << (bit & 7))) != 0 )
#define CompareKeyBits(key1, key2, bit)		( (key1[bit >> 3] & (1 << (bit & 7))) == (key2[bit >> 3] & (1 << (bit & 7))) )
















static TNode *MakeNewNode(TPatriciaTree *tree, const unsigned char *key, short bit, void *nodeData)
{
	static long		nodeID = 0;
	TNode		*newNode;
	
	newNode = (TNode *)calloc(1, sizeof(TNode));
	if (newNode == NULL) return NULL;

	newNode->key = (unsigned char *)calloc(tree->keySize + 1, 1);		
	if (newNode->key == NULL) {
		free(newNode);
		return NULL;
	}
	memcpy(newNode->key, key, tree->keySize);
	newNode->bit = bit;
	newNode->data = nodeData;
	newNode->nodeID = nodeID;
	nodeID ++;
	
	return newNode;
}
















static TNode *MakeHeadNode(TPatriciaTree *tree)
{
	TNode		*newNode;
	
	newNode = (TNode *)calloc(1, sizeof(TNode));
	if (newNode == NULL) return NULL;

	newNode->key = (unsigned char *)calloc(tree->keySize + 1, 1);		
	if (newNode->key == NULL) {
		free(newNode);
		return NULL;
	}
	memset(newNode->key, 0, tree->keySize);
	newNode->bit = tree->numKeyBits;
	newNode->data = NULL;
	newNode->nodeID = -1;
	
	
	newNode->left = newNode;
	newNode->right = newNode;
	
	return newNode;
}



















static TNode *InternalSearch(TPatriciaTree *tree, TNode *x, const unsigned char *key)
{
	TNode	*p;

	AE_ASSERT(x, "No node");

	do {
		p = x;
		
		if (TestKeyBit(key, x->bit))
			x = x->right;
		else
			x = x->left;
		
	} while (p->bit > x->bit);
	
	return x;
}





















static int InternalTraverse(TPatriciaTree *tree, TNode *x, NodeTraverseFunction traverseFunc, void *arg1, void *arg2)
{
	TNode	*p;
	int		err = 0;
	
	AE_ASSERT(x, "No node");
	AE_ASSERT(x->left && x->right, "Left or right child missing");

#ifdef VERBOSE
	printf("Visiting node %ld with left %ld and right %ld\n", x->nodeID, x->left->nodeID, x->right->nodeID);
#endif

	if (x != tree->headNode) {
		err = (*traverseFunc)(x->data, x->key, arg1, arg2);
		if (err != 0) return err;
	}
	
	p = x->left;
	if (p->bit < x->bit)
		err = InternalTraverse(tree, p, traverseFunc, arg1, arg2);

	if (err != 0) return err;
	
	p = x->right;
	if (p->bit < x->bit)
		err = InternalTraverse(tree, p, traverseFunc, arg1, arg2);
	
	return err;
}





















static int TraverseAndFree(TPatriciaTree *tree, TNode *x, NodeFreeFunction freeFunc, void *refCon)
{
	TNode	*p;
	int		err = 0;
	
	AE_ASSERT(x, "No node");
	AE_ASSERT(x->left && x->right, "Left or right child missing");
		
	p = x->left;
	if (p->bit < x->bit) {
		err = TraverseAndFree(tree, p, freeFunc, refCon);
		if (err != 0) return err;
	}
	
	p = x->right;
	if (p->bit < x->bit) {
		err = TraverseAndFree(tree, p, freeFunc, refCon);
		if (err != 0) return err;
	}

	err = (*freeFunc)(x->data, x->key, refCon);
	
#ifdef VERBOSE
	printf("Freeing node %ld\n", x->nodeID);
#endif

	free(x->key);
	free(x);
	
	return err;
}


#pragma mark -















PatriciaTreeRef PatriciaInitTree(long numKeyBits)
{
	TPatriciaTree		*tree = NULL;

	tree = (TPatriciaTree *)calloc(1, sizeof(TPatriciaTree));
	if (tree == NULL) return NULL;
	
	tree->numKeyBits = numKeyBits;
	tree->keySize = (numKeyBits >> 3) + ((numKeyBits & 7) != 0);
	tree->numNodes = 0;
	
	tree->headNode = MakeHeadNode(tree);
	if (tree->headNode == NULL) {
		free(tree);
		return NULL;
	}
	
	return (PatriciaTreeRef)tree;
}



















void PatriciaFreeTree(PatriciaTreeRef treeRef, NodeFreeFunction freeFunc, void *refCon)
{
	TPatriciaTree	*tree = (TPatriciaTree *)treeRef;

	if (tree == NULL) return;
	
	
	TraverseAndFree(tree, tree->headNode, freeFunc, refCon);
	
	free(tree);
}



















int PatriciaSearch(PatriciaTreeRef treeRef, const unsigned char *key, void **data)
{
	TPatriciaTree	*tree = (TPatriciaTree *)treeRef;
	TNode		*foundNode;
	
	AE_ASSERT(tree, "Where is my tree?");

	foundNode = InternalSearch(tree, tree->headNode, key);
	AE_ASSERT(foundNode, "Should have found node");

	if (memcmp(foundNode->key, key, tree->keySize) == 0) {
		if (data != NULL)
			*data = foundNode->data;
		return 1;
	} else
		return 0;
}




















int PatriciaInsert(PatriciaTreeRef treeRef, NodeReplaceFunction replaceFunc, const unsigned char *key, void *data, void *refCon)
{
	TPatriciaTree	*tree = (TPatriciaTree *)treeRef;
	TNode		*x, *t, *p;
	short		i;

	x = tree->headNode;
	t = InternalSearch(tree, x, key);

	AE_ASSERT(t, "Should have found node");

	if (memcmp(t->key, key, tree->keySize) == 0) {
		if (replaceFunc) (*replaceFunc)(&t->data, t->key, data, refCon);
		return 1;			
	}
	
	i = tree->numKeyBits - 1;
	
	while (CompareKeyBits(key, t->key, i))
		i --;	
		
	do {
		p = x;
		x = (TestKeyBit(key, x->bit)) ? x->right : x->left;
	} while (x->bit > i && p->bit > x->bit);
	
	t = MakeNewNode(tree, key, i, data);
	if (t == NULL) return -1;
	
	if (TestKeyBit(key, t->bit)) {
		t->right = t;
		t->left = x;
	} else {
		t->right = x;
		t->left = t;
	}
		
	if (TestKeyBit(key, p->bit))
		p->right = t;
	else
		p->left = t;
	
#ifdef VERBOSE
	printf("Inserted node %ld with left %ld and right %ld\n", t->nodeID, t->left->nodeID, t->right->nodeID);
#endif

	tree->numNodes ++;
	
	return 0;
}























int PatriciaTraverse(PatriciaTreeRef treeRef, NodeTraverseFunction traverseFunc, void *arg, void *refCon)
{
	TPatriciaTree	*tree = (TPatriciaTree *)treeRef;
	
	return InternalTraverse(tree, tree->headNode, traverseFunc, arg, refCon);
}










long PatriciaGetNumNodes(PatriciaTreeRef treeRef)
{
	return ((TPatriciaTree *)treeRef)->numNodes;
}
