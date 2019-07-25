






#ifndef GrCustomStage_DEFINED
#define GrCustomStage_DEFINED

#include "GrRefCnt.h"
#include "GrNoncopyable.h"
#include "GrProgramStageFactory.h"

class GrContext;






class GrCustomStage : public GrRefCnt {

public:
    typedef GrProgramStageFactory::StageKey StageKey;

    GrCustomStage();
    virtual ~GrCustomStage();

    

    virtual bool isOpaque(bool inputTextureIsOpaque) const;

    
















    virtual const GrProgramStageFactory& getFactory() const = 0;

    






    virtual bool isEqual(const GrCustomStage *) const = 0;

     

    const char* name() const { return this->getFactory().name(); }

private:

    typedef GrRefCnt INHERITED;
};

#endif
