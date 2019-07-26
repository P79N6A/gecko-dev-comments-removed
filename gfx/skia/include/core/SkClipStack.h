






#ifndef SkClipStack_DEFINED
#define SkClipStack_DEFINED

#include "SkDeque.h"
#include "SkPath.h"
#include "SkRect.h"
#include "SkRegion.h"
#include "SkTDArray.h"








class SK_API SkClipStack {
public:
    enum BoundsType {
        
        kNormal_BoundsType,
        
        
        
        
        
        kInsideOut_BoundsType
    };

    class Element {
    public:
        enum Type {
            
            kEmpty_Type,
            
            kRect_Type,
            
            kPath_Type,
        };

        Element() {
            this->initCommon(0, SkRegion::kReplace_Op, false);
            this->setEmpty();
        }

        Element(const SkRect& rect, SkRegion::Op op, bool doAA) {
            this->initRect(0, rect, op, doAA);
        }

        Element(const SkPath& path, SkRegion::Op op, bool doAA) {
            this->initPath(0, path, op, doAA);
        }

        bool operator== (const Element& element) const {
            if (this == &element) {
                return true;
            }
            if (fOp != element.fOp ||
                fType != element.fType ||
                fDoAA != element.fDoAA ||
                fSaveCount != element.fSaveCount) {
                return false;
            }
            switch (fType) {
                case kPath_Type:
                    return fPath == element.fPath;
                case kRect_Type:
                    return fRect == element.fRect;
                case kEmpty_Type:
                    return true;
                default:
                    SkDEBUGFAIL("Unexpected type.");
                    return false;
            }
        }
        bool operator!= (const Element& element) const { return !(*this == element); }

        
        Type getType() const { return fType; }

        
        const SkPath& getPath() const { return fPath; }

        
        const SkRect& getRect() const { return fRect; }

        
        SkRegion::Op getOp() const { return fOp; }

        

        bool isAA() const { return fDoAA; }

        
        void invertShapeFillType();

        
        void setOp(SkRegion::Op op) { fOp = op; }

        




        int32_t getGenID() const { return fGenID; }

        



        const SkRect& getBounds() const {
            static const SkRect kEmpty = { 0, 0, 0, 0 };
            switch (fType) {
                case kRect_Type:
                    return fRect;
                case kPath_Type:
                    return fPath.getBounds();
                case kEmpty_Type:
                    return kEmpty;
                default:
                    SkDEBUGFAIL("Unexpected type.");
                    return kEmpty;
            }
        }

        



        bool contains(const SkRect& rect) const {
            switch (fType) {
                case kRect_Type:
                    return fRect.contains(rect);
                case kPath_Type:
                    return fPath.conservativelyContainsRect(rect);
                case kEmpty_Type:
                    return false;
                default:
                    SkDEBUGFAIL("Unexpected type.");
                    return false;
            }
        }

        


        bool isInverseFilled() const {
            return kPath_Type == fType && fPath.isInverseFillType();
        }

    private:
        friend class SkClipStack;

        SkPath          fPath;
        SkRect          fRect;
        int             fSaveCount; 
        SkRegion::Op    fOp;
        Type            fType;
        bool            fDoAA;

        









        SkClipStack::BoundsType fFiniteBoundType;
        SkRect                  fFiniteBound;

        
        
        bool                    fIsIntersectionOfRects;

        int                     fGenID;

        Element(int saveCount) {
            this->initCommon(saveCount, SkRegion::kReplace_Op, false);
            this->setEmpty();
        }

        Element(int saveCount, const SkRect& rect, SkRegion::Op op, bool doAA) {
            this->initRect(saveCount, rect, op, doAA);
        }

        Element(int saveCount, const SkPath& path, SkRegion::Op op, bool doAA) {
            this->initPath(saveCount, path, op, doAA);
        }

        void initCommon(int saveCount, SkRegion::Op op, bool doAA) {
            fSaveCount = saveCount;
            fOp = op;
            fDoAA = doAA;
            
            
            fFiniteBoundType = kInsideOut_BoundsType;
            fFiniteBound.setEmpty();
            fIsIntersectionOfRects = false;
            fGenID = kInvalidGenID;
        }

        void initRect(int saveCount, const SkRect& rect, SkRegion::Op op, bool doAA) {
            fRect = rect;
            fType = kRect_Type;
            this->initCommon(saveCount, op, doAA);
        }

        void initPath(int saveCount, const SkPath& path, SkRegion::Op op, bool doAA) {
            fPath = path;
            fType = kPath_Type;
            this->initCommon(saveCount, op, doAA);
        }

