














































#ifndef FIFOSamplePipe_H
#define FIFOSamplePipe_H

#include <assert.h>
#include <stdlib.h>
#include "STTypes.h"

namespace soundtouch
{


class FIFOSamplePipe
{
public:
    
    virtual ~FIFOSamplePipe() {}


    
    
    
    
    
    
    
    virtual SAMPLETYPE *ptrBegin() = 0;

    
    
    virtual void putSamples(const SAMPLETYPE *samples,  
                            uint numSamples             
                            ) = 0;


    
    void moveSamples(FIFOSamplePipe &other  
         )
    {
        int oNumSamples = other.numSamples();

        putSamples(other.ptrBegin(), oNumSamples);
        other.receiveSamples(oNumSamples);
    };

    
    
    
    
    
    virtual uint receiveSamples(SAMPLETYPE *output, 
                                uint maxSamples                 
                                ) = 0;

    
    
    
    
    
    virtual uint receiveSamples(uint maxSamples   
                                ) = 0;

    
    virtual uint numSamples() const = 0;

    
    virtual int isEmpty() const = 0;

    
    virtual void clear() = 0;

    
    
    virtual uint adjustAmountOfSamples(uint numSamples) = 0;

};











class FIFOProcessor :public FIFOSamplePipe
{
protected:
    
    FIFOSamplePipe *output;

    
    void setOutPipe(FIFOSamplePipe *pOutput)
    {
        assert(output == NULL);
        assert(pOutput != NULL);
        output = pOutput;
    }


    
    
    FIFOProcessor()
    {
        output = NULL;
    }


    
    FIFOProcessor(FIFOSamplePipe *pOutput   
                 )
    {
        output = pOutput;
    }


    
    virtual ~FIFOProcessor()
    {
    }


    
    
    
    
    
    
    
    virtual SAMPLETYPE *ptrBegin()
    {
        return output->ptrBegin();
    }

public:

    
    
    
    
    
    virtual uint receiveSamples(SAMPLETYPE *outBuffer, 
                                uint maxSamples                    
                                )
    {
        return output->receiveSamples(outBuffer, maxSamples);
    }


    
    
    
    
    
    virtual uint receiveSamples(uint maxSamples   
                                )
    {
        return output->receiveSamples(maxSamples);
    }


    
    virtual uint numSamples() const
    {
        return output->numSamples();
    }


    
    virtual int isEmpty() const
    {
        return output->isEmpty();
    }

    
    
    virtual uint adjustAmountOfSamples(uint numSamples)
    {
        return output->adjustAmountOfSamples(numSamples);
    }

};

}

#endif
