



#pragma once

#include "CC_Common.h"

extern "C"
{
#include "ccapi_types.h"
}

namespace CSF
{
    class ECC_API CC_Line
    {
    protected:
        CC_Line () { }

    public:
        virtual ~CC_Line () {};

        virtual std::string toString() = 0;

        virtual cc_lineid_t getID() = 0;
        virtual CC_LineInfoPtr getLineInfo () = 0;
        virtual CC_CallPtr createCall () = 0;
    };
};