        void setEmpty() {
            fType = kEmpty_Type;
            fFiniteBound.setEmpty();
            fFiniteBoundType = kNormal_BoundsType;
            fIsIntersectionOfRects = false;
            fRect.setEmpty();
            fPath.reset();
            fGenID = kEmptyGenID;
        }

        
        inline void checkEmpty() const;
        inline bool canBeIntersectedInPlace(int saveCount, SkRegion::Op op) const;
        


        bool rectRectIntersectAllowed(const SkRect& newR, bool newAA) const;
        

        void updateBoundAndGenID(const Element* prior);
        
        enum FillCombo {
            kPrev_Cur_FillCombo,
            kPrev_InvCur_FillCombo,
            kInvPrev_Cur_FillCombo,
            kInvPrev_InvCur_FillCombo
        };
        
        inline void combineBoundsDiff(FillCombo combination, const SkRect& prevFinite);
        inline void combineBoundsXOR(int combination, const SkRect& prevFinite);
        inline void combineBoundsUnion(int combination, const SkRect& prevFinite);
        inline void combineBoundsIntersection(int combination, const SkRect& prevFinite);
        inline void combineBoundsRevDiff(int combination, const SkRect& prevFinite);
    };

    SkClipStack();
    SkClipStack(const SkClipStack& b);
    explicit SkClipStack(const SkRect& r);
    explicit SkClipStack(const SkIRect& r);
    ~SkClipStack();

    SkClipStack& operator=(const SkClipStack& b);
    bool operator==(const SkClipStack& b) const;
    bool operator!=(const SkClipStack& b) const { return !(*this == b); }

    void reset();

    int getSaveCount() const { return fSaveCount; }
    void save();
    void restore();

    








    void getBounds(SkRect* canvFiniteBound,
                   BoundsType* boundType,
                   bool* isIntersectionOfRects = NULL) const;

    




    bool intersectRectWithClip(SkRect* devRect) const;

    




    bool quickContains(const SkRect& devRect) const;

    void clipDevRect(const SkIRect& ir, SkRegion::Op op) {
        SkRect r;
        r.set(ir);
        this->clipDevRect(r, op, false);
    }
    void clipDevRect(const SkRect&, SkRegion::Op, bool doAA);
    void clipDevPath(const SkPath&, SkRegion::Op, bool doAA);
    
    void clipEmpty();

    



    bool isWideOpen() const;

    






    typedef void (*PFPurgeClipCB)(int genID, void* data);
    void addPurgeClipCallback(PFPurgeClipCB callback, void* data) const;

    


    void removePurgeClipCallback(PFPurgeClipCB callback, void* data) const;

    



    static const int32_t kInvalidGenID = 0;
    static const int32_t kEmptyGenID = 1;       
    static const int32_t kWideOpenGenID = 2;    

    int32_t getTopmostGenID() const;

public:
    class Iter {
    public:
        enum IterStart {
            kBottom_IterStart = SkDeque::Iter::kFront_IterStart,
            kTop_IterStart = SkDeque::Iter::kBack_IterStart
        };

        


        Iter();

        Iter(const SkClipStack& stack, IterStart startLoc);

        



        const Element* next();
        const Element* prev();

        



        const Element* skipToTopmost(SkRegion::Op op);

        


        void reset(const SkClipStack& stack, IterStart startLoc);

    private:
        const SkClipStack* fStack;
        SkDeque::Iter      fIter;
    };

    



    class B2TIter : private Iter {
    public:
        B2TIter() {}

        



        B2TIter(const SkClipStack& stack)
        : INHERITED(stack, kBottom_IterStart) {
        }

        using Iter::next;

        



        void reset(const SkClipStack& stack) {
            this->INHERITED::reset(stack, kBottom_IterStart);
        }

    private:

        typedef Iter INHERITED;
    };

    












    void getConservativeBounds(int offsetX,
                               int offsetY,
                               int maxWidth,
                               int maxHeight,
                               SkRect* devBounds,
                               bool* isIntersectionOfRects = NULL) const;

private:
    friend class Iter;

    SkDeque fDeque;
    int     fSaveCount;

    
    
    
    static int32_t     gGenID;

    struct ClipCallbackData {
        PFPurgeClipCB   fCallback;
        void*           fData;

        friend bool operator==(const ClipCallbackData& a,
                               const ClipCallbackData& b) {
            return a.fCallback == b.fCallback && a.fData == b.fData;
        }
    };

    mutable SkTDArray<ClipCallbackData> fCallbackData;

    


    void purgeClip(Element* element);

    


    static int32_t GetNextGenID();
};

#endif
