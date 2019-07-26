








#ifndef SkPDFShader_DEFINED
#define SkPDFShader_DEFINED

#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkMatrix.h"
#include "SkRefCnt.h"
#include "SkShader.h"

class SkObjRef;
class SkPDFCatalog;







class SkPDFShader {
public:
    











    static SkPDFObject* GetPDFShader(const SkShader& shader,
                                     const SkMatrix& matrix,
                                     const SkIRect& surfaceBBox);

protected:
    class State;

    class ShaderCanonicalEntry {
    public:
        ShaderCanonicalEntry(SkPDFObject* pdfShader, const State* state);
        bool operator==(const ShaderCanonicalEntry& b) const;

        SkPDFObject* fPDFShader;
        const State* fState;
    };
    
    static SkTDArray<ShaderCanonicalEntry>& CanonicalShaders();
    static SkBaseMutex& CanonicalShadersMutex();

    
    
    
    static SkPDFObject* GetPDFShaderByState(State* shaderState);
    static void RemoveShader(SkPDFObject* shader);

    SkPDFShader();
    virtual ~SkPDFShader() {};

    virtual bool isValid() = 0;
};

#endif
