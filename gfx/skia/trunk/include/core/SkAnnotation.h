






#ifndef SkAnnotation_DEFINED
#define SkAnnotation_DEFINED

#include "SkRefCnt.h"
#include "SkString.h"

class SkData;
class SkReadBuffer;
class SkWriteBuffer;
class SkStream;
class SkWStream;
struct SkPoint;





class SkAnnotation : public SkRefCnt {
public:
    virtual ~SkAnnotation();

    static SkAnnotation* Create(const char key[], SkData* value) {
        return SkNEW_ARGS(SkAnnotation, (key, value));
    }

    static SkAnnotation* Create(SkReadBuffer& buffer) {
        return SkNEW_ARGS(SkAnnotation, (buffer));
    }

    


    SkData* find(const char key[]) const;

    void writeToBuffer(SkWriteBuffer&) const;

private:
    SkAnnotation(const char key[], SkData* value);
    SkAnnotation(SkReadBuffer&);

    SkString    fKey;
    SkData*     fData;

    typedef SkRefCnt INHERITED;
};




class SkAnnotationKeys {
public:
    


    static const char* URL_Key();

    



    static const char* Define_Named_Dest_Key();

    



    static const char* Link_Named_Dest_Key();
};






struct SkRect;
class SkCanvas;











SK_API void SkAnnotateRectWithURL(SkCanvas*, const SkRect&, SkData*);











SK_API void SkAnnotateNamedDestination(SkCanvas*, const SkPoint&, SkData*);












SK_API void SkAnnotateLinkToDestination(SkCanvas*, const SkRect&, SkData*);


#endif
