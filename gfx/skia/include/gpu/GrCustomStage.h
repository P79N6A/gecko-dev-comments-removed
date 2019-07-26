






#ifndef GrCustomStage_DEFINED
#define GrCustomStage_DEFINED

#include "GrRefCnt.h"
#include "GrNoncopyable.h"
#include "GrProgramStageFactory.h"
#include "GrCustomStageUnitTest.h"
#include "GrTextureAccess.h"

class GrContext;
class GrTexture;
class SkString;











class GrCustomStage : public GrRefCnt {

public:
    SK_DECLARE_INST_COUNT(GrCustomStage)

    typedef GrProgramStageFactory::StageKey StageKey;

    GrCustomStage();
    virtual ~GrCustomStage();

    

    virtual bool isOpaque(bool inputTextureIsOpaque) const;

    
















    virtual const GrProgramStageFactory& getFactory() const = 0;

    













    virtual bool isEqual(const GrCustomStage&) const;

    

    const char* name() const { return this->getFactory().name(); }

    virtual int numTextures() const;

    

    virtual const GrTextureAccess& textureAccess(int index) const;

    
    GrTexture* texture(int index) const { return this->textureAccess(index).getTexture(); }

    void* operator new(size_t size);
    void operator delete(void* target);

private:
    typedef GrRefCnt INHERITED;
};

#endif
