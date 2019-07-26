







#include "SkPictureStateTree.h"
#include "SkCanvas.h"

SkPictureStateTree::SkPictureStateTree()
    : fAlloc(2048)
    , fLastRestoredNode(NULL)
    , fStateStack(sizeof(Draw), 16) {
    fRootMatrix.reset();
    fRoot.fParent = NULL;
    fRoot.fMatrix = &fRootMatrix;
    fRoot.fFlags = Node::kSave_Flag;
    fRoot.fOffset = 0;
    fRoot.fLevel = 0;
    fCurrentState.fNode = &fRoot;
    fCurrentState.fMatrix = &fRootMatrix;
    *static_cast<Draw*>(fStateStack.push_back()) = fCurrentState;
}

SkPictureStateTree::~SkPictureStateTree() {
}

SkPictureStateTree::Draw* SkPictureStateTree::appendDraw(size_t offset) {
    Draw* draw = static_cast<Draw*>(fAlloc.allocThrow(sizeof(Draw)));
    *draw = fCurrentState;
    draw->fOffset = SkToU32(offset);
    return draw;
}

void SkPictureStateTree::appendSave() {
    *static_cast<Draw*>(fStateStack.push_back()) = fCurrentState;
    fCurrentState.fNode->fFlags |= Node::kSave_Flag;
}

void SkPictureStateTree::appendSaveLayer(size_t offset) {
    *static_cast<Draw*>(fStateStack.push_back()) = fCurrentState;
    this->appendNode(offset);
    fCurrentState.fNode->fFlags |= Node::kSaveLayer_Flag;
}

void SkPictureStateTree::saveCollapsed() {
    SkASSERT(NULL != fLastRestoredNode);
    SkASSERT(SkToBool(fLastRestoredNode->fFlags & \
        (Node::kSaveLayer_Flag | Node::kSave_Flag)));
    SkASSERT(fLastRestoredNode->fParent == fCurrentState.fNode);
    
    
    
    
    fLastRestoredNode->fFlags = 0;
}

void SkPictureStateTree::appendRestore() {
    fLastRestoredNode = fCurrentState.fNode;
    fCurrentState = *static_cast<Draw*>(fStateStack.back());
    fStateStack.pop_back();
}

void SkPictureStateTree::appendTransform(const SkMatrix& trans) {
    SkMatrix* m = static_cast<SkMatrix*>(fAlloc.allocThrow(sizeof(SkMatrix)));
    *m = trans;
    fCurrentState.fMatrix = m;
}

void SkPictureStateTree::appendClip(size_t offset) {
    this->appendNode(offset);
}

SkPictureStateTree::Iterator SkPictureStateTree::getIterator(const SkTDArray<void*>& draws,
                                                             SkCanvas* canvas) {
    return Iterator(draws, canvas, &fRoot);
}

void SkPictureStateTree::appendNode(size_t offset) {
    Node* n = static_cast<Node*>(fAlloc.allocThrow(sizeof(Node)));
    n->fOffset = SkToU32(offset);
    n->fFlags = 0;
    n->fParent = fCurrentState.fNode;
    n->fLevel = fCurrentState.fNode->fLevel + 1;
    n->fMatrix = fCurrentState.fMatrix;
    fCurrentState.fNode = n;
}

SkPictureStateTree::Iterator::Iterator(const SkTDArray<void*>& draws, SkCanvas* canvas, Node* root)
    : fDraws(&draws)
    , fCanvas(canvas)
    , fCurrentNode(root)
    , fPlaybackMatrix(canvas->getTotalMatrix())
    , fCurrentMatrix(NULL)
    , fPlaybackIndex(0)
    , fSave(false)
    , fValid(true) {
}

uint32_t SkPictureStateTree::Iterator::draw() {
    SkASSERT(this->isValid());
    if (fPlaybackIndex >= fDraws->count()) {
        
        fCanvas->setMatrix(fPlaybackMatrix);
        if (fCurrentNode->fFlags & Node::kSaveLayer_Flag) { fCanvas->restore(); }
        fCurrentNode = fCurrentNode->fParent;
        while (NULL != fCurrentNode) {
            if (fCurrentNode->fFlags & Node::kSave_Flag) { fCanvas->restore(); }
            if (fCurrentNode->fFlags & Node::kSaveLayer_Flag) { fCanvas->restore(); }
            fCurrentNode = fCurrentNode->fParent;
        }
        return kDrawComplete;
    }

    Draw* draw = static_cast<Draw*>((*fDraws)[fPlaybackIndex]);
    Node* targetNode = draw->fNode;

    if (fSave) {
        fCanvas->save(SkCanvas::kClip_SaveFlag);
        fSave = false;
    }

    if (fCurrentNode != targetNode) {
        
        
        if (fNodes.count() == 0) {
            
            
            
            
            
            Node* tmp = fCurrentNode;
            Node* ancestor = targetNode;
            while (tmp != ancestor) {
                uint16_t currentLevel = tmp->fLevel;
                uint16_t targetLevel = ancestor->fLevel;
                if (currentLevel >= targetLevel) {
                    if (tmp != fCurrentNode && tmp->fFlags & Node::kSave_Flag) { fCanvas->restore(); }
                    if (tmp->fFlags & Node::kSaveLayer_Flag) { fCanvas->restore(); }
                    tmp = tmp->fParent;
                }
                if (currentLevel <= targetLevel) {
                    fNodes.push(ancestor);
                    ancestor = ancestor->fParent;
                }
            }

            if (ancestor->fFlags & Node::kSave_Flag) {
                if (fCurrentNode != ancestor) { fCanvas->restore(); }
                if (targetNode != ancestor) { fCanvas->save(SkCanvas::kClip_SaveFlag); }
            }
            fCurrentNode = ancestor;
        }

        
        
        if (fCurrentNode != targetNode) {
            if (fCurrentMatrix != fNodes.top()->fMatrix) {
                fCurrentMatrix = fNodes.top()->fMatrix;
                SkMatrix tmp = *fNodes.top()->fMatrix;
                tmp.postConcat(fPlaybackMatrix);
                fCanvas->setMatrix(tmp);
            }
            uint32_t offset = fNodes.top()->fOffset;
            fCurrentNode = fNodes.top();
            fSave = fCurrentNode != targetNode && fCurrentNode->fFlags & Node::kSave_Flag;
            fNodes.pop();
            return offset;
        }
    }

    
    

    if (fCurrentMatrix != draw->fMatrix) {
        SkMatrix tmp = *draw->fMatrix;
        tmp.postConcat(fPlaybackMatrix);
        fCanvas->setMatrix(tmp);
        fCurrentMatrix = draw->fMatrix;
    }

    ++fPlaybackIndex;
    return draw->fOffset;
}
