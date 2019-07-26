




#ifndef __nsUCConstructors_h
#define __nsUCConstructors_h

#include <stdint.h>
#include "nscore.h"
#include "nsID.h"
#include "uconvutil.h"

class nsISupports;


NS_METHOD
CreateMultiTableDecoder(int32_t aTableCount,
                        const uRange * aRangeArray, 
                        uScanClassID * aScanClassArray,
                        uMappingTable ** aMappingTable,
                        uint32_t aMaxLengthFactor,
                        
                        nsISupports* aOuter,
                        REFNSIID aIID,
                        void** aResult);

NS_METHOD
CreateMultiTableEncoder(int32_t aTableCount,
                        uScanClassID * aScanClassArray,
                        uShiftOutTable ** aShiftOutTable,
                        uMappingTable  ** aMappingTable,
                        uint32_t aMaxLengthFactor,
                        nsISupports* aOuter,
                        REFNSIID aIID,
                        void** aResult);

NS_METHOD
CreateTableEncoder(uScanClassID aScanClass,
                   uShiftOutTable * aShiftOutTable,
                   uMappingTable  * aMappingTable,
                   uint32_t aMaxLengthFactor,
                   nsISupports* aOuter,
                   REFNSIID aIID,
                   void** aResult);

NS_METHOD
CreateMultiTableEncoder(int32_t aTableCount,
                        uScanClassID * aScanClassArray,
                        uMappingTable  ** aMappingTable,
                        uint32_t aMaxLengthFactor,
                        nsISupports* aOuter,
                        REFNSIID aIID,
                        void** aResult);

NS_METHOD
CreateTableEncoder(uScanClassID aScanClass,
                   uMappingTable  * aMappingTable,
                   uint32_t aMaxLengthFactor,
                   nsISupports* aOuter,
                   REFNSIID aIID,
                   void** aResult);

NS_METHOD
CreateOneByteDecoder(uMappingTable * aMappingTable,
                     nsISupports* aOuter,
                     REFNSIID aIID,
                     void** aResult);

                   
#endif
