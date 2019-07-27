





#include "compiler/translator/IntermNode.h"
#include "compiler/translator/RemoveTree.h"




void RemoveAllTreeNodes(TIntermNode* root)
{
    std::queue<TIntermNode*> nodeQueue;

    nodeQueue.push(root);

    while (!nodeQueue.empty())
    {
        TIntermNode *node = nodeQueue.front();
        nodeQueue.pop();

        node->enqueueChildren(&nodeQueue);

        delete node;
    }
}

