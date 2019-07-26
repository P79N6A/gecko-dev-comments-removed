







#ifndef SkPictureStateTree_DEFINED
#define SkPictureStateTree_DEFINED

#include "SkTDArray.h"
#include "SkChunkAlloc.h"
#include "SkDeque.h"
#include "SkMatrix.h"
#include "SkRefCnt.h"

class SkCanvas;






class SkPictureStateTree : public SkRefCnt {
private:
    struct Node;
public:
    SK_DECLARE_INST_COUNT(SkPictureStateTree)

    



    struct Draw {
        SkMatrix* fMatrix;
        Node* fNode;
        uint32_t fOffset;
        bool operator<(const Draw& other) const { return fOffset < other.fOffset; }
    };

    class Iterator;

    SkPictureStateTree();
    ~SkPictureStateTree();

    


    Draw* appendDraw(size_t offset);

    




    Iterator getIterator(const SkTDArray<void*>& draws, SkCanvas* canvas);

    void appendSave();
    void appendSaveLayer(size_t offset);
    void appendRestore();
    void appendTransform(const SkMatrix& trans);
    void appendClip(size_t offset);

    




    void saveCollapsed();

    


    class Iterator {
    public:
        
        uint32_t draw();
        static const uint32_t kDrawComplete = SK_MaxU32;
        Iterator() : fPlaybackMatrix(), fValid(false) { }
        bool isValid() const { return fValid; }
    private:
        Iterator(const SkTDArray<void*>& draws, SkCanvas* canvas, Node* root);
        
        const SkTDArray<void*>* fDraws;

        
        SkCanvas* fCanvas;

        
        Node* fCurrentNode;

        
        SkTDArray<Node*> fNodes;

        
        const SkMatrix fPlaybackMatrix;

        
        SkMatrix* fCurrentMatrix;

        
        int fPlaybackIndex;
        
        bool fSave;

        
        bool fValid;

        friend class SkPictureStateTree;
    };

private:

    void appendNode(size_t offset);

    SkChunkAlloc fAlloc;
    
    
    
    Node* fLastRestoredNode;

    
    Draw fCurrentState;
    
    SkDeque fStateStack;

    
    
    struct Node {
        Node* fParent;
        uint32_t fOffset;
        uint16_t fLevel;
        uint16_t fFlags;
        SkMatrix* fMatrix;
        enum Flags {
            kSave_Flag      = 0x1,
            kSaveLayer_Flag = 0x2
        };
    };

    Node fRoot;
    SkMatrix fRootMatrix;

    typedef SkRefCnt INHERITED;
};

#endif
