






































#ifndef __PATRICIA__
#define __PATRICIA__


















#ifdef __cplusplus
extern "C" {
#endif








typedef void *PatriciaTreeRef;






typedef int (*NodeReplaceFunction) (void* *nodeDataPtr, unsigned char *key, void *replaceData, void *refCon);
typedef int (*NodeTraverseFunction) (void *nodeData, unsigned char *key, void *arg, void *refCon);
typedef int (*NodeFreeFunction) (void *nodeData, unsigned char *key, void *refCon);







PatriciaTreeRef PatriciaInitTree(long numKeyBits);
void	PatriciaFreeTree(PatriciaTreeRef treeRef, NodeFreeFunction freeFunc, void *refCon);





int 	PatriciaInsert(PatriciaTreeRef treeRef, NodeReplaceFunction replaceFunc, const unsigned char *key, void *data, void *refCon);
int 	PatriciaSearch(PatriciaTreeRef treeRef, const unsigned char *key, void **data);
int 	PatriciaTraverse(PatriciaTreeRef treeRef, NodeTraverseFunction traverseFunc, void *arg, void *refCon);



long PatriciaGetNumNodes(PatriciaTreeRef treeRef);

#ifdef __cplusplus
}
#endif

#endif

