






































#include <AEUtils.h>

#include "PatriciaTree.h"






CPatriciaTree::CPatriciaTree(long keyBitsLen)
:	mTree(nil)
,	mKeyBits(keyBitsLen)
{

	mTree = PatriciaInitTree(mKeyBits);
	ThrowErrIfNil(mTree, paramErr);
}





CPatriciaTree::~CPatriciaTree()
{
	if (mTree)
	{
		PatriciaFreeTree(mTree, NodeFreeCallback, (void *)this);
	}
}


#pragma mark -








Boolean CPatriciaTree::InsertNode(TPatriciaKey key, CPatriciaNode* nodeData)
{
	int	result = PatriciaInsert(mTree, NodeReplaceCallback, key, (void *)nodeData, (void *)this);
	return (result == 1);
}






Boolean CPatriciaTree::SeekNode(TPatriciaKey key, CPatriciaNode**outNodeData)
{
	int	result = PatriciaSearch(mTree, key, (void **)outNodeData);
	return (result == 1);
}







Boolean CPatriciaTree::Traverse(NodeTraverseFunction traverseFcn, void *arg, void *refCon)
{
	int	result = PatriciaTraverse(mTree, traverseFcn, arg, refCon);
	return (result == 0);
}







long CPatriciaTree::GetNumNodes()
{
	return (mTree) ? PatriciaGetNumNodes(mTree) : 0;
}


#pragma mark -







int CPatriciaTree::ReplaceNode(CPatriciaNode**nodeDataPtr, TPatriciaKey key, CPatriciaNode *replaceData)
{
	return 0;
}







int CPatriciaTree::FreeNode(CPatriciaNode *nodeData, TPatriciaKey key)
{
	return 0;
}


#pragma mark -







int CPatriciaTree::NodeReplaceCallback(void**nodeDataPtr, unsigned char *key, void *replaceData, void *refCon)
{
	CPatriciaTree*		theTree = reinterpret_cast<CPatriciaTree *>(refCon);
	Assert(theTree);
	return theTree->ReplaceNode((CPatriciaNode**)nodeDataPtr, key, static_cast<CPatriciaNode *>(replaceData));
}







int CPatriciaTree::NodeFreeCallback(void *nodeData, unsigned char *key, void *refCon)
{
	CPatriciaTree*		theTree = reinterpret_cast<CPatriciaTree *>(refCon);
	Assert(theTree);
	return theTree->FreeNode(static_cast<CPatriciaNode *>(nodeData), key);
}

