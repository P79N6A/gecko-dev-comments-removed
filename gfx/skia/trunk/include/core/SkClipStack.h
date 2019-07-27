






#ifndef SkClipStack_DEFINED
#define SkClipStack_DEFINED

#include "SkDeque.h"
#include "SkPath.h"
#include "SkRect.h"
#include "SkRRect.h"
#include "SkRegion.h"
#include "SkTDArray.h"
#include "SkTLazy.h"

class SkCanvasClipVisitor;







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
            
            kRRect_Type,
            
            kPath_Type,

            kLastType = kPath_Type
        };
        static const int kTypeCnt = kLastType + 1;

        Element() {
            this->initCommon(0, SkRegion::kReplace_Op, false);
            this->setEmpty();
        }

        Element(const Element&);

        Element(const SkRect& rect, SkRegion::Op op, bool doAA) {
            this->initRect(0, rect, op, doAA);
        }

        Element(const SkRRect& rrect, SkRegion::Op op, bool doAA) {
            this->initRRect(0, rrect, op, doAA);
        }

        Element(const SkPath& path, SkRegion::Op op, bool doAA) {
            this->initPath(0, path, op, doAA);
        }

        bool operator== (const Element& element) const;
        bool operator!= (const Element& element) const { return !(*this == element); }

        
        Type getType() const { return fType; }

        
        int getSaveCount() const { return fSaveCount; }

        
        const SkPath& getPath() const { SkASSERT(kPath_Type == fType); return *fPath.get(); }

        
        const SkRRect& getRRect() const { SkASSERT(kRRect_Type == fType); return fRRect; }

        
        const SkRect& getRect() const {
            SkASSERT(kRect_Type == fType && (fRRect.isRect() || fRRect.isEmpty()));
            return fRRect.getBounds();
        }

        
        SkRegion::Op getOp() const { return fOp; }

        
        void asPath(SkPath* path) const;

        

        bool isAA() const { return fDoAA; }

        
        void invertShapeFillType();

        
        void setOp(SkRegion::Op op) { fOp = op; }

        




        int32_t getGenID() const { SkASSERT(kInvalidGenID != fGenID); return fGenID; }

        



        const SkRect& getBounds() const {
            static const SkRect kEmpty = { 0, 0, 0, 0 };
            switch (fType) {
                case kRect_Type:  
                case kRRect_Type:
                    return fRRect.getBounds();
                case kPath_Type:
                    return fPath.get()->getBounds();
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
                    return this->getRect().contains(rect);
                case kRRect_Type:
                    return fRRect.contains(rect);
                case kPath_Type:
                    return fPath.get()->conservativelyContainsRect(rect);
                case kEmpty_Type:
                    return false;
                default:
                    SkDEBUGFAIL("Unexpected type.");
                    return false;
            }
        }

        


        bool isInverseFilled() const {
            return kPath_Type == fType && fPath.get()->isInverseFillType();
        }

        


        void replay(SkCanvasClipVisitor*) const;

#ifdef SK_DEVELOPER
        



        void dump() const;
#endif

    private:
        friend class SkClipStack;

        SkTLazy<SkPath> fPath;
        SkRRect         fRRect;
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

        Element(int saveCount, const SkRRect& rrect, SkRegion::Op op, bool doAA) {
            this->initRRect(saveCount, rrect, op, doAA);
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
            fRRect.setRect(rect);
            fType = kRect_Type;
            this->initCommon(saveCount, op, doAA);
        }

        void initRRect(int saveCount, const SkRRect& rrect, SkRegion::Op op, bool doAA) {
            SkRRect::Type type = rrect.getType();
            fRRect = rrect;
            if (SkRRect::kRect_Type == type || SkRRect::kEmpty_Type == type) {
                fType = kRect_Type;
            } else {
                fType = kRRect_Type;
            }
            this->initCommon(saveCount, op, doAA);
        }

        void initPath(int saveCount, const SkPath& path, SkRegion::Op op, bool doAA);

        void setEmpty();

        
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
    void clipDevRRect(const SkRRect&, SkRegion::Op, bool doAA);
    void clipDevPath(const SkPath&, SkRegion::Op, bool doAA);
    
    void clipEmpty();

    



    bool isWideOpen() const;

    



    static const int32_t kInvalidGenID = 0;     
                                                
                                                
    static const int32_t kEmptyGenID = 1;       
    static const int32_t kWideOpenGenID = 2;    

    int32_t getTopmostGenID() const;

#ifdef SK_DEVELOPER
    



    void dump() const;
#endif

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

    


    void pushElement(const Element& element);

    


    void restoreTo(int saveCount);

    


    static int32_t GetNextGenID();
};

#endif
