






































#ifndef PatricaTree_h_
#define PatricaTree_h_

#include "patricia.h"



class CPatriciaNode
{
public:
						CPatriciaNode()		{}
	virtual				~CPatriciaNode()	{}
};




class CPatriciaTree
{
public:
	
	typedef	const unsigned char*		TPatriciaKey;	
	
						CPatriciaTree(long keyBitsLen);
	virtual				~CPatriciaTree();
				
	
	
	virtual Boolean			InsertNode(TPatriciaKey key, CPatriciaNode* nodeData);
	
	
	Boolean				SeekNode(TPatriciaKey key, CPatriciaNode**outNodeData);
	
	
	Boolean				Traverse(NodeTraverseFunction traverseFcn, void *arg, void *refCon);
	
	long					GetNumNodes();
	
protected:

	
	virtual int				ReplaceNode(CPatriciaNode**nodeDataPtr, TPatriciaKey key, CPatriciaNode *replaceData);
	virtual int				FreeNode(CPatriciaNode *nodeData, TPatriciaKey key);

private:

	
	static int				NodeReplaceCallback(void**nodeDataPtr, unsigned char *key, void *replaceData, void *refCon);
	static int				NodeFreeCallback(void *nodeData, unsigned char *key, void *refCon);

protected:

	PatriciaTreeRef		mTree;
	long					mKeyBits;

};




#endif 

