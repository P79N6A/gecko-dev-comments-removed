






































#ifndef _CC_SIPCCLINE_H
#define _CC_SIPCCLINE_H

#include "CC_Line.h"
#include <map>
#include <iomanip>
#include <sstream>

#include "common/Wrapper.h"

namespace CSF
{
	DECLARE_PTR(CC_SIPCCLine);
    class CC_SIPCCLine : public CC_Line
    {
    private:
        CSF_DECLARE_WRAP(CC_SIPCCLine, cc_lineid_t);

        cc_lineid_t lineId;
        CC_SIPCCLine (cc_lineid_t aLineId) : lineId(aLineId) { }

    public:
        virtual std::string toString() {
        	std::stringstream sstream;
            sstream << "0x" << std::setw( 5 ) << std::setfill( '0' ) << std::hex << lineId;
            return sstream.str();
        };
        virtual cc_lineid_t getID();
        virtual CC_LineInfoPtr getLineInfo ();
        virtual CC_CallPtr createCall ();
    };

};


#endif
