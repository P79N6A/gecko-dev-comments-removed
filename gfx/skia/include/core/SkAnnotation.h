






#ifndef SkAnnotation_DEFINED
#define SkAnnotation_DEFINED

#include "SkFlattenable.h"

class SkData;
class SkDataSet;
class SkStream;
class SkWStream;





class SkAnnotation : public SkFlattenable {
public:
    enum Flags {
        
        kNoDraw_Flag  = 1 << 0,
    };

    SkAnnotation(SkDataSet*, uint32_t flags);
    virtual ~SkAnnotation();

    uint32_t getFlags() const { return fFlags; }
    SkDataSet* getDataSet() const { return fDataSet; }

    bool isNoDraw() const { return SkToBool(fFlags & kNoDraw_Flag); }

    


    SkData* find(const char name[]) const;

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkAnnotation)

protected:
    SkAnnotation(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

private:
    SkDataSet*  fDataSet;
    uint32_t    fFlags;

    void writeToStream(SkWStream*) const;
    void readFromStream(SkStream*);

    typedef SkFlattenable INHERITED;
};




class SkAnnotationKeys {
public:
    


    static const char* URL_Key();
};






struct SkRect;
class SkCanvas;











SK_API void SkAnnotateRectWithURL(SkCanvas*, const SkRect&, SkData*);

#endif
