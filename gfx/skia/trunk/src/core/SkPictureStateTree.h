







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

    




    void initIterator(SkPictureStateTree::Iterator* iter, 
                      const SkTDArray<void*>& draws, 
                      SkCanvas* canvas);

    void appendSave();
    void appendSaveLayer(size_t offset);
    void appendRestore();
    void appendTransform(const SkMatrix& trans);
    void appendClip(size_t offset);

    




    void saveCollapsed();

    


    class Iterator {
    public:
        






        uint32_t nextDraw();
        static const uint32_t kDrawComplete = SK_MaxU32;
        Iterator() : fValid(false) { }
        bool isValid() const { return fValid; }

    private:
        void init(const SkTDArray<void*>& draws, SkCanvas* canvas, Node* root);

        void setCurrentMatrix(const SkMatrix*);

        
        const SkTDArray<void*>* fDraws;

        
        SkCanvas* fCanvas;

        
        Node* fCurrentNode;

        
        SkTDArray<Node*> fNodes;

        
        SkMatrix fPlaybackMatrix;

        
        const SkMatrix* fCurrentMatrix;

        
        int fPlaybackIndex;
        
        bool fSave;

        
        bool fValid;

        uint32_t finish();

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
