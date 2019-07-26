






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
    SkAnnotation(const char key[], SkData* value);
    virtual ~SkAnnotation();

    


    SkData* find(const char key[]) const;

    SkAnnotation(SkReadBuffer&);
    void writeToBuffer(SkWriteBuffer&) const;

private:
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
