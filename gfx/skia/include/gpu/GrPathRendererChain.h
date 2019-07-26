








#ifndef GrPathRendererChain_DEFINED
#define GrPathRendererChain_DEFINED

#include "GrRefCnt.h"
#include "SkTArray.h"

class GrContext;
class GrDrawTarget;
class GrPathRenderer;
class SkPath;
class SkStrokeRec;







class GrPathRendererChain : public SkRefCnt {
public:
    
    enum StencilSupport {
        kNoSupport_StencilSupport,
        kStencilOnly_StencilSupport,
        kNoRestriction_StencilSupport,
    };

    SK_DECLARE_INST_COUNT(GrPathRendererChain)

    GrPathRendererChain(GrContext* context);

    ~GrPathRendererChain();

    
    GrPathRenderer* addPathRenderer(GrPathRenderer* pr);

    

    enum DrawType {
        kColor_DrawType,                    
        kColorAntiAlias_DrawType,           
        kStencilOnly_DrawType,              
        kStencilAndColor_DrawType,          
        kStencilAndColorAntiAlias_DrawType  
                                            
    };
    



    GrPathRenderer* getPathRenderer(const SkPath& path,
                                    const SkStrokeRec& rec,
                                    const GrDrawTarget* target,
                                    DrawType drawType,
                                    StencilSupport* stencilSupport);

private:

    GrPathRendererChain();

    void init();

    enum {
        kPreAllocCount = 8,
    };
    bool fInit;
    GrContext*                                          fOwner;
    SkSTArray<kPreAllocCount, GrPathRenderer*, true>    fChain;

    typedef SkRefCnt INHERITED;
};


#endif
