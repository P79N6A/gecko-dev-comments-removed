






#ifndef __CSRUTF8_H
#define __CSRUTF8_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN






class CharsetRecog_UTF8: public CharsetRecognizer {

 public:
		
    virtual ~CharsetRecog_UTF8();		 

    const char *getName() const;

    


    UBool match(InputText *input, CharsetMatch *results) const;
	
};

U_NAMESPACE_END

#endif
#endif 
