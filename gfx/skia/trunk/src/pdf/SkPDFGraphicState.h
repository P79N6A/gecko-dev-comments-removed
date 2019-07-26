








#ifndef SkPDFGraphicState_DEFINED
#define SkPDFGraphicState_DEFINED

#include "SkPaint.h"
#include "SkPDFTypes.h"
#include "SkTemplates.h"
#include "SkThread.h"

class SkPDFFormXObject;










class SkPDFGraphicState : public SkPDFDict {
    SK_DECLARE_INST_COUNT(SkPDFGraphicState)
public:
    enum SkPDFSMaskMode {
        kAlpha_SMaskMode,
        kLuminosity_SMaskMode
    };

    virtual ~SkPDFGraphicState();

    virtual void getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                              SkTSet<SkPDFObject*>* newResourceObjects);

    
    
    virtual void emitObject(SkWStream* stream, SkPDFCatalog* catalog,
                            bool indirect);
    virtual size_t getOutputSize(SkPDFCatalog* catalog, bool indirect);

    






    static SkPDFGraphicState* GetGraphicStateForPaint(const SkPaint& paint);

    






    static SkPDFGraphicState* GetSMaskGraphicState(SkPDFFormXObject* sMask,
                                                   bool invert,
                                                   SkPDFSMaskMode sMaskMode);

    





    static SkPDFGraphicState* GetNoSMaskGraphicState();

private:
    const SkPaint fPaint;
    SkTDArray<SkPDFObject*> fResources;
    bool fPopulated;
    bool fSMask;

    class GSCanonicalEntry {
    public:
        SkPDFGraphicState* fGraphicState;
        const SkPaint* fPaint;

        bool operator==(const GSCanonicalEntry& b) const;
        explicit GSCanonicalEntry(SkPDFGraphicState* gs)
            : fGraphicState(gs),
              fPaint(&gs->fPaint) {}
        explicit GSCanonicalEntry(const SkPaint* paint)
            : fGraphicState(NULL),
              fPaint(paint) {}
    };

    
    static SkTDArray<GSCanonicalEntry>& CanonicalPaints();
    static SkBaseMutex& CanonicalPaintsMutex();

    SkPDFGraphicState();
    explicit SkPDFGraphicState(const SkPaint& paint);

    void populateDict();

    static SkPDFObject* GetInvertFunction();

    static int Find(const SkPaint& paint);
    typedef SkPDFDict INHERITED;
};

#endif
